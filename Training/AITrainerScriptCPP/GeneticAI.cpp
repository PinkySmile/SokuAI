//
// Created by PinkySmile on 06/12/2021.
//

#include <fstream>
#include <random>
#include "GeneticAI.hpp"
#include "GenNeuron.hpp"

#undef max
#undef min

#define PALETTE_COUNT 74
#define STATE_NEURONS_COUNT (38 + 3 + 14)
#define INPUT_NEURONS_COUNT (STATE_NEURONS_COUNT * 2 + 3 + 2 + 4 + 1 + 1)
#define OBJECTS_OFFSET 0
#define WEATHER_OFFSET 2
#define STATE_OFFSET 5
#define HAND_INDEX 22
#define SKILLS_INDEX 24
#define NEW_NEURON_CHANCE_NUM 1
#define NEW_NEURON_CHANCE_DEN 10

#define DIVISOR_DIRECTION                   1.f
#define DIVISOR_OPPONENT_RELATIVE_POS_X     1240.f
#define DIVISOR_OPPONENT_RELATIVE_POS_Y     1240.f
#define DIVISOR_DIST_TO_LEFT_CORNER         1240.f
#define DIVISOR_DIST_TO_RIGHT_CORNER        1240.f
#define DIVISOR_DIST_TO_GROUND              1240.f
#define DIVISOR_ACTION                      1000.f
#define DIVISOR_ACTION_BLOCK_ID             50.f
#define DIVISOR_ANIMATION_COUNTER           50.f
#define DIVISOR_ANIMATION_SUBFRAME          50.f
#define DIVISOR_FRAME_COUNT                 65535.f
#define DIVISOR_COMBO_DAMAGE                10000.f
#define DIVISOR_COMBO_LIMIT                 200.f
#define DIVISOR_AIRBORNE                    1.f
#define DIVISOR_HP                          10000.f
#define DIVISOR_AIR_DASH_COUNT              3.f
#define DIVISOR_SPIRIT                      10000.f
#define DIVISOR_MAX_SPIRIT                  10000.f
#define DIVISOR_UNTECH                      200.f
#define DIVISOR_HEALING_CHARM               250.f
#define DIVISOR_SWORD_OF_RAPTURE            1200.f
#define DIVISOR_SCORE                       2.f
#define DIVISOR_HAND                        220.f
#define DIVISOR_CARD_GAUGE                  500.f
#define DIVISOR_SKILLS                      4.f
#define DIVISOR_FAN_LEVEL                   4.f
#define DIVISOR_DROP_INVUL_TIME_LEFT        1200.f
#define DIVISOR_SUPER_ARMOR_TIME_LEFT       1500.f
#define DIVISOR_SUPER_ARMOR_HP              3000.f
#define DIVISOR_MILLENIUM_VAMPIRE_TIME_LEFT 600.f
#define DIVISOR_PHILOSOFER_STONE_TIME_LEFT  1200.f
#define DIVISOR_SAKUYAS_WORLD_TIME_LEFT     300.f
#define DIVISOR_PRIVATE_SQUARE_TIME_LEFT    300.f
#define DIVISOR_ORRERIES_TIME_LEFT          600.f
#define DIVISOR_MPP_TIME_LEFT               480.f
#define DIVISOR_KANAKO_COOLDOWN             1500.f
#define DIVISOR_SUWAKO_COOLDOWN             1500.f
#define DIVISOR_OBJECT_COUNT                70.f

extern std::mt19937 random;
extern std::uniform_int_distribution<unsigned short> dist1{0, 0xFFFF};
extern std::uniform_int_distribution<unsigned short> dist2{0, 1};
extern std::uniform_int_distribution<unsigned short> dist3{0, sizeof(*Trainer::Gene::data) * 8};
extern std::uniform_int_distribution<unsigned short> dist4{0, sizeof(Trainer::Gene) / sizeof(*Trainer::Gene::data)};
extern std::uniform_int_distribution<unsigned short> dist5{0, 1000};

