//
// Created by PinkySmile on 23/05/2021.
//


#include <Windows.h>
#include <cstdio>
#include <ctime>
#include <string>
#include "SwissTournamentManager.hpp"
#include "NeuronAi.hpp"

using namespace Trainer;

#define MATING_FACTOR   10
#define LOWERING_FACTOR 5

void generateNextGeneration(const std::vector<PlayerEntry> &results, std::vector<std::unique_ptr<NeuronAI>> &ais, unsigned popSize, unsigned currentLatestGen) {
	std::vector<std::pair<const NeuronAI *, const NeuronAI *>> pairs;
	std::vector<const NeuronAI *> bestCandidates;
	std::vector<NeuronAI *> result;
	int i;

	for (i = 0; i < results.size() / MATING_FACTOR; i++)
		bestCandidates.push_back(reinterpret_cast<const NeuronAI *>(results[i].ai));

	for (i = 1; i < bestCandidates.size(); i++)
		for (int j = 0; j < bestCandidates.size() - i; j++)
			pairs.emplace_back(bestCandidates[j], bestCandidates[j + i]);

	for (i = 0;;)
		for (auto &parents : pairs) {
			result.push_back(parents.first->mateOnce(*parents.second, i, SokuLib::CHARACTER_REMILIA, SokuLib::CHARACTER_REMILIA, currentLatestGen));
			i++;
			if (i == popSize) {
				ais.erase(std::remove_if(ais.begin(), ais.end(), [&results, popSize](const std::unique_ptr<NeuronAI> &a){
					auto end = min(results.end(), results.begin() + popSize / LOWERING_FACTOR);

					return std::find_if(results.begin(), end, [&a](const PlayerEntry &e){
						return e.ai == &*a;
					}) == end;
				}), ais.end());
				ais.reserve(ais.size() + result.size());
				for (auto child : result)
					ais.emplace_back(child);
				return;
			}
		}
}

int main(int argc, char *argv[])
{
	if (argc < 6) {
		printf("Usage: %s <game_path> <port_start> <nb_game_instances> <population_size> <input_delay> [<SWRSToys.ini>]\n", argv[0]);
		return EXIT_FAILURE;
	}

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	srand(time(nullptr));

	std::string client = argv[1];
	unsigned short port = std::stoul(argv[2]);
	unsigned nb = std::stoul(argv[3]);
	unsigned popSize = std::stoul(argv[4]);
	unsigned inputDelay = std::stoul(argv[5]);
	const char *ini = argc == 7 ? argv[6] : nullptr;
	SwissTournamentManager tournament(nb, port, client, inputDelay, 5 * 60 * 60, 1, ini);
	auto latest = NeuronAI::getLatestGen(SokuLib::CHARACTER_REMILIA, SokuLib::CHARACTER_REMILIA);
	std::vector<std::unique_ptr<NeuronAI>> ais;

	// first one is a bit special since we either create the AIs
	printf("Loading generation %i\n", latest);

	// TODO: Store the best of latest generation
	ais.reserve(popSize + popSize / LOWERING_FACTOR);
	for (int i = 0; i < popSize; i++)
		ais.emplace_back(new NeuronAI(rand() % 72, i, latest));
	if (latest > 0)
		for (int i = 0; i < popSize / LOWERING_FACTOR; i++)
			ais.emplace_back(new NeuronAI(rand() % 72, i, latest - 1));
	else
		for (int i = 0; i < popSize / LOWERING_FACTOR; i++)
			ais.emplace_back(new NeuronAI(rand() % 72, i + popSize, -1));
	for (auto &ai : ais) {
		ai->createRequiredPath(SokuLib::CHARACTER_REMILIA, SokuLib::CHARACTER_REMILIA);
		ai->init(SokuLib::CHARACTER_REMILIA, SokuLib::CHARACTER_REMILIA);
	}
	nb = ceil(log2(ais.size()));
	while (true) {
		std::vector<BaseAI *> baseAis;

		baseAis.reserve(ais.size());
		for (auto &ai : ais)
			baseAis.push_back(&*ai);

		auto results = tournament.playTournament(baseAis, nb);

		std::sort(results.begin(), results.end(), [](const PlayerEntry &e1, const PlayerEntry &e2){
			if (e1.score != e2.score)
				return e1.score > e2.score;
			return e1.wins > e2.wins;
		});
		generateNextGeneration(results, ais, popSize, latest);
		latest++;
	}
}
