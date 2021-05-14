import GameInstance
import BaseAI
import time
import sys


class GameManager:
    game_instance: GameInstance.GameInstance = None
    left_ai: BaseAI.BaseAI = None
    right_ai: BaseAI.BaseAI = None

    def __init__(self, game_path, port, ais, tps=60, has_display=True, has_sound=True, ini_path=None):
        self.game_instance = GameInstance.GameInstance(game_path, ini_path, port)
        self.left_ai = ais[0]
        self.right_ai = ais[1]
        self.game_instance.set_display_mode(has_display)
        self.game_instance.set_game_speed(tps)
        if not has_sound:
            self.game_instance.set_game_volume(0, 0)

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
        state = self.start_game_sequence(stage, music, left_params, right_params)
        left_inputs = [BaseAI.BaseAI.EMPTY] * input_delay
        right_inputs = [BaseAI.BaseAI.EMPTY] * input_delay
        self.left_ai.on_game_start(input_delay)
        self.right_ai.on_game_start(input_delay)
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
                    state["left"], state["right"], state["weather"], state["left_objs"], state["right_objs"]
                ))
                right_inputs.append(self.left_ai.get_inputs(
                    state["right"], state["left"], state["weather"], state["right_objs"], state["left_objs"]
                ))

                state = self.game_instance.tick({
                    "left": left_inputs.pop(0),
                    "right": right_inputs.pop(0),
                })
            except GameInstance.GameEndedException:
                winner = sys.exc_info()[1].winner
                if winner == 1:
                    self.left_ai.on_win()
                    self.right_ai.on_lose()
                elif winner == 2:
                    self.right_ai.on_win()
                    self.left_ai.on_lose()
                else:
                    self.left_ai.on_timeout()
                    self.right_ai.on_timeout()
                return winner

    def run(self, left_params, right_params, stage=0, music=0, nb=1, frame_timout=float("inf"), input_delay=0):
        if not self.left_ai.can_play_against(right_params["character"]):
            raise Exception("LeftAI cannot play against character " + str(right_params["character"]))
        if not self.right_ai.can_play_against(left_params["character"]):
            raise Exception("RightAI cannot play against character " + str(left_params["character"]))
        result = []
        for i in range(nb):
            result.append(self.run_once(stage, music, left_params, right_params, frame_timout, input_delay))
        return result
