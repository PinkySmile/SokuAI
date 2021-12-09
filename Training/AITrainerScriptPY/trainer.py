import GameManager
import sys
import BaseAI
import ReactAI
import NeuronAI
import random

if len(sys.argv) < 3:
    print("Usage:", sys.argv[0], "<game_path> <port> [<SWRSToys.ini>]")

client = sys.argv[1]
port = int(sys.argv[2])
ini = None if len(sys.argv) == 3 else sys.argv[3]

# ReactAI.ReactAI(0)
player1Ai = BaseAI.BaseAI(20, 0)
player2Ai = BaseAI.BaseAI(20, 1)
player1 = player1Ai.get_prefered_character()
player2 = player2Ai.get_prefered_character()

game = GameManager.GameManager(client, port, (player1Ai, player2Ai), tps=60, ini_path=ini, has_sound=(50, 50), has_display=True)
print("Done")
while True:
    left, right, speed = [int(i) for i in input().split(" ")]
    game.game_instance.set_game_speed(speed)
    game.left_ai = BaseAI.BaseAI(left, 0)
    game.right_ai = BaseAI.BaseAI(right, 1)
    player1 = game.left_ai.get_prefered_character()
    player2 = game.right_ai.get_prefered_character()
    print(game.run(player1, player2, stage=0, music=0, input_delay=0))
