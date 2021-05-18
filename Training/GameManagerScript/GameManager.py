import GameInstance
import BaseAI
import time
import sys

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
    game_instance: GameInstance.GameInstance = None
    left_ai: BaseAI.BaseAI = None
    right_ai: BaseAI.BaseAI = None

    def __init__(self, game_path, port, ais, tps=60, has_display=True, has_sound=False, ini_path=None, just_connect=False):
        self.game_instance = GameInstance.GameInstance(game_path, ini_path, port, just_connect)
        self.left_ai = ais[0]
        self.right_ai = ais[1]
        self.port = port
        self.game_instance.set_display_mode(has_display)
        self.game_instance.set_game_speed(tps)
        self.game_instance.set_game_volume(has_sound[0] if has_sound else 0, has_sound[1] if has_sound else 0)

    def start_game_sequence(self, stage, music, left_params, right_params):
        while True:
            try:
                return self.game_instance.start_game({
                    "stage": stage,
                    "music": music,
                    "left": left_params,
                    "right": right_params,
                })
            except GameInstance.ProtocolError:
                if sys.exc_info()[1].code != 12:
                    raise
                time.sleep(0.1)

    def run_once(self, stage, music, left_params, right_params, frame_timout, input_delay):
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
            except GameInstance.GameEndedException:
                exc = sys.exc_info()[1]
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

    def run(self, left_params, right_params, stage=0, music=0, nb=1, frame_timout=float("inf"), input_delay=0):
        result = []
        for i in range(nb):
            result.append(self.run_once(stage, music, left_params, right_params, frame_timout, input_delay))
        return result
