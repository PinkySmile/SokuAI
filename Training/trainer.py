import GameManager
import DeckFactory
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

deck_factory = DeckFactory.DeckFactory()

player1 = {
    "character": 6,
    "palette": 2,
    "deck": deck_factory.build_deck(6)
}
player2 = {
    "character": 6,
    "palette": 0,
    "deck":  deck_factory.build_deck(6)
}

game = GameManager.GameManager(client, port, (BaseAI.BaseAI(), BaseAI.BaseAI()), tps=600, ini_path=ini, has_sound=False, has_display=False)
print(game.run(13, 13, player1, player2, 5, 18000))
