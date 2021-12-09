//
// Created by PinkySmile on 26/05/2021.
//

#include <string>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include "GameManager.hpp"
#include "NeuronAi.hpp"

using namespace Trainer;

int main(int argc, char **argv)
{
	for (int i = 1; i < argc; i++) {
		for (int j = 0; argv[i][j]; j++)
			printf("%i %c\n", (unsigned char)argv[i][j], argv[i][j]);
		puts("");
	}
	if (argc < 6) {
		printf("Usage: %s <game_path> <port> <input_delay> <ai_file> [<SWRSToys.ini>]\n", argv[0]);
		system("pause");
		return EXIT_FAILURE;
	}

	srand(time(nullptr));

	std::string client = argv[1];
	unsigned short port = std::stoul(argv[2]);
	unsigned inputDelay = std::stoul(argv[3]);
	std::string aiPath = argv[4];
	const char *ini = argc == 6 ? argv[5] : nullptr;
	GameManager gameManager{client, port, 60, true, 20, 20, ini};
	NeuronAI ai{0, 0, 0};
	BaseAI irrel{SokuLib::CHARACTER_REMILIA, 0};
	GameInstance::StartGameParams params;

	ai.loadFile(SokuLib::CHARACTER_REMILIA, aiPath);
	params.left = irrel.getParams();
	params.right = ai.getParams();
	params.stage = 13;
	params.music = 13;

	gameManager.leftAi = &irrel;
	gameManager.rightAi = &ai;
	try {
		while (true)
			gameManager.runOnce(params, -1, inputDelay);
	} catch (std::exception &e) {
		puts(e.what());
	}
}