import time
import sys
import threading

from GameInstance import GameInstance, ProtocolError, GameEndedException, GameCrashedError
from BaseAI import BaseAI

chr_names = [
    "Reimu",
    "Marisa",
    "Sakuya",
    "Alice",
    "Patchouli",
    "Youmu",
    "Remilia",
    "Yuyuko",
    "Yukari",
    "Suika",
    "Reisen",
    "Aya",
    "Komachi",
    "Iku",
    "Tenshi",
    "Sanae",
    "Cirno",
    "Meiling",
    "Utsuho",
    "Suwako"
]


class GameManager:
    class GameFrame:
        def __init__(self, frame):
            self.weather = frame['weather']
            self.left = frame['left']
            self.right = frame['right']
            self.sideLeft = True

        def __getattr__(self, name):
            if name == "me":
                return self.left if self.sideLeft else self.right
            if name == "opponent":
                return self.left if not self.sideLeft else self.right
            return self.__dict__[name]


    game_instance: GameInstance = None
    left_ai: BaseAI = None
    right_ai: BaseAI = None
    fightingCPU = False
    lastFrame = None

    def __init__(self, game_path, port, ais, tps=60, has_display=True, has_sound=(False, False), just_connect=False):
        self.game_path = game_path
        self.has_display = has_display
        self.has_sound = has_sound
        self.tps = tps
        self.port = port
        self.just_connect = just_connect
        self.left_ai = ais[0]
        self.right_ai = ais[1]
        self.start_instance()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.game_instance.quit()

    def start_game_sequence(self, stage, music, left_params, right_params):
        while True:
            try:
                self.fightingCPU = self.right_ai is None
                return self.game_instance.start_game({
                    "stage": stage,
                    "music": music,
                    "left": left_params,
                    "right": right_params,
                }, self.fightingCPU)
            except ProtocolError:
                if sys.exc_info()[1].code != 12:
                    raise
                time.sleep(0.1)

    def start_instance(self):
        if self.game_instance:
            self.game_instance.quit()
        if not self.game_instance:
            self.game_instance = GameInstance(self.game_path, self.port, self.just_connect)
        else:
            self.game_instance.reconnect(self.game_path, self.port, self.just_connect)
        self.game_instance.set_display_mode(self.has_display)
        self.game_instance.set_game_speed(self.tps)
        self.game_instance.set_game_volume(self.has_sound[0] if self.has_sound else 0, self.has_sound[1] if self.has_sound else 0)

    def run_once(self, stage, music, left_params, right_params, frame_timout, input_delay, max_crashes=5):
        crash_exc = None
        try:
            if not self.left_ai.can_play_matchup(left_params["character"], right_params["character"]):
                raise Exception("LeftAI cannot play as {} against {}".format(chr_names[left_params["character"]], chr_names[right_params["character"]]))
            if not self.right_ai.can_play_matchup(right_params["character"], left_params["character"]):
                raise Exception("RightAI cannot play as {} against {}".format(chr_names[right_params["character"]], chr_names[left_params["character"]]))

            # We discard the very first frame and always send empty inputs because we can't move for 30f anyway
            self.lastFrame = self.GameFrame(self.start_game_sequence(stage, music, left_params, right_params))
            state = self.GameFrame(self.game_instance.tick({ "left": BaseAI.EMPTY, "right": BaseAI.EMPTY }))
            left_inputs = [BaseAI.EMPTY] * input_delay
            right_inputs = [BaseAI.EMPTY] * input_delay
            self.left_ai.on_game_start(left_params["character"], right_params["character"], input_delay)
            if self.right_ai:
                self.right_ai.on_game_start(right_params["character"], left_params["character"], input_delay)
            while True:
                try:
                    frame_timout -= 1

                    if frame_timout <= 0:
                        self.game_instance.end_game()

                    left_inputs.append(self.left_ai.get_inputs(state, self.lastFrame))
                    if self.right_ai:
                        self.lastFrame.sideLeft = False
                        state.sideLeft = False
                        right_inputs.append(self.right_ai.get_inputs(state, self.lastFrame))
                        self.lastFrame.sideLeft = True
                        state.sideLeft = True
                    else:
                        right_inputs.append(BaseAI.EMPTY)

                    self.lastFrame = state
                    state = self.GameFrame(self.game_instance.tick({
                        "left": left_inputs.pop(0),
                        "right": right_inputs.pop(0),
                    }))
                except GameEndedException as exc:
                    winner = exc.winner
                    if winner == 1:
                        self.left_ai.on_win(exc.left_score, exc.right_score)
                        if self.right_ai:
                            self.right_ai.on_lose(exc.right_score, exc.left_score)
                    elif winner == 2:
                        self.left_ai.on_lose(exc.left_score, exc.right_score)
                        if self.right_ai:
                            self.right_ai.on_win(exc.right_score, exc.left_score)
                    else:
                        self.left_ai.on_timeout(exc.left_score, exc.right_score)
                        if self.right_ai:
                            self.right_ai.on_timeout(exc.right_score, exc.left_score)
                    return winner, (exc.left_score, state.left.hp), (exc.right_score, state.right.hp)
        except ConnectionResetError:
            pass
        except GameCrashedError as crash:
            crash_exc = crash
            pass
        except:
            raise
        if crash_exc:
            print("Game crashed!")
            print(crash_exc.args[0])
        self.left_ai.on_game_crashed()
        if self.right_ai:
            self.right_ai.on_game_crashed()
        print("Our list of allies grows thin !")
        code = self.game_instance.fd.wait()
        print("Exit code {}".format(hex(code)))
        print("Restarting game instance...")
        self.start_instance()
        if max_crashes == 0:
            print("Too many crashes, aborting the game")
            raise
        else:
            print("Restarting game from beginning...")
            return self.run_once(stage, music, left_params, right_params, frame_timout, input_delay, max_crashes - 1)

    def run(self, left_params, right_params, stage=0, music=0, nb=1, frame_timout=float("inf"), input_delay=0, swap=False, max_crashes=5):
        result = []
        for i in range(nb):
            result.append(self.run_once(stage, music, left_params, right_params, frame_timout, input_delay, max_crashes=max_crashes))
            if swap:
                self.left_ai, self.right_ai = self.right_ai, self.left_ai
                left_params, right_params = right_params, left_params
        return result


class GameOpenThread(threading.Thread):
    def __init__(self, container, *args):
        super().__init__(None, None)
        self.container = container
        self.args = args

    def run(self):
        self.container.append(GameManager.GameManager(*self.args))