import select
import socket
import subprocess
import struct
import psutil
import os


OPCODE_HELLO = 0
OPCODE_GOODBYE = 1
OPCODE_SPEED = 2
OPCODE_DISPLAY = 3
OPCODE_SOUND = 4
OPCODE_VS_PLAYER_START_GAME = 5
OPCODE_FAULT = 6
OPCODE_ERROR = 7
OPCODE_GAME_FRAME = 8
OPCODE_GAME_INPUTS = 9
OPCODE_GAME_CANCEL = 10
OPCODE_GAME_ENDED = 11
OPCODE_OK = 12
OPCODE_SET_HEALTH = 13
OPCODE_SET_POSITION = 14
OPCODE_SET_WEATHER = 15
OPCODE_RESTRICT_MOVES = 16
OPCODE_VS_COM_START_GAME = 17


class InvalidHandshakeError(Exception):
    pass


class GameCrashedError(Exception):
    def __init__(self, packet):
        self.address, self.code = struct.unpack("II", packet)
        super().__init__("Game crashed at address 0x{:X} with error code 0x{:X}".format(self.address, self.code))


class GameEndedException(Exception):
    def __init__(self, packet):
        self.winner = packet[0]
        self.left_score = packet[1]
        self.right_score = packet[2]
        super().__init__(
            "Winner is {} with a score of {}-{}".format(
                ["no one", "left", "right"][self.winner], self.left_score, self.right_score
            )
        )


class ProtocolError(Exception):
    errors = [
        "INVALID_OPCODE",
        "INVALID_PACKET",
        "INVALID_LEFT_DECK",
        "INVALID_LEFT_CHARACTER",
        "INVALID_LEFT_PALETTE",
        "INVALID_RIGHT_DECK",
        "INVALID_RIGHT_CHARACTER",
        "INVALID_RIGHT_PALETTE",
        "INVALID_MUSIC",
        "INVALID_STAGE",
        "INVALID_MAGIC",
        "HELLO_NOT_SENT",
        "STILL_PLAYING",
        "POSITION_OUT_OF_BOUND",
        "INVALID_HEALTH",
        "INVALID_WEATHER",
        "INVALID_WEATHER_TIME",
        "NOT_IN_GAME"
    ]
    code = 0

    def __init__(self, code):
        self.code = code
        super().__init__("Game responded with error code " + ProtocolError.errors[self.code])


class InvalidPacketError(Exception):
    def __init__(self, packet):
        self.packet = packet
        super().__init__("The received packet is malformed: " + str(packet))


