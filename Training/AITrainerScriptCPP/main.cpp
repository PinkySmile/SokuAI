//
// Created by PinkySmile on 23/05/2021.
//

#include <random>
#include <Windows.h>
#include <cstdio>
#include <ctime>
#include <string>
#include <fstream>
#include <iostream>
#include <direct.h>
#include "SwissTournamentManager.hpp"
#include "GeneticAI.hpp"

using namespace Trainer;

//#define NEURON_COUNT    1000
//#define GENES_COUNT     100000
#define TRAINING_CHARACTER CHARACTER_REMILIA
#define MATING_FACTOR   10
#define LOWERING_FACTOR 5

unsigned NEURON_COUNT;
unsigned GENES_COUNT;

void generateNextGeneration(const std::vector<PlayerEntry> &results, std::vector<std::unique_ptr<GeneticAI>> &ais, unsigned popSize, unsigned currentLatestGen) {
	std::string basePath = "GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT) + "/" + std::to_string(SokuLib::TRAINING_CHARACTER) + " vs " + std::to_string(SokuLib::TRAINING_CHARACTER) + "/ais/";
	std::vector<std::pair<const GeneticAI *, const GeneticAI *>> pairs;
	std::vector<const GeneticAI *> bestCandidates;
	std::vector<GeneticAI *> result;
	int i;

	for (i = 0; i < results.size() / MATING_FACTOR; i++)
		bestCandidates.push_back(reinterpret_cast<const GeneticAI *>(results[i].ai));

	for (i = 1; i < bestCandidates.size(); i++)
		for (int j = 0; j < bestCandidates.size() - i; j++)
			pairs.emplace_back(bestCandidates[j], bestCandidates[j + i]);

	for (i = 0;;)
		for (auto &parents : pairs) {
			result.push_back(new GeneticAI(currentLatestGen + 1, i, NEURON_COUNT, *parents.first, *parents.second));
			result.back()->save(basePath + std::to_string(currentLatestGen + 1) + "_" + std::to_string(i) + ".ai");
			i++;
			if (i == popSize) {
				ais.erase(std::remove_if(ais.begin(), ais.end(), [&results, popSize](const std::unique_ptr<GeneticAI> &a){
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

void saveTournamentsResults(const std::vector<PlayerEntry> &results, unsigned popSize, unsigned currentLatestGen)
{
	std::ofstream stream{
		"GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT) + "/" +
			std::to_string(SokuLib::TRAINING_CHARACTER) + " vs " + std::to_string(SokuLib::TRAINING_CHARACTER) + "/" +
			std::to_string(currentLatestGen) + "_results.txt"
	};

	for (auto &result : results) {
		auto ai = reinterpret_cast<GeneticAI *>(result.ai);

		stream << ai->getId() << ',' << ai->getGeneration() << std::endl;
	}
}

void loadGeneration(std::vector<std::unique_ptr<GeneticAI>> &ais, unsigned popSize, int latest)
{
	std::string basePath = "GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT) + "/" + std::to_string(SokuLib::TRAINING_CHARACTER) + " vs " + std::to_string(SokuLib::TRAINING_CHARACTER) + "/ais/";
	std::ifstream stream{
		"GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT) + "/" +
			std::to_string(SokuLib::TRAINING_CHARACTER) + " vs " + std::to_string(SokuLib::TRAINING_CHARACTER) + "/" +
			std::to_string(latest) + "_results.txt"
	};

	_mkdir(("GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT)).c_str());
	_mkdir(("GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT) + "/" + std::to_string(SokuLib::TRAINING_CHARACTER) + " vs " + std::to_string(SokuLib::TRAINING_CHARACTER)).c_str());
	_mkdir(("GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT) + "/" + std::to_string(SokuLib::TRAINING_CHARACTER) + " vs " + std::to_string(SokuLib::TRAINING_CHARACTER) + "/ais").c_str());
	ais.reserve(popSize + popSize / LOWERING_FACTOR);
	for (int i = 0; i < popSize; i++) {
		try {
			ais.emplace_back(new GeneticAI(
				latest < 0 ? 0 : latest,
				i,
				NEURON_COUNT,
				GENES_COUNT,
				basePath + std::to_string(latest) + "_" + std::to_string(i) + ".ai"
			));
		} catch (std::exception &e) {
			printf("Cannot load AI: %s. Creating dummy one.\n", e.what());
			ais.emplace_back(new GeneticAI(latest + 1, i, NEURON_COUNT, GENES_COUNT));
			ais.back()->save(basePath + std::to_string(ais.back()->getGeneration()) + "_" + std::to_string(ais.back()->getId()) + ".ai");
		}
	}
	if (latest < 0) {
		for (int i = 0; i < popSize / LOWERING_FACTOR; i++) {
			ais.emplace_back(new GeneticAI(0, i + popSize, NEURON_COUNT, GENES_COUNT));
			ais.back()->save(basePath + std::to_string(ais.back()->getGeneration()) + "_" + std::to_string(ais.back()->getId()) + ".ai");
		}
		return;
	} else if (stream.fail()) {
		printf("Tournament results for generation %i not found\n", latest);
		for (int i = 0; i < popSize / LOWERING_FACTOR; i++) {
			ais.emplace_back(new GeneticAI(0, i, NEURON_COUNT, GENES_COUNT));
			ais.back()->save(basePath + std::to_string(ais.back()->getGeneration()) + "_" + std::to_string(ais.back()->getId()) + ".ai");
		}
		return;
	}

	std::string id;
	std::string gen;

	printf("Loading tournament results for generation %i\n", latest - 1);
	for (
		int i = 0;
		i < popSize / LOWERING_FACTOR &&
		std::getline(stream, id, ',') &&
		std::getline(stream, gen);
		i++
	)
		ais.emplace_back(new GeneticAI(std::stoul(gen), std::stoul(id), NEURON_COUNT, GENES_COUNT, basePath + gen + "_" + id + ".ai"));

}

int getLatestGen(SokuLib::Character lchr, SokuLib::Character rchr)
{
	int gen = -1;
	WIN32_FIND_DATAA data;
	HANDLE handle = FindFirstFileA(("GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT) + "/" + std::to_string(lchr) + " vs " + std::to_string(rchr) + "\\*").c_str(), &data);

	if (handle == INVALID_HANDLE_VALUE)
		return -1;
	do {
		try {
			auto ptr = strchr(data.cFileName, '_');

			if (!ptr)
				continue;
			*ptr = '\0';
			gen = max(gen, std::stoi(data.cFileName));
		} catch (...) {}
	} while (FindNextFileA(handle, &data));
	return gen;
}

std::mt19937 random;

int main(int argc, char *argv[])
{
	if (argc < 8) {
		printf("Usage: %s <game_path> <port_start> <nb_game_instances> <population_size> <input_delay> <middle_layer_size> <gene_count> [<SWRSToys.ini>]\n", argv[0]);
		return EXIT_FAILURE;
	}

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	random.seed(time(nullptr));

	std::string client = argv[1];
	unsigned short port = std::stoul(argv[2]);
	unsigned nb = std::stoul(argv[3]);
	unsigned popSize = std::stoul(argv[4]);
	unsigned inputDelay = std::stoul(argv[5]);
	NEURON_COUNT = std::stoul(argv[6]);
	GENES_COUNT = std::stoul(argv[7]);
	const char *ini = argc == 9 ? argv[8] : nullptr;
	SwissTournamentManager tournament(nb, port, client, inputDelay, 5 * 60 * 60, 1, ini);
	auto latest = getLatestGen(SokuLib::TRAINING_CHARACTER, SokuLib::TRAINING_CHARACTER);
	std::vector<std::unique_ptr<GeneticAI>> ais;

	// first one is a bit special since we either create the AIs
	printf("Loading generation %i\n", latest);
	try {
		loadGeneration(ais, popSize, latest);
	} catch (std::exception &e) {
		puts(e.what());
		return EXIT_FAILURE;
	}
	if (latest < 0)
		latest = 0;
	nb = ceil(log2(ais.size()));
	while (true) {
		std::vector<BaseAI *> baseAis;

		baseAis.reserve(ais.size());
		for (auto &ai : ais)
			baseAis.push_back(&*ai);

#ifndef _DEBUG
		try {
#endif
			auto results = tournament.playTournament(latest, baseAis, nb);

			std::sort(results.begin(), results.end(), [](const PlayerEntry &e1, const PlayerEntry &e2) {
				if (e1.score != e2.score)
					return e1.score > e2.score;
				return e1.wins > e2.wins;
			});
			saveTournamentsResults(results, popSize, latest);
			generateNextGeneration(results, ais, popSize, latest);
			latest++;
#ifndef _DEBUG
		} catch (std::exception &e) {
			std::cerr << e.what() << std::endl;
			throw;
		}
#endif
	}
}