namespace Trainer
{
	GeneticAI::GeneticAI(unsigned generation, unsigned id, unsigned middleLayerSize, const GeneticAI &parent1, const GeneticAI &parent2) :
		BaseAI(SokuLib::CHARACTER_REMILIA, 0),
		_generation(generation),
		_id(id)
	{
		printf("Creating child AI from %s and %s\n", parent1.toString().c_str(), parent2.toString().c_str());
		assert(parent1._genome.size() == parent2._genome.size());
		this->_genome.reserve(parent1._genome.size());
		puts("Generating genome...");
		for (int i = 0; i < parent1._genome.size(); i++) {
			this->_genome.push_back((dist2(random) ? parent1 : parent2)._genome[i]);
			if (dist5(random) == 0)
				this->_genome.back().data[dist4(random)] ^= (1 << dist3(random));
		}
		puts("Creating neurons...");
		this->_createNeurons(middleLayerSize);
		puts("Generating links...");
		this->_generateLinks();
		puts("Making palette...");
		this->_palette = this->_calculatePalette();
		puts("Done!");
	}

	GeneticAI::GeneticAI(unsigned generation, unsigned id, unsigned middleLayerSize, unsigned int genCount) :
		BaseAI(SokuLib::CHARACTER_REMILIA, 0),
		_generation(generation),
		_id(id)
	{
		printf("Creating new AI with %i neurons and %i genes\n", middleLayerSize, genCount);
		this->_genome.reserve(genCount);
		for (int i = 0; i < genCount; i++) {
			this->_genome.push_back({});
			this->_genome.back().isInput = dist2(random);
			this->_genome.back().isOutput = dist2(random);
			this->_genome.back().neuronIdIn = dist1(random);
			this->_genome.back().neuronIdOut = dist1(random);
			this->_genome.back().weight = dist1(random);
		}
		printf("%X %X %X %X %X (%i)\n", this->_genome.back().data[0], this->_genome.back().data[1], this->_genome.back().data[2], this->_genome.back().data[3], this->_genome.back().data[4], sizeof(this->_genome.back()));
		printf("%x:%s\n", this->_genome.back().neuronIdIn, this->_genome.back().isInput ? "true" : "false");
		printf("%x:%s\n", this->_genome.back().neuronIdOut, this->_genome.back().isOutput ? "true" : "false");
		printf("%x\n", this->_genome.back().weight);
		puts("Creating neurons...");
		this->_createNeurons(middleLayerSize);
		puts("Generating links...");
		this->_generateLinks();
		puts("Making palette...");
		this->_palette = this->_calculatePalette();
		puts("Done!");
	}

	GeneticAI::GeneticAI(unsigned int generation, unsigned int id, unsigned int middleLayerSize, unsigned int genCount, const std::string &path) :
		BaseAI(SokuLib::CHARACTER_REMILIA, 0),
		_generation(generation),
		_id(id)
	{
		printf("Loading AI from %s\n", path.c_str());

		std::ifstream stream{path, std::ifstream::binary};

		if (stream.fail())
			throw std::invalid_argument(path + ": " + strerror(errno));
		puts("Loading...");
		this->_genome.reserve(genCount);
		for (int i = 0; i < genCount; i++) {
			this->_genome.push_back({});
			this->_genome.back().isInput = dist2(random);
			this->_genome.back().isOutput = dist2(random);
			this->_genome.back().neuronIdIn = dist1(random);
			this->_genome.back().neuronIdOut = dist1(random);
			this->_genome.back().weight = dist1(random);
		}
		stream.read(reinterpret_cast<char *>(this->_genome.data()), this->_genome.size() * sizeof(*this->_genome.data()));
		puts("Creating neurons...");
		this->_createNeurons(middleLayerSize);
		puts("Generating links...");
		this->_generateLinks();
		puts("Making palette...");
		this->_palette = this->_calculatePalette();
		puts("Done!");
	}

	void GeneticAI::save(const std::string &path) const
	{
		std::ofstream stream{path, std::ifstream::binary};

		if (stream.fail())
			throw std::invalid_argument(path + ": " + strerror(errno));
		stream.write(reinterpret_cast<const char *>(this->_genome.data()), this->_genome.size() * sizeof(*this->_genome.data()));
	}

	std::string GeneticAI::toString() const
	{
		return "GeneticAI gen" + std::to_string(this->_generation) + "-" + std::to_string(this->_id);
	}

	void GeneticAI::onWin(unsigned char myScore, unsigned char opponentScore)
	{
		BaseAI::onWin(myScore, opponentScore);
	}

	void GeneticAI::onLose(unsigned char myScore, unsigned char opponentScore)
	{
		BaseAI::onLose(myScore, opponentScore);
	}