class GameInstance:
    class PlayerState:
        elem_names = [
            "direction",
            "position.x",
            "position.y",
            "action",
            "sequence",
            "pose",
            "poseFrame",
            "frameCount",
            "comboDamage",
            "comboLimit",
            "airBorne",
            "timeStop",
            "hitStop",
            "hp",
            "airDashCount",
            "spirit",
            "maxSpirit",
            "untech",
            "healingCharm",
            "confusion",
            "swordOfRapture",
            "score",
            "hand",
            "cardGauge",
            "skills",
            "fanLevel",
            "rodsLevel",
            "booksLevel",
            "dollLevel",
            "dropLevel",
            "dropInvulTimeLeft",
            "superArmorHp",
            "image",
            "characterSpecificData",
            "objectCount"
        ]
        opponent = None
        objects = None

        @staticmethod
        def parse(packet):
            return struct.unpack('<b2fHHHHIHH?HHHBHHHHHHB10sH15sBBBBBHHH20sB', packet)

        def __init__(self, blob):
            self.data = dict(zip(self.elem_names, self.parse(blob)))
            self.data['hand'] = [
                { 'id': elem[0], 'cost': elem[1] }
                for elem in zip(self.data['hand'][::2], self.data['hand'][1::2])
            ]

        def __getattr__(self, item):
            if item == "position":
                return {
                    'x': self.data['position.x'],
                    'y': self.data['position.y']
                }
            if item == "opponentRelativePos":
                return {
                    'x': self.position["x"] - self.opponent.position["x"] if self.direction == -1 else self.opponent.position["x"] - self.position["x"],
                    'y': self.position["y"] - self.opponent.position["y"]
                }
            if item == "distToLeftCorner":
                return self.position["x"] - 40
            if item == "distToRightCorner":
                return 1240 - self.position["x"]
            if item == "distToFrontCorner":
                return self.distToRightCorner if self.direction == -1 else self.distToLeftCorner
            if item == "distToBackCorner":
                return self.distToRightCorner if self.direction == 1 else self.distToLeftCorner
            if item in self.data:
                return self.data[item]
            return self.__dict__[item]

        def init(self, opponent, objects):
            self.opponent = opponent
            self.objects = objects
            for o in objects:
                o.owner = self
                o.opponent = opponent

        def dict(self):
            result = self.data.copy()
            result["opponentRelativePos.x"] = self.opponentRelativePos["x"]
            result["opponentRelativePos.y"] = self.opponentRelativePos["y"]
            result["distToLeftCorner"] = self.distToLeftCorner
            result["distToRightCorner"] = self.distToRightCorner
            result["distToFrontCorner"] = self.distToFrontCorner
            result["distToBackCorner"] = self.distToBackCorner
            result["objects"] = [i.dict() for i in self.objects]
            return result


    class ObjectState:
        elem_names = [
            "direction",
            "position.x",
            "position.y",
            "action",
            "sequence",
            "hitStop",
            "image",
        ]

        def parse(self, packet):
            return struct.unpack('<B2fHHHH', packet)

        def __init__(self, blob, owner, opponent):
            self.data = dict(zip(self.elem_names, self.parse(blob)))
            self.owner = owner
            self.opponent = opponent

        def __getattr__(self, item):
            if item == "position":
                return {
                    'x': self.data['position.x'],
                    'y': self.data['position.y']
                }
            if item == "relativePosMe":
                return {
                    'x': self.position["x"] - self.owner.position["x"] if self.direction == -1 else self.owner.position["x"] - self.position["x"],
                    'y': self.position["y"] - self.owner.position["y"]
                }
            if item == "relativePosOpponent":
                return {
                    'x': self.position["x"] - self.opponent.position["x"] if self.direction == -1 else self.opponent.position["x"] - self.position["x"],
                    'y': self.position["y"] - self.opponent.position["y"]
                }
            if item in self.data:
                return self.data[item]
            return self.__dict__[item]

        def dict(self):
            result = self.data.copy()
            result["relativePosMe.x"] = self.relativePosMe["x"]
            result["relativePosMe.y"] = self.relativePosMe["y"]
            result["relativePosOpponent.x"] = self.relativePosOpponent["x"]
            result["relativePosOpponent.y"] = self.relativePosOpponent["y"]
            return result


    baseSocket = None
    socket = None
    weather_elem_name = [
        "displayed",
        "active",
        "timer"
    ]
    EMPTY_INPUT = {"A": False, "B": False, "C": False, "D": False, "SC": False, "SW": False, "H": 0, "V": 0}

    def __init__(self, exe_path, port=10800, just_connect=False):
        print(f"Starting instance {exe_path} {port} {just_connect}")
        self.baseSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM, socket.IPPROTO_TCP)
        self.baseSocket.bind(("127.0.0.1", port))
        self.baseSocket.listen(1)
        self.fd = 0
        self.exe_path = exe_path
        self.port = port
        self.just_connect = just_connect
        self.reconnect()

    def wait_for_ready(self):
        while True:
            self.socket.send(struct.pack(
                '<BBBI20sB32sI20sB32s',
                OPCODE_VS_COM_START_GAME, 255, 255,
                2 ** 32 - 1, b'\0' * 20, 255, b'\0' * 32,
                2 ** 32 - 1, b'\0' * 20, 255, b'\0' * 32
            ))
            assert self.socket.recv(1)[0] == OPCODE_ERROR
            err = self.socket.recv(1)[0]
            if err != 12:
                return
            self.socket.send(bytes([OPCODE_GAME_CANCEL]))
            op = self.socket.recv(1)[0]
            if op == OPCODE_ERROR:
                err = self.socket.recv(1)[0]
                if err != 17:
                    return
                continue
            try:
                self.tick({ "left": self.EMPTY_INPUT, "right": self.EMPTY_INPUT })
            except GameEndedException:
                pass
            except ProtocolError:
                pass
            except TimeoutError:
                continue

    def reconnect(self):
        if not self.just_connect:
            if os.name == 'posix':
                args = ['/usr/bin/wine', self.exe_path, str(self.port)]
            else:
                args = [self.exe_path, str(self.port)]
            d = os.path.dirname(self.exe_path)
            self.fd = subprocess.Popen(args, stdin=subprocess.DEVNULL, stdout=open(d + "/stdout.txt", "a"), stderr=open(d + "/stderr.txt", "a"))
            p = psutil.Process(self.fd.pid)
            if os.name == 'posix':
                p.nice(0)
            else:
                p.nice(psutil.HIGH_PRIORITY_CLASS)
        self.socket = self.baseSocket.accept()[0]
        # Send hello packet
        packet = b'\x00\x2A\x9D\x6E\xF5'
        self.socket.send(packet)
        if packet != self.socket.recv(6):
            raise InvalidHandshakeError()

    def recv_game_frame(self, can_timeout=True):
        if can_timeout:
            r, w, x = select.select([self.socket], [], [], 1)
            if not r:
                raise TimeoutError()
        byte = self.socket.recv(1)
        if not len(byte):
            raise ConnectionResetError()
        if byte[0] == OPCODE_GAME_ENDED:
            raise GameEndedException(self.socket.recv(3))
        if byte[0] == OPCODE_ERROR:
            raise ProtocolError(self.socket.recv(1)[0])
        if byte[0] == OPCODE_FAULT:
            raise GameCrashedError(self.socket.recv(8))
        if byte[0] != OPCODE_GAME_FRAME:
            raise InvalidPacketError(byte)
        byte = self.socket.recv(214)
        if len(byte) != 214:
            raise InvalidPacketError(byte)
        result = {
            "left": self.PlayerState(byte[:105]),
            "right": self.PlayerState(byte[105:210]),
            "weather": dict(zip(self.weather_elem_name, struct.unpack('<BBH', byte[210:214])))
        }
        result["left"].init(result["right"], [self.ObjectState(
            self.socket.recv(17), result["left"], result["right"]
        ) for _ in range(result['left'].objectCount)])
        result["right"].init(result["left"], [self.ObjectState(
            self.socket.recv(17), result["right"], result["left"]
        ) for _ in range(result['right'].objectCount)])
        return result

    def start_game(self, params, vs_com):
        packet = struct.pack(
            '<BBBI20sB32sI20sB32s',
            OPCODE_VS_COM_START_GAME if vs_com else OPCODE_VS_PLAYER_START_GAME,
            params["stage"],
            params["music"],
            params["left"]["character"],
            bytes(params["left"]["deck"]),
            params["left"]["palette"],
            bytes(params["left"]["name"].encode() + bytes([0]) * (32 - len(params["left"]["name"]))),
            params["right"]["character"],
            bytes(params["right"]["deck"]),
            params["right"]["palette"],
            bytes(params["right"]["name"].encode() + bytes([0]) * (32 - len(params["right"]["name"])))
        )
        self.socket.send(packet)
        if self.socket.recv(1)[0] == OPCODE_ERROR:
            raise ProtocolError(self.socket.recv(1)[0])
        return self.recv_game_frame(False)

    def quit(self):
        try:
            self.socket.send(bytes([OPCODE_GOODBYE]))
            if self.socket.recv(1) == OPCODE_ERROR:
                raise ProtocolError(self.socket.recv(1))
            self.socket.close()
        except ConnectionResetError:
            pass
        except BrokenPipeError:
            pass
        self.socket = None

    def send_inputs(self, inputs):
        l = {i: int(j) for i, j in inputs["left"].items()}
        r = {i: int(j) for i, j in inputs["right"].items()}
        left = ((l["A"] & 1) << 0) | \
               ((l["B"] & 1) << 1) | \
               ((l["C"] & 1) << 2) | \
               ((l["D"] & 1) << 3) | \
               ((l["SC"] & 1) << 4) | \
               ((l["SW"] & 1) << 5) | \
               ((((l["H"] > 0) - (l["H"] < 0)) & 3) << 6) | \
               ((((l["V"] > 0) - (l["V"] < 0)) & 3) << 8)
        right = ((r["A"] & 1) << 0) | \
                ((r["B"] & 1) << 1) | \
                ((r["C"] & 1) << 2) | \
                ((r["D"] & 1) << 3) | \
                ((r["SC"] & 1) << 4) | \
                ((r["SW"] & 1) << 5) | \
                ((((r["H"] > 0) - (r["H"] < 0)) & 3) << 6) | \
                ((((r["V"] > 0) - (r["V"] < 0)) & 3) << 8)
        self.socket.send(struct.pack('<BHH', OPCODE_GAME_INPUTS, left, right))

    def tick(self, inputs):
        self.send_inputs(inputs)
        return self.recv_game_frame()

    def set_game_speed(self, tps):
        self.socket.send(struct.pack('<BI', OPCODE_SPEED, tps))
        if self.socket.recv(1)[0] == OPCODE_ERROR:
            raise ProtocolError(self.socket.recv(1)[0])

    def end_game(self):
        self.socket.send(bytes([OPCODE_GAME_CANCEL]))
        if self.socket.recv(1)[0] == OPCODE_ERROR:
            raise ProtocolError(self.socket.recv(1)[0])

    def set_display_mode(self, shown=True):
        self.socket.send(bytes([OPCODE_DISPLAY, shown]))
        if self.socket.recv(1)[0] == OPCODE_ERROR:
            raise ProtocolError(self.socket.recv(1)[0])

    def set_game_volume(self, sfx, bgm):
        self.socket.send(bytes([OPCODE_SOUND, sfx, bgm]))
        if self.socket.recv(1)[0] == OPCODE_ERROR:
            raise ProtocolError(self.socket.recv(1)[0])

    def set_positions(self, left, right):
        self.socket.send(struct.pack(
            '<Bffff',
            OPCODE_SET_POSITION,
            left["x"],
            left["y"],
            right["x"],
            right["y"],
        ))
        if self.socket.recv(1)[0] == OPCODE_ERROR:
            raise ProtocolError(self.socket.recv(1)[0])

    def set_health(self, left, right):
        self.socket.send(struct.pack(
            '<BHH',
            OPCODE_SET_HEALTH,
            left,
            right,
        ))
        if self.socket.recv(1)[0] == OPCODE_ERROR:
            raise ProtocolError(self.socket.recv(1)[0])

    def set_weather(self, weather, timer, freeze_timer=False):
        self.socket.send(struct.pack('<BIH?', OPCODE_SET_WEATHER, weather, timer, freeze_timer))
        if self.socket.recv(1)[0] == OPCODE_ERROR:
            raise ProtocolError(self.socket.recv(1)[0])

    def set_restricted_moves(self, moves):
        self.socket.send(struct.pack('<BB{}H'.format(len(moves)), OPCODE_RESTRICT_MOVES, len(moves), *moves))
        if self.socket.recv(1)[0] == OPCODE_ERROR:
            raise ProtocolError(self.socket.recv(1)[0])
