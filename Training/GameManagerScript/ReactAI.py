import BaseAI
from GameInstance import GameInstance

remilia_high_block_moves = [
    302,
    306,
    307,
    308,
    309,
    320
]
remilia_low_block_moves = [
    301,
    304
]


def get_right_be(me, op, move, char):
    if me['maxSpirit'] == 0:
        return get_right_block(move, char)
    if move == 304:
        return "be2"
    if move == 302 and op["frameCount"] > 15:
        return "be6"
    if move == 307:
        return "be6"
    if move == 308 and me["distToGround"] == 0:
        return "be2"
    if move == 308 and me["distToGround"] > 60:
        return "be6"
    if move == 309:
        return "be6"
    if move == 325 and op["frameCount"] > 5 and me["distToGround"] > 60:
        return "be6"
    return get_right_block(move, char)


def get_right_block(move, char):
    if char != 6:
        return "backward"
    if move in remilia_low_block_moves:
        return "block_low"
    return "backward"


class ReactAI(BaseAI.BaseAI):
    def __init__(self, palette):
        super().__init__(6, palette)
        self.input_delay = 0

    # def on_win(self):
    #     pass

    # def on_lose(self):
    #     pass

    # def on_timeout(self):
    #     self.on_lose()

    def on_game_start(self, my_chr, opponent_chr, input_delay):
        self.input_delay = input_delay
        pass

    def can_play_matchup(self, my_char_id, op_char_id):
        return my_char_id == 6 and op_char_id == 6  # We can only play Remirror

    def get_action(self, me, opponent, my_projectiles, opponent_projectiles, weather_infos):
        my_state = dict(zip(GameInstance.state_elems_names, me))
        op_state = dict(zip(GameInstance.state_elems_names, opponent))
        my_objs = [dict(zip(GameInstance.obj_elem_names, i)) for i in my_projectiles]
        op_objs = [dict(zip(GameInstance.obj_elem_names, i)) for i in opponent_projectiles]
        weather = dict(zip(GameInstance.weather_elem_name, weather_infos))

        print(op_state["action"], op_state["actionBlockId"], op_state["animationCounter"], op_state["animationSubFrame"], op_state["frameCount"])
        if 300 <= op_state["action"] < 400 and 150 <= my_state["action"] <= 158:
            return get_right_be(my_state, op_state, op_state["action"], 6)
        if 300 <= op_state["action"] < 400:
            return get_right_block(op_state["action"], 6)
        return "nothing"