	void GeneticAI::onTimeout(unsigned char myScore, unsigned char opponentScore)
	{
		BaseAI::onTimeout(myScore, opponentScore);
	}

	void GeneticAI::onGameStart(SokuLib::Character myChr, SokuLib::Character opponentChr, unsigned int inputDelay)
	{
		BaseAI::onGameStart(myChr, opponentChr, inputDelay);
	}

	GameInstance::PlayerParams GeneticAI::getParams() const
	{
		GameInstance::PlayerParams params{
			this->_character,
			this->_palette
		};

		buildDeck(this->_character, params.deck);
		strcpy_s(params.name, this->toString().c_str());
		return params;
	}

	const char *GeneticAI::getAction(const GameInstance::GameFrame &frame, bool isLeft, unsigned int frameId)
	{
		if (this->_noWire)
			return BaseAI::getAction(frame, isLeft, frameId);

		auto &myFrame = isLeft ? frame.left : frame.right;
		auto &opFrame = !isLeft ? frame.left : frame.right;

		this->_myObjects->objects = isLeft ? frame.leftObjects : frame.rightObjects;
		this->_opObjects->objects = isLeft ? frame.rightObjects : frame.leftObjects;

		// Weather
		this->_inNeurons[WEATHER_OFFSET + 0]->setValue(frame.displayedWeather <= 19 ? frame.displayedWeather / 19.f : -1);
		this->_inNeurons[WEATHER_OFFSET + 1]->setValue(frame.activeWeather <= 19 ? frame.activeWeather / 19.f : -1);
		this->_inNeurons[WEATHER_OFFSET + 2]->setValue(frame.weatherTimer / 999.f);

		// God, please forgive me for I have sinned
		this->_inNeurons[STATE_OFFSET + 0x00 + 0x00]->setValue(myFrame.direction                / DIVISOR_DIRECTION);
		this->_inNeurons[STATE_OFFSET + 0x01 + 0x00]->setValue(myFrame.opponentRelativePos.x    / DIVISOR_OPPONENT_RELATIVE_POS_X);
		this->_inNeurons[STATE_OFFSET + 0x02 + 0x00]->setValue(myFrame.opponentRelativePos.y    / DIVISOR_OPPONENT_RELATIVE_POS_Y);
		this->_inNeurons[STATE_OFFSET + 0x03 + 0x00]->setValue(myFrame.distToBackCorner         / DIVISOR_DIST_TO_LEFT_CORNER);
		this->_inNeurons[STATE_OFFSET + 0x04 + 0x00]->setValue(myFrame.distToFrontCorner        / DIVISOR_DIST_TO_RIGHT_CORNER);
		this->_inNeurons[STATE_OFFSET + 0x05 + 0x00]->setValue(myFrame.distToGround             / DIVISOR_DIST_TO_GROUND);
		this->_inNeurons[STATE_OFFSET + 0x06 + 0x00]->setValue(myFrame.action                   / DIVISOR_ACTION);
		this->_inNeurons[STATE_OFFSET + 0x07 + 0x00]->setValue(myFrame.actionBlockId            / DIVISOR_ACTION_BLOCK_ID);
		this->_inNeurons[STATE_OFFSET + 0x08 + 0x00]->setValue(myFrame.animationCounter         / DIVISOR_ANIMATION_COUNTER);
		this->_inNeurons[STATE_OFFSET + 0x09 + 0x00]->setValue(myFrame.animationSubFrame        / DIVISOR_ANIMATION_SUBFRAME);
		this->_inNeurons[STATE_OFFSET + 0x0A + 0x00]->setValue(myFrame.frameCount               / DIVISOR_FRAME_COUNT);
		this->_inNeurons[STATE_OFFSET + 0x0B + 0x00]->setValue(myFrame.comboDamage              / DIVISOR_COMBO_DAMAGE);
		this->_inNeurons[STATE_OFFSET + 0x0C + 0x00]->setValue(myFrame.comboLimit               / DIVISOR_COMBO_LIMIT);
		this->_inNeurons[STATE_OFFSET + 0x0D + 0x00]->setValue(myFrame.airBorne                 / DIVISOR_AIRBORNE);
		this->_inNeurons[STATE_OFFSET + 0x0E + 0x00]->setValue(myFrame.hp                       / DIVISOR_HP);
		this->_inNeurons[STATE_OFFSET + 0x0F + 0x00]->setValue(myFrame.airDashCount             / DIVISOR_AIR_DASH_COUNT);
		this->_inNeurons[STATE_OFFSET + 0x10 + 0x00]->setValue(myFrame.spirit                   / DIVISOR_SPIRIT);
		this->_inNeurons[STATE_OFFSET + 0x11 + 0x00]->setValue(myFrame.maxSpirit                / DIVISOR_MAX_SPIRIT);
		this->_inNeurons[STATE_OFFSET + 0x12 + 0x00]->setValue(myFrame.untech                   / DIVISOR_UNTECH);
		this->_inNeurons[STATE_OFFSET + 0x13 + 0x00]->setValue(myFrame.healingCharm             / DIVISOR_HEALING_CHARM);
		this->_inNeurons[STATE_OFFSET + 0x14 + 0x00]->setValue(myFrame.swordOfRapture           / DIVISOR_SWORD_OF_RAPTURE);
		this->_inNeurons[STATE_OFFSET + 0x15 + 0x00]->setValue(myFrame.score                    / DIVISOR_SCORE);
		for (int i = 0; i < 5; i++) {
			auto card = myFrame.hand[i];

			this->_inNeurons[STATE_OFFSET + 0x16 + 0x00 + i]->setValue(card == 0xFF ? -1 : (card / DIVISOR_HAND));
		}
		this->_inNeurons[STATE_OFFSET + 0x1B + 0x00]->setValue(myFrame.cardGauge                / DIVISOR_CARD_GAUGE);
		for (int i = 0; i < 15; i++) {
			auto skill = myFrame.skills[i];

			this->_inNeurons[STATE_OFFSET + 0x1C + 0x00 + i]->setValue(skill.notUsed ? -1 : (skill.level / DIVISOR_SKILLS));
		}
		this->_inNeurons[STATE_OFFSET + 0x2B + 0x00]->setValue(myFrame.fanLevel                 / DIVISOR_FAN_LEVEL);
		this->_inNeurons[STATE_OFFSET + 0x2C + 0x00]->setValue(myFrame.dropInvulTimeLeft        / DIVISOR_DROP_INVUL_TIME_LEFT);

		auto t = myFrame.superArmorTimeLeft;
		auto h = myFrame.superArmorHp;

		this->_inNeurons[STATE_OFFSET + 0x2D + 0x00]->setValue(t < 0 ? t : t / DIVISOR_SUPER_ARMOR_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x2E + 0x00]->setValue(h < 0 ? h : h / DIVISOR_SUPER_ARMOR_HP);
		this->_inNeurons[STATE_OFFSET + 0x2F + 0x00]->setValue(myFrame.milleniumVampireTimeLeft / DIVISOR_MILLENIUM_VAMPIRE_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x30 + 0x00]->setValue(myFrame.philosoferStoneTimeLeft  / DIVISOR_PHILOSOFER_STONE_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x31 + 0x00]->setValue(myFrame.sakuyasWorldTimeLeft     / DIVISOR_SAKUYAS_WORLD_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x32 + 0x00]->setValue(myFrame.privateSquareTimeLeft    / DIVISOR_PRIVATE_SQUARE_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x33 + 0x00]->setValue(myFrame.orreriesTimeLeft         / DIVISOR_ORRERIES_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x34 + 0x00]->setValue(myFrame.mppTimeLeft              / DIVISOR_MPP_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x35 + 0x00]->setValue(myFrame.kanakoCooldown           / DIVISOR_KANAKO_COOLDOWN);
		this->_inNeurons[STATE_OFFSET + 0x36 + 0x00]->setValue(myFrame.suwakoCooldown           / DIVISOR_SUWAKO_COOLDOWN);



		this->_inNeurons[STATE_OFFSET + 0x00 + 0x37]->setValue(opFrame.direction                / DIVISOR_DIRECTION);
		this->_inNeurons[STATE_OFFSET + 0x01 + 0x37]->setValue(opFrame.opponentRelativePos.x    / DIVISOR_OPPONENT_RELATIVE_POS_X);
		this->_inNeurons[STATE_OFFSET + 0x02 + 0x37]->setValue(opFrame.opponentRelativePos.y    / DIVISOR_OPPONENT_RELATIVE_POS_Y);
		this->_inNeurons[STATE_OFFSET + 0x03 + 0x37]->setValue(opFrame.distToBackCorner         / DIVISOR_DIST_TO_LEFT_CORNER);
		this->_inNeurons[STATE_OFFSET + 0x04 + 0x37]->setValue(opFrame.distToFrontCorner        / DIVISOR_DIST_TO_RIGHT_CORNER);
		this->_inNeurons[STATE_OFFSET + 0x05 + 0x37]->setValue(opFrame.distToGround             / DIVISOR_DIST_TO_GROUND);
		this->_inNeurons[STATE_OFFSET + 0x06 + 0x37]->setValue(opFrame.action                   / DIVISOR_ACTION);
		this->_inNeurons[STATE_OFFSET + 0x07 + 0x37]->setValue(opFrame.actionBlockId            / DIVISOR_ACTION_BLOCK_ID);
		this->_inNeurons[STATE_OFFSET + 0x08 + 0x37]->setValue(opFrame.animationCounter         / DIVISOR_ANIMATION_COUNTER);
		this->_inNeurons[STATE_OFFSET + 0x09 + 0x37]->setValue(opFrame.animationSubFrame        / DIVISOR_ANIMATION_SUBFRAME);
		this->_inNeurons[STATE_OFFSET + 0x0A + 0x37]->setValue(opFrame.frameCount               / DIVISOR_FRAME_COUNT);
		this->_inNeurons[STATE_OFFSET + 0x0B + 0x37]->setValue(opFrame.comboDamage              / DIVISOR_COMBO_DAMAGE);
		this->_inNeurons[STATE_OFFSET + 0x0C + 0x37]->setValue(opFrame.comboLimit               / DIVISOR_COMBO_LIMIT);
		this->_inNeurons[STATE_OFFSET + 0x0D + 0x37]->setValue(opFrame.airBorne                 / DIVISOR_AIRBORNE);
		this->_inNeurons[STATE_OFFSET + 0x0E + 0x37]->setValue(opFrame.hp                       / DIVISOR_HP);
		this->_inNeurons[STATE_OFFSET + 0x0F + 0x37]->setValue(opFrame.airDashCount             / DIVISOR_AIR_DASH_COUNT);
		this->_inNeurons[STATE_OFFSET + 0x10 + 0x37]->setValue(opFrame.spirit                   / DIVISOR_SPIRIT);
		this->_inNeurons[STATE_OFFSET + 0x11 + 0x37]->setValue(opFrame.maxSpirit                / DIVISOR_MAX_SPIRIT);
		this->_inNeurons[STATE_OFFSET + 0x12 + 0x37]->setValue(opFrame.untech                   / DIVISOR_UNTECH);
		this->_inNeurons[STATE_OFFSET + 0x13 + 0x37]->setValue(opFrame.healingCharm             / DIVISOR_HEALING_CHARM);
		this->_inNeurons[STATE_OFFSET + 0x14 + 0x37]->setValue(opFrame.swordOfRapture           / DIVISOR_SWORD_OF_RAPTURE);
		this->_inNeurons[STATE_OFFSET + 0x15 + 0x37]->setValue(opFrame.score                    / DIVISOR_SCORE);
		for (int i = 0; i < 5; i++) {
			auto card = opFrame.hand[i];

			this->_inNeurons[STATE_OFFSET + 0x16 + 0x37 + i]->setValue(card == 0xFF ? -1 : (card / DIVISOR_HAND));
		}
		this->_inNeurons[STATE_OFFSET + 0x1B + 0x37]->setValue(opFrame.cardGauge                / DIVISOR_CARD_GAUGE);
		for (int i = 0; i < 15; i++) {
			auto skill = opFrame.skills[i];

			this->_inNeurons[STATE_OFFSET + 0x1C + 0x37 + i]->setValue(skill.notUsed ? -1 : (skill.level / DIVISOR_SKILLS));
		}
		this->_inNeurons[STATE_OFFSET + 0x2B + 0x37]->setValue(opFrame.fanLevel                 / DIVISOR_FAN_LEVEL);
		this->_inNeurons[STATE_OFFSET + 0x2C + 0x37]->setValue(opFrame.dropInvulTimeLeft        / DIVISOR_DROP_INVUL_TIME_LEFT);

		t = opFrame.superArmorTimeLeft;
		h = opFrame.superArmorHp;

		this->_inNeurons[STATE_OFFSET + 0x2D + 0x37]->setValue(t < 0 ? t : t / DIVISOR_SUPER_ARMOR_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x2E + 0x37]->setValue(h < 0 ? h : h / DIVISOR_SUPER_ARMOR_HP);
		this->_inNeurons[STATE_OFFSET + 0x2F + 0x37]->setValue(opFrame.milleniumVampireTimeLeft / DIVISOR_MILLENIUM_VAMPIRE_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x30 + 0x37]->setValue(opFrame.philosoferStoneTimeLeft  / DIVISOR_PHILOSOFER_STONE_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x31 + 0x37]->setValue(opFrame.sakuyasWorldTimeLeft     / DIVISOR_SAKUYAS_WORLD_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x32 + 0x37]->setValue(opFrame.privateSquareTimeLeft    / DIVISOR_PRIVATE_SQUARE_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x33 + 0x37]->setValue(opFrame.orreriesTimeLeft         / DIVISOR_ORRERIES_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x34 + 0x37]->setValue(opFrame.mppTimeLeft              / DIVISOR_MPP_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x35 + 0x37]->setValue(opFrame.kanakoCooldown           / DIVISOR_KANAKO_COOLDOWN);
		this->_inNeurons[STATE_OFFSET + 0x36 + 0x37]->setValue(opFrame.suwakoCooldown           / DIVISOR_SUWAKO_COOLDOWN);

		this->_inNeurons[STATE_OFFSET + 0x6E]->setValue((random() - random.min()) * 2.f / (random.max() - random.min()) - 1);
		this->_inNeurons[STATE_OFFSET + 0x6F]->setValue((random() - random.min()) * 2.f / (random.max() - random.min()) - 1);
		this->_inNeurons[STATE_OFFSET + 0x70]->setValue((random() - random.min()) * 2.f / (random.max() - random.min()) - 1);
		this->_inNeurons[STATE_OFFSET + 0x71]->setValue((random() - random.min()) * 2.f / (random.max() - random.min()) - 1);
		this->_inNeurons[STATE_OFFSET + 0x71]->setValue(frameId / 65535.f);
		this->_inNeurons[STATE_OFFSET + 0x70]->setValue(1);

		for (auto &neuron : this->_neurons)
			reinterpret_cast<GenNeuron &>(*neuron).startComputed();
		for (auto &neuron : this->_outNeurons)
			reinterpret_cast<GenNeuron &>(*neuron).startComputed();

		try {
			float sum = 0;
			int neur = 0;
			unsigned whitelist[]{
				 0, //nothing
				 1, //forward
				 2, //backward
				// 3, //block_low
				// 4, //crouch
				// 5, //forward_jump
				// 6, //backward_jump
				// 7, //neutral_jump
				// 8, //forward_highjump
				// 9, //backward_highjump
				//10, //neutral_highjump
				//11, //forward_dash
				//12, //backward_dash
				//13, //fly1
				//14, //fly2
				//15, //fly3
				//16, //fly4
				//17, //fly6
				//18, //fly7
				//19, //fly8
				//20, //fly9
				//21, //switch_to_card_1
				//22, //switch_to_card_2
				//23, //switch_to_card_3
				//24, //switch_to_card_4
				//25, //use_current_card
				//26, //upgrade_skillcard
				//27, //236b
				//28, //236c
				//29, //623b
				//30, //623c
				//31, //214b
				//32, //214c
				//33, //421b
				//34, //421c
				//35, //22b
				//36, //22c
				//37, //be1
				//38, //be2
				//39, //be4
				//40, //be6
				//41, //2b
				//42, //3b
				//43, //4b
				//44, //5b
				//45, //6b
				//46, //66b
				//47, //1c
				//48, //2c
				//49, //3c
				//50, //4c
				//51, //5c
				//52, //6c
				//53, //66c
				//54, //1a
				//55, //2a
				//56, //3a
				//57, //5a
				//58, //6a
				//59, //8a
				//60, //66a
				61, //4a
			};

			for (unsigned i : whitelist)
				if (this->_outNeurons[i]->getValue() > 0)
					sum += this->_outNeurons[i]->getValue();

			std::uniform_real_distribution<float> dist(0, sum);

			if (sum == 0)
				return "nothing";//BaseAI::getAction(frame, isLeft, frameId);
			sum = dist(random);
			for (unsigned i : whitelist) {
				if (this->_outNeurons[i]->getValue() <= 0)
					continue;
				if (sum <= this->_outNeurons[i]->getValue())
					return BaseAI::actions[i];
				sum -= this->_outNeurons[i]->getValue();
				neur = i;
			}
			return BaseAI::actions[neur];
			//return {
			//	this->_outNeurons[0]->getValue() >= 0.5,
			//	this->_outNeurons[1]->getValue() >= 0.5,
			//	this->_outNeurons[2]->getValue() >= 0.5,
			//	this->_outNeurons[3]->getValue() >= 0.5,
			//	this->_outNeurons[4]->getValue() >= 0.5,
			//	this->_outNeurons[5]->getValue() >= 0.5,
			//	static_cast<char>((this->_outNeurons[6]->getValue() >= 0.5) - (this->_outNeurons[6]->getValue() <= -0.5)),
			//	static_cast<char>((this->_outNeurons[7]->getValue() >= 0.5) - (this->_outNeurons[7]->getValue() <= -0.5)),
			//};
		} catch (std::exception &e) {
			puts(e.what());
			throw;
		}
	}

