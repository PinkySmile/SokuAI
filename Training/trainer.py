import GameInstance
import time
import sys

if len(sys.argv) < 3:
    print("Usage:", sys.argv[0], "<game_path> <port> [<SWRSToys.ini>]")

c = True
client = sys.argv[1]
port = int(sys.argv[2])
ini = None if len(sys.argv) == 3 else sys.argv[3]

game = GameInstance.GameInstance(client, ini, port)
while True:
    time.sleep(0.1)
    try:
        game.start_game({
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

game.set_game_speed(1200)
game.set_display_mode(False)
game.set_game_volume(0, 0)
while True:
    game.tick({
        "left": {
            "A": False,
            "B": False,
            "C": c,
            "D": False,
            "SW": False,
            "SC": False,
            "H": 0,
            "V": 0,
        },
        "right": {
            "A": False,
            "B": False,
            "C": False,
            "D": False,
            "SW": False,
            "SC": False,
            "H": 0,
            "V": 0,
        }
    })
    c = not c
