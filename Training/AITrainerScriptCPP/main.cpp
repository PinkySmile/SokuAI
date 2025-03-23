//
// Created by PinkySmile on 23/05/2021.
//

#define random __random
#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/resource.h>
#include <unistd.h>
#include <sys/stat.h>
#include "winDefines.hpp"
#endif

#include <algorithm>
#include <random>
#include <cstdio>
#include <ctime>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <csignal>
#include "TournamentManager.hpp"
#include "GeneticAI.hpp"
#undef random

using namespace Trainer;

//#define NEURON_COUNT    1000
//#define GENES_COUNT     100000
#define TRAINING_CHARACTER CHARACTER_REMILIA
#define MATING_FACTOR   (100 / 10)
#define LOWERING_FACTOR (100 / 1)
#define GEN_AI_THREADS 15

unsigned NEURON_COUNT;
unsigned GENES_COUNT;

std::mt19937 random;

void generateNextGeneration(const std::vector<PlayerEntry> &results, std::vector<std::unique_ptr<GeneticAI>> &ais, unsigned popSize, unsigned currentLatestGen) {
	std::string basePath = "GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT) + "/" + std::to_string(SokuLib::TRAINING_CHARACTER) + " vs " + std::to_string(SokuLib::TRAINING_CHARACTER) + "/ais/";
	std::vector<std::pair<const GeneticAI *, const GeneticAI *>> pairs;
	std::vector<const GeneticAI *> bestCandidates;
	std::vector<GeneticAI *> result;
	int i;

	mkdir(("GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT) + "/" + std::to_string(SokuLib::TRAINING_CHARACTER) + " vs " + std::to_string(SokuLib::TRAINING_CHARACTER) + "/ais/" + std::to_string(currentLatestGen + 1)).c_str(), 0755);
	for (i = 0; i < results.size() / MATING_FACTOR; i++)
		bestCandidates.push_back(reinterpret_cast<const GeneticAI *>(results[i].ai));

	for (i = 1; i < bestCandidates.size(); i++)
		for (int j = 0; j < bestCandidates.size() - i; j++)
			pairs.emplace_back(bestCandidates[j], bestCandidates[j + i]);

	for (i = 0;;)
		for (auto &parents : pairs) {
			result.push_back(new GeneticAI(currentLatestGen + 1, i, NEURON_COUNT, *parents.first, *parents.second));
			result.back()->save(basePath + std::to_string(currentLatestGen + 1) + "/" + std::to_string(i) + ".ai");
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

	mkdir(("GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT)).c_str(), 0755);
	mkdir(("GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT) + "/" + std::to_string(SokuLib::TRAINING_CHARACTER) + " vs " + std::to_string(SokuLib::TRAINING_CHARACTER)).c_str(), 0755);
	mkdir(("GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT) + "/" + std::to_string(SokuLib::TRAINING_CHARACTER) + " vs " + std::to_string(SokuLib::TRAINING_CHARACTER) + "/ais").c_str(), 0755);
	mkdir(("GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT) + "/" + std::to_string(SokuLib::TRAINING_CHARACTER) + " vs " + std::to_string(SokuLib::TRAINING_CHARACTER) + "/ais/" + std::to_string(latest + 1)).c_str(), 0755);
	ais.resize(popSize + popSize / LOWERING_FACTOR);

	std::vector<std::thread> pool;
	std::mutex mutex;

	pool.reserve(GEN_AI_THREADS);
	for (int i = 0; i < GEN_AI_THREADS; i++) {
		pool.emplace_back([i, popSize, &ais, latest, &basePath, &mutex]{
			unsigned index = popSize / GEN_AI_THREADS * i + (i < popSize % GEN_AI_THREADS ? i : popSize % GEN_AI_THREADS);
			unsigned count = popSize / GEN_AI_THREADS + (i < popSize % GEN_AI_THREADS);

			for (int j = 0; j < count; j++) {
				try {
					ais[index + j].reset(new GeneticAI(
						latest < 0 ? 0 : latest,
						index + j,
						NEURON_COUNT,
						GENES_COUNT,
						basePath + std::to_string(latest + 1) + "/" + std::to_string(index + j) + ".ai"
					));
				} catch (std::exception &e) {
					printf("Cannot load AI: %s. Creating dummy one.\n", e.what());
					ais[index + j].reset(new GeneticAI(latest + 1, index + j, NEURON_COUNT, GENES_COUNT));
					ais[index + j]->save(basePath + std::to_string(ais[index + j]->getGeneration()) + "/" + std::to_string(ais[index + j]->getId()) + ".ai");
				}
			}
		});
	}

	if (latest < 0) {
		for (int i = 0; i < popSize / LOWERING_FACTOR; i++) {
			ais[popSize + i].reset(new GeneticAI(0, i + popSize, NEURON_COUNT, GENES_COUNT));
			ais[popSize + i]->save(basePath + std::to_string(ais[popSize + i]->getGeneration()) + "/" + std::to_string(ais[popSize + i]->getId()) + ".ai");
		}
		for (auto &thread : pool)
			thread.join();
		return;
	} else if (stream.fail()) {
		printf("Tournament results for generation %i not found\n", latest);
		for (int i = 0; i < popSize / LOWERING_FACTOR; i++) {
			ais[popSize + i].reset(new GeneticAI(0, i, NEURON_COUNT, GENES_COUNT));
			ais[popSize + i]->save(basePath + std::to_string(ais[popSize + i]->getGeneration()) + "/" + std::to_string(ais[popSize + i]->getId()) + ".ai");
		}
		for (auto &thread : pool)
			thread.join();
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
		ais[popSize + i] = std::make_unique<GeneticAI>(std::stoul(gen), std::stoul(id), NEURON_COUNT, GENES_COUNT, basePath + gen + "/" + id + ".ai");
	for (auto &thread : pool)
		thread.join();
}

int getLatestGen(SokuLib::Character lchr, SokuLib::Character rchr)
{
	int gen = 0;
	std::error_code err;
	auto it = std::filesystem::directory_iterator("GeneticAI_" + std::to_string(NEURON_COUNT) + "_" + std::to_string(GENES_COUNT) + "/" + std::to_string(lchr) + " vs " + std::to_string(rchr) + "/ais", err);

	if (err)
		return -1;

	for (auto &entry : it) {
		auto str = entry.path().filename().string();

		try {
			gen = max(gen, std::stoi(str));
		} catch (...) {}
	}
	return gen - 1;
}


int main(int argc, char *argv[])
{
	if (argc < 10) {
		printf("Usage: %s <game_path> <port_start> <nb_game_instances> <population_size> <input_delay> <middle_layer_size> <gene_count> <first_to> <pool_size> [<SWRSToys.ini>]\n", argv[0]);
		return EXIT_FAILURE;
	}

#ifdef _WIN32
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#else
	setpriority(PRIO_PROCESS, 0, 10);
#endif
	random.seed(time(nullptr));

	std::string client = argv[1];
	unsigned short port = std::stoul(argv[2]);
	unsigned nb = std::stoul(argv[3]);
	unsigned popSize = std::stoul(argv[4]);
	unsigned inputDelay = std::stoul(argv[5]);
	NEURON_COUNT = std::stoul(argv[6]);
	GENES_COUNT = std::stoul(argv[7]);
	unsigned ft = std::stoul(argv[8]);
	unsigned ps = std::stoul(argv[9]);
	const char *ini = argc == 11 ? argv[10] : nullptr;
	TournamentManager tournament(nb, port, client, inputDelay, 0.5 * 60 * 60, ft, ini);
	auto latest = getLatestGen(SokuLib::TRAINING_CHARACTER, SokuLib::TRAINING_CHARACTER);
	std::vector<std::unique_ptr<GeneticAI>> ais;

	signal(SIGPIPE, SIG_IGN);
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
	else
		latest++;
	nb = ceil(log(ais.size()) / log(4));
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
