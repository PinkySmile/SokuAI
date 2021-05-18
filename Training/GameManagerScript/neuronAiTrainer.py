import SwissTournamentManager
import NeuronAI
import random
import sys
import time
import pprint


if len(sys.argv) < 4:
    print("Usage:", sys.argv[0], "<game_path> <port_start> <nb_game_instances> <population_size> [<SWRSToys.ini>]")
    exit(1)

random.seed(time.time())
client = sys.argv[1]
port = int(sys.argv[2])
nb = int(sys.argv[3])
pop_size = int(sys.argv[4])
ini = None if len(sys.argv) == 5 else sys.argv[5]
tournament = SwissTournamentManager.SwissTournamentManager(nb, port, client, input_delay=0, time_limit=5 * 60 * 60, first_to=3, ini_path=ini)
tournament.ais = [NeuronAI.NeuronAI(random.randint(0, 7), i, -1) for i in range(pop_size)]
for i in tournament.ais:
    i.create_required_path(6, 6)
    i.init(6, 6)
pp = pprint.PrettyPrinter(indent=4)
pp.pprint(tournament.play_tournament())