	unsigned int GeneticAI::getId() const
	{
		return this->_id;
	}

	int GeneticAI::getGeneration() const
	{
		return this->_generation;
	}

	void GeneticAI::_createNeurons(unsigned middleLayerSize)
	{
		this->_inNeurons.clear();
		this->_inNeurons.reserve(STATE_NEURONS_COUNT);
		this->_myObjects = new ObjectsNeuron(0);
		this->_opObjects = new ObjectsNeuron(1);
		this->_inNeurons.emplace_back(this->_myObjects);
		this->_inNeurons.emplace_back(this->_opObjects);
		for (int i = 2; i < INPUT_NEURONS_COUNT; i++)
			this->_inNeurons.emplace_back(new Neuron(i));
		this->_neurons.reserve(middleLayerSize);
		for (int i = 0; i < middleLayerSize; i++)
			this->_neurons.emplace_back(new GenNeuron());
		this->_outNeurons.reserve(BaseAI::actions.size());
		for (int i = 0; i < BaseAI::actions.size(); i++)
			this->_outNeurons.emplace_back(new GenNeuron());
	}

	void GeneticAI::_generateLinks()
	{
		for (auto &gene : this->_genome) {
			gene.neuronIdIn  &= 0x1FF;
			gene.neuronIdOut &= 0x1FF;
			//printf("%s%i -> %s%i (", gene.isInput ? "I" : "N", gene.neuronIdIn, gene.isOutput ? "O" : "N", gene.neuronIdOut);
			//fflush(stdout);

			auto *neurOut = reinterpret_cast<GenNeuron *>(this->getOutNeuron(gene));
			auto *neurIn = this->getInNeuron(gene);

			//printf("%p -> %p)\n", neurOut, neurIn);
			if (!neurOut || !neurIn) {
				//puts("Bad");
				continue;
			}
			this->_noWire &= !gene.isOutput;
			//printf("Adding link (%s)\n", this->_noWire ? "true" : "false");
			neurOut->addLink(
				gene.weight > 0 ? (static_cast<float>(gene.weight) / INT16_MAX) : (static_cast<float>(gene.weight) / INT16_MIN),
				*neurIn
			);
			//puts("Loop");
		}
		if (this->_noWire)
			printf("%s: No output neuron connected.\n", this->toString().c_str());
		puts("Done");
	}

	int GeneticAI::_calculatePalette()
	{
		unsigned sum = 0;

		for (auto &gene : this->_genome)
			for (auto data : gene.data)
				sum += data;
		return sum % PALETTE_COUNT;
	}

	Neuron *GeneticAI::getInNeuron(const Gene &gene) const
	{
		auto id = gene.neuronIdIn & 0x1FF;
		auto &arr = (gene.isInput ? this->_inNeurons : this->_neurons);

		if (id >= arr.size())
			return nullptr;
		return &*arr[id];
	}

	Neuron *GeneticAI::getOutNeuron(const Gene &gene)
	{
		auto id = gene.neuronIdOut & 0x1FF;
		auto &arr = (gene.isOutput ? this->_outNeurons : this->_neurons);

		if (id >= arr.size())
			return nullptr;
		return &*arr[id];
	}

}