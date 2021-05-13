import socket
import subprocess
import struct


OPCODE_HELLO = 0
OPCODE_GOODBYE = 1
OPCODE_SPEED = 2
OPCODE_DISPLAY = 3
OPCODE_SOUND = 4
OPCODE_START_GAME = 5
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


class InvalidHandshakeError(Exception):
    pass


class GameEndedException(Exception):
    winner = 0

    def __init__(self, winner):
        self.winner = winner
        super().__init__("Winner is " + ["no one", "left", "right"][winner])


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
        super().__init__("The received packet is malformed: " + str(packet))


class GameInstance:
    baseSocket = None
    socket = None
    state_elems_names = [
        "direction",
        "opponentRelativePos.x",
        "opponentRelativePos.y",
        "distToLeftCorner",
        "distToRightCorner",
        "action",
        "actionBlockId",
        "animationCounter",
        "animationSubFrame",
        "frameCount",
        "comboDamage",
        "comboLimit",
        "airBorne",
        "hp",
        "airDashCount",
        "spirit",
        "maxSpirit",
        "untech",
        "healingCharm",
        "swordOfRapture",
        "score",
        "hand",
        "cardGauge",
        "skills",
        "fanLevel",
        "dropInvulTimeLeft",
        "superArmorTimeLeft",
        "superArmorHp",
        "milleniumVampireTimeLeft",
        "philosoferStoneTimeLeft",
        "sakuyasWorldTimeLeft",
        "privateSquareTimeLeft",
        "orreriesTimeLeft",
        "mppTimeLeft",
        "kanakoCooldown",
        "suwakoCooldown",
        "objectCount"
    ]

    def __init__(self, exe_path, ini_path, port):
        if ini_path is None:
            subprocess.Popen([exe_path, str(port)])
        else:
            subprocess.Popen([exe_path, ini_path, str(port)])
        self.baseSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM, socket.IPPROTO_TCP)
        self.baseSocket.bind(("127.0.0.1", port))
        self.baseSocket.listen(5)
        self.socket = self.baseSocket.accept()[0]
        # Send hello packet
        packet = b'\x00\x2A\x9D\x6E\xF5'
        self.socket.send(packet)
        if packet != self.socket.recv(6):
            raise InvalidHandshakeError()

    @staticmethod
    def parse_character_state(packet):
        return struct.unpack('<b2fffHHHHIHH?HBHHHHHB5sH15sBHHHHHHHHHHHB', packet)

    def recv_game_frame(self):
        byte = self.socket.recv(199)
        if byte[0] == OPCODE_GAME_ENDED:
            raise GameEndedException(byte[1])
        if byte[0] == OPCODE_ERROR:
            raise ProtocolError(byte[1])
        if len(byte) != 199:
            print(byte)
            raise InvalidPacketError(byte)
        if byte[0] != OPCODE_GAME_FRAME:
            raise InvalidPacketError(byte)
        result = {
            "left": GameInstance.parse_character_state(byte[1:95]),
            "right": GameInstance.parse_character_state(byte[95:189]),
            "weather": struct.unpack('<IIH', byte[189:199]),
            "left_objs": [],
            "right_objs": []
        }
        for i in range(result["left"][-1]):
            result["left_objs"].append(struct.unpack('<BffffHI', self.socket.recv(23)))
        for i in range(result["right"][-1]):
            result["right_objs"].append(struct.unpack('<BffffHI', self.socket.recv(23)))
        return result

    def start_game(self, params):
        packet = struct.pack(
            '<BBBI20sBI20sB',
            OPCODE_START_GAME,
            params["stage"],
            params["music"],
            params["left"]["character"],
            bytes(params["left"]["deck"]),
            params["left"]["palette"],
            params["right"]["character"],
            bytes(params["right"]["deck"]),
            params["right"]["palette"]
        )
        self.socket.send(packet)
        if self.socket.recv(1)[0] == OPCODE_ERROR:
            raise ProtocolError(self.socket.recv(1)[0])
        return self.recv_game_frame()

    def quit(self):
        self.socket.send(bytes([OPCODE_GOODBYE]))
        if self.socket.recv(1) == OPCODE_ERROR:
            raise ProtocolError(self.socket.recv(1))
        try:
            self.socket.recv(1)
            raise Exception("Wtf ?")
        except ConnectionResetError:
            self.socket = None

    def tick(self, inputs):
        l = inputs["left"]
        r = inputs["right"]
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
