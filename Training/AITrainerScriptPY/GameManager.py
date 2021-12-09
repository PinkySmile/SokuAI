import time
import sys

from GameInstance import GameInstance, ProtocolError, GameEndedException
import BaseAI

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
    game_instance: GameInstance = None
    left_ai: BaseAI.BaseAI = None
    right_ai: BaseAI.BaseAI = None

    def __init__(self, game_path, port, ais, tps=60, has_display=True, has_sound=False, ini_path=None, just_connect=False):
        self.game_path = game_path
        self.has_display = has_display
        self.has_sound = has_sound
        self.ini_path = ini_path
        self.tps = tps
        self.port = port
        self.just_connect = just_connect
        self.game_instance = None
        self.left_ai = ais[0]
        self.right_ai = ais[1]
        self.start_instance()

    def start_game_sequence(self, stage, music, left_params, right_params):
        while True:
            try:
                return self.game_instance.start_game({
                    "stage": stage,
                    "music": music,
                    "left": left_params,
                    "right": right_params,
                })
            except ProtocolError:
                if sys.exc_info()[1].code != 12:
                    raise
                time.sleep(0.1)

    def start_instance(self):
        if self.game_instance:
            self.game_instance.quit()
        if not self.game_instance:
            self.game_instance = GameInstance(self.game_path, self.ini_path, self.port, self.just_connect)
        else:
            self.game_instance.reconnect(self.game_path, self.ini_path, self.port, self.just_connect)
        self.game_instance.set_display_mode(self.has_display)
        self.game_instance.set_game_speed(self.tps)
        self.game_instance.set_game_volume(self.has_sound[0] if self.has_sound else 0, self.has_sound[1] if self.has_sound else 0)

    def run_once(self, stage, music, left_params, right_params, frame_timout, input_delay, max_crashes=5):
        try:
            if not self.left_ai.can_play_matchup(left_params["character"], right_params["character"]):
                raise Exception("LeftAI cannot play as {} against {}".format(chr_names[left_params["character"]], chr_names[right_params["character"]]))
            if not self.right_ai.can_play_matchup(right_params["character"], left_params["character"]):
                raise Exception("RightAI cannot play as {} against {}".format(chr_names[right_params["character"]], chr_names[left_params["character"]]))

            state = self.start_game_sequence(stage, music, left_params, right_params)
            left_inputs = [BaseAI.BaseAI.EMPTY] * input_delay
            right_inputs = [BaseAI.BaseAI.EMPTY] * input_delay
            self.left_ai.on_game_start(left_params["character"], right_params["character"], input_delay)
            self.right_ai.on_game_start(right_params["character"], left_params["character"], input_delay)
            while True:
                try:
                    frame_timout -= 1

                    #if frame_timout == 17500:
                    #    self.game_instance.set_health(500, 500)
                    #    self.game_instance.set_weather(13, 999, False)
                    #    self.game_instance.set_positions({"x": 40, "y": 0}, {"x": 1240, "y": 0})

                    if frame_timout <= 0:
                        self.game_instance.end_game()

                    left_inputs.append(self.left_ai.get_inputs(
                        state["left"], state["right"], state["left_objs"], state["right_objs"], state["weather"]
                    ))
                    right_inputs.append(self.right_ai.get_inputs(
                        state["right"], state["left"], state["right_objs"], state["left_objs"], state["weather"]
                    ))

                    state = self.game_instance.tick({
                        "left": left_inputs.pop(0),
                        "right": right_inputs.pop(0),
                    })
                except GameEndedException as exc:
                    winner = exc.winner
                    if winner == 1:
                        self.left_ai.on_win(exc.left_score, exc.right_score)
                        self.right_ai.on_lose(exc.right_score, exc.left_score)
                    elif winner == 2:
                        self.right_ai.on_win(exc.left_score, exc.right_score)
                        self.left_ai.on_lose(exc.right_score, exc.left_score)
                    else:
                        self.left_ai.on_timeout(exc.left_score, exc.right_score)
                        self.right_ai.on_timeout(exc.right_score, exc.left_score)
                    return winner, (exc.left_score, state["left"][14]), (exc.right_score, state["right"][14])
        except ConnectionResetError:
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

    def run(self, left_params, right_params, stage=0, music=0, nb=1, frame_timout=float("inf"), input_delay=0, swap=False):
        result = []
        for i in range(nb):
            result.append(self.run_once(stage, music, left_params, right_params, frame_timout, input_delay))
            if swap:
                self.left_ai, self.right_ai = self.right_ai, self.left_ai
                left_params, right_params = right_params, left_params
        return result
