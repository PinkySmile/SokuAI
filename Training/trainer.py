import GameInstance
import time
import sys
import BaseAI
import random

if len(sys.argv) < 3:
    print("Usage:", sys.argv[0], "<game_path> <port> [<SWRSToys.ini>]")

random.seed(time.time())
client = sys.argv[1]
port = int(sys.argv[2])
ini = None if len(sys.argv) == 3 else sys.argv[3]

state = None
game = GameInstance.GameInstance(client, ini, port)
while True:
    time.sleep(0.1)
    try:
        state = game.start_game({
            "stage": 13,
            "music": 13,
            "left": {
                "character": 6,
                "palette": 2,
                "deck": [
                    0, 0, 0, 0,
                    1, 1, 1, 1,
                    2, 2, 2, 2,
                    3, 3, 3, 3,
                    4, 4, 4, 4
                ]
            },
            "right": {
                "character": 6,
                "palette": 0,
                "deck": [
                    5, 5, 5, 5,
                    6, 6, 6, 6,
                    7, 7, 7, 7,
                    8, 8, 8, 8,
                    9, 9, 9, 9
                ]
            },
        })
        break
    except GameInstance.ProtocolError:
        if sys.exc_info()[1].code != 12:
            raise

leftAi = BaseAI.BaseAI()
rightAi = BaseAI.BaseAI()
#game.set_game_speed(999999)
game.set_display_mode(True)
game.set_game_volume(0, 0)
while True:
    state = game.tick({
        "left": leftAi.get_inputs(
            state["left"],
            state["right"],
            state["weather"],
            state["left_objs"],
            state["right_objs"],
        ),
        "right": leftAi.get_inputs(
            state["right"],
            state["left"],
            state["weather"],
            state["right_objs"],
            state["left_objs"],
        ),
    })
