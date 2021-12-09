import SwissTournamentManager
import NeuronAI
import random
import sys
import time
import math
import pprint
import psutil
import os

p = psutil.Process(os.getpid())
p.nice(psutil.HIGH_PRIORITY_CLASS)

if len(sys.argv) < 4:
    print("Usage:", sys.argv[0], "<game_path> <port_start> <nb_game_instances> <population_size> <input_delay> [<SWRSToys.ini>]")
    exit(1)

lowering_factor = 6
random.seed(time.time())
client = sys.argv[1]
port = int(sys.argv[2])
nb = int(sys.argv[3])
pop_size = int(sys.argv[4])
input_delay = int(sys.argv[5])
ini = None if len(sys.argv) == 6 else sys.argv[6]
tournament = SwissTournamentManager.SwissTournamentManager(nb, port, client, input_delay=input_delay, time_limit=5 * 60 * 60, first_to=1, ini_path=ini)
latest = NeuronAI.NeuronAI.get_latest_gen(6, 6)


def generate_next_generation(tournament_results):
    best_candidates = [t[0] for t in tournament_results[-(len(tournament_results) // lowering_factor):]]
    pairs = [(best_candidates[j], best_candidates[j + i]) for i in range(1, len(best_candidates)) for j in range(0, len(best_candidates) - i)]
    result = []
    i = 0
    while True:
        for parent1, parent2 in pairs:
            result.append(parent1.mate_once(parent2, i, 6, 6))
            i += 1
            if i == pop_size:
                return result + best_candidates


def main():
    # first one is a bit special since we either create the AIs
    print("Loading generation {}".format(latest))
    # TODO: Store the best of latest generation
    tournament.ais = [NeuronAI.NeuronAI(random.randint(0, 71), i, latest) for i in range(pop_size)]
    if latest > 0:
        tournament.ais += [NeuronAI.NeuronAI(random.randint(0, 71), i, latest - 1) for i in range(pop_size // lowering_factor)]
    else:
        tournament.ais += [NeuronAI.NeuronAI(random.randint(0, 71), i + pop_size, latest - 1) for i in range(pop_size // lowering_factor)]
    for i in tournament.ais:
        i.create_required_path(6, 6)
        i.init(6, 6)
    pp = pprint.PrettyPrinter(indent=4)
    nb = math.ceil(math.log(len(tournament.ais), 2))
    while True:
        results = sorted(tournament.play_tournament(nb), key=lambda x: x[1] * nb + x[2], reverse=True)
        pp.pprint(results)
        tournament.ais = generate_next_generation(results)


if __name__ == '__main__':
    main()
