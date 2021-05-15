import GameManager
import time
import sys
import BaseAI
import ReactAI
import NeuronAI
import random

if len(sys.argv) < 3:
    print("Usage:", sys.argv[0], "<game_path> <port> [<SWRSToys.ini>]")

random.seed(time.time())
client = sys.argv[1]
port = int(sys.argv[2])
ini = None if len(sys.argv) == 3 else sys.argv[3]

# ReactAI.ReactAI(0)
player1Ai = BaseAI.BaseAI(6, 2)
player2AiParent1 = NeuronAI.NeuronAI(2, 0, -1)
player2AiParent2 = NeuronAI.NeuronAI(2, 1, -1)
player1 = player1Ai.get_prefered_character()
player2 = player2AiParent2.get_prefered_character()

player2AiParent1.create_required_path(player2["character"], player1["character"])
player2AiParent1.init(player2["character"], player1["character"])
player2AiParent2.create_required_path(player2["character"], player1["character"])
player2AiParent2.init(player2["character"], player1["character"])

player2Ai = player2AiParent1.mate_once(player2AiParent2, 0, player2["character"], player1["character"])

game = GameManager.GameManager(client, port, (player1Ai, player2Ai), tps=600, ini_path=ini, has_sound=False, has_display=True)
print(game.run(player1, player2, stage=13, music=13, nb=5, input_delay=0))
