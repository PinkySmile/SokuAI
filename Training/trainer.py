import GameManager
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

player1 = {
    "character": 6,
    "palette": 2,
    "deck": [
        0, 0, 0, 0,
        1, 1, 1, 1,
        2, 2, 2, 2,
        3, 3, 3, 3,
        4, 4, 4, 4
    ]
}
player2 = {
    "character": 6,
    "palette": 0,
    "deck": [
        5, 5, 5, 5,
        6, 6, 6, 6,
        7, 7, 7, 7,
        8, 8, 8, 8,
        9, 9, 9, 9
    ]
}

game = GameManager.GameManager(client, port, (BaseAI.BaseAI(), BaseAI.BaseAI()), tps=600, ini_path=ini)
print(game.run(13, 13, player1, player2, 5, 18000))
