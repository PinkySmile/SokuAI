import GameManager
import time
import sys
import BaseAI
import ReactAI
import random

if len(sys.argv) < 3:
    print("Usage:", sys.argv[0], "<game_path> <port> [<SWRSToys.ini>]")

random.seed(time.time())
client = sys.argv[1]
port = int(sys.argv[2])
ini = None if len(sys.argv) == 3 else sys.argv[3]

player1Ai = BaseAI.BaseAI(6, 2)
player2Ai = ReactAI.ReactAI(0)
player1 = player1Ai.get_prefered_character()
player2 = player2Ai.get_prefered_character()

game = GameManager.GameManager(client, port, (player1Ai, player2Ai), tps=600, ini_path=ini, has_sound=False, has_display=True)
print(game.run(player1, player2, stage=13, music=13, nb=5, frame_timout=18000, input_delay=0))
