#!/bin/env python3
from Soku.GameManager import GameManager
import sys
from BaseAI import BaseAI
import random

if len(sys.argv) < 3:
    print("Usage:", sys.argv[0], "<game_path> <port> [<SWRSToys.ini>]")

client = sys.argv[1]
port = int(sys.argv[2])

with GameManager(client, port, (None, None), tps=60, has_sound=(0, 0), has_display=False) as game:
    left, right, speed = [int(i) for i in input("<left chr> <right chr> <speed>: ").split(" ")]
    game.game_instance.set_game_speed(speed)
    game.left_ai = BaseAI(left, 0)
    game.right_ai = BaseAI(right, 1)
    player1 = game.left_ai.get_params()
    player2 = game.right_ai.get_params()
    print(game.run(player1, player2, stage=0, music=0, input_delay=0, max_crashes=0))
