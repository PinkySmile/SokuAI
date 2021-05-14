import BaseAI
from GameInstance import GameInstance
import pprint


pp = pprint.PrettyPrinter(indent=4)


class ReactAI(BaseAI.BaseAI):
    def __init__(self, palette):
        super().__init__(6, palette)
        self.input_delay = 0

    def on_win(self):
        pass

    def on_lose(self):
        pass

    def on_timeout(self):
        self.on_lose()

    def on_game_start(self, my_chr, opponent_chr, input_delay):
        self.input_delay = input_delay
        pass

    def can_play_as(self, char_id):
        return char_id == 6  # We can only play Remilia

    def can_play_against(self, char_id):
        return char_id == 6  # We can only play against Remilia

    def get_action(self, me, opponent, my_projectiles, opponent_projectiles, weather_infos):
        my_state = dict(zip(GameInstance.state_elems_names, me))
        op_state = dict(zip(GameInstance.state_elems_names, opponent))
        my_objs = [dict(zip(GameInstance.obj_elem_names, i)) for i in my_projectiles]
        op_objs = [dict(zip(GameInstance.obj_elem_names, i)) for i in opponent_projectiles]
        weather = dict(zip(GameInstance.weather_elem_name, weather_infos))
        return "nothing"
