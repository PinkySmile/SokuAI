//
// Created by PinkySmile on 22/05/2021.
//

#include <cstdlib>
#include <direct.h>
#include <fstream>
#include "Exceptions.hpp"
#include "NeuronAi.hpp"

#define STATE_NEURONS_COUNT (38 + 3 + 14)
#define INPUT_NEURONS_COUNT (STATE_NEURONS_COUNT * 2 + 3 + 2)
#define OBJECTS_OFFSET 0
#define WEATHER_OFFSET 3
#define STATE_OFFSET 5
#define HAND_INDEX 22
#define SKILLS_INDEX 24
#define NEW_NEURON_CHANCE_NUM 1
#define NEW_NEURON_CHANCE_DEN 100

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

namespace Trainer
{
	static const char *chrNames[] = {
		"Reimu",
		"Marisa",
		"Sakuya",
		"Alice",
		"Patchouli",
		"Youmu",
		"Remilia",
		"Yuyuko",
		"Yukari",
		"Suika",
		"Reisen",
		"Aya",
		"Komachi",
		"Iku",
		"Tenshi",
		"Sanae",
		"Cirno",
		"Meiling",
		"Utsuho",
		"Suwako"
	};

	const std::string NeuronAI::_path = std::string(__argv[0]).substr(0, std::string(__argv[0]).find_last_of('\\')) + "\\generated\\";

	void NeuronAI::_createBaseNeurons()
	{
		this->_neurons.clear();
		this->_neurons.reserve(STATE_NEURONS_COUNT);
		this->myObjects = new ObjectsNeuron(0);
		this->opObjects = new ObjectsNeuron(1);
		this->_neurons.emplace_back(this->myObjects);
		this->_neurons.emplace_back(this->opObjects);
		for (int i = 2; i < INPUT_NEURONS_COUNT; i++)
			this->_neurons.emplace_back(new Neuron(i));
	}

	NeuronAI::NeuronAI(unsigned char palette, unsigned index, int generation) :
		BaseAI(SokuLib::CHARACTER_RANDOM, palette),
		_generation(generation),
		_id(index)
	{
	}

	int NeuronAI::getLatestGen(SokuLib::Character myChar, SokuLib::Character opChar)
	{
		int gen = 0;
		WIN32_FIND_DATAA data;
		HANDLE handle = FindFirstFileA((NeuronAI::_path + chrNames[myChar] + " vs " + chrNames[opChar] + "\\*").c_str(), &data);

		if (handle == INVALID_HANDLE_VALUE)
			return -1;
		do {
			try {
				gen = max(gen, std::stoul(data.cFileName));
			} catch (...) {}
		} while (FindNextFileA(handle, &data));
		return gen;
	}

	void NeuronAI::createRequiredPath(SokuLib::Character myChar, SokuLib::Character opChar)
	{
		_mkdir(NeuronAI::_path.c_str());
		_mkdir((NeuronAI::_path + chrNames[myChar] + " vs " + chrNames[opChar]).c_str());
		if (this->_generation <= 0)
			_mkdir((NeuronAI::_path + chrNames[myChar] + " vs " + chrNames[opChar] + "\\0").c_str());
		else
			_mkdir((NeuronAI::_path + chrNames[myChar] + " vs " + chrNames[opChar] + "\\" + std::to_string(this->_generation)).c_str());
	}

	void NeuronAI::init(SokuLib::Character myChar, SokuLib::Character opChar)
	{
		this->_character = myChar;
		if (this->_generation < 0) {
			auto output = new Neuron(INPUT_NEURONS_COUNT);

			puts("Making AI from scratch");
			this->_generation = 0;
			this->_createBaseNeurons();
			output->setValue(2.f * rand() / RAND_MAX - 1);
			for (auto &neuron :this->_neurons)
				output->link(&*neuron, 2.f * rand() / RAND_MAX - 1);
			this->_neurons.emplace_back(output);
			this->save(myChar, opChar);
			return;
		}
		this->loadFile(myChar, opChar);
	}

	void NeuronAI::save(SokuLib::Character myChar, SokuLib::Character opChar) const
	{
		std::string file =
			NeuronAI::_path + chrNames[myChar] + " vs " + chrNames[opChar] + "/" +
			std::to_string(this->_generation) + "/" +
			std::to_string(this->_id) + ".neur";
		std::ofstream stream{file};

		printf("Saving to %s\n", file.c_str());
		stream << this->_palette << std::endl;
		this->_neurons[OBJECTS_OFFSET]->save(stream);
		this->_neurons[OBJECTS_OFFSET + 1]->save(stream);
		for (int i = INPUT_NEURONS_COUNT; i < this->_neurons.size(); i++)
			this->_neurons[i]->save(stream);
	}

	void NeuronAI::onGameStart(SokuLib::Character myChar, SokuLib::Character opChar, unsigned int inputDelay)
	{
		BaseAI::onGameStart(myChar, opChar, inputDelay);
		this->_inputDelay = inputDelay;
	}

	void NeuronAI::loadFile(SokuLib::Character myChar, SokuLib::Character opChar)
	{
		std::string file =
			NeuronAI::_path + chrNames[myChar] + " vs " + chrNames[opChar] + "/" +
			std::to_string(this->_generation) + "/" +
			std::to_string(this->_id) + ".neur";
		std::ifstream stream{file};
		std::string line;

		this->_createBaseNeurons();
		printf("Loading %s\n", file.c_str());
		std::getline(stream, line);
		this->_palette = line[0];
		std::getline(stream, line);
		this->_neurons[OBJECTS_OFFSET]->loadFromLine(line);
		std::getline(stream, line);
		this->_neurons[OBJECTS_OFFSET + 1]->loadFromLine(line);
		while (std::getline(stream, line))
			if (!line.empty()) {
				auto neuron = new Neuron(this->_neurons.size());

				neuron->loadFromLine(line);
				this->_neurons.emplace_back(neuron);
			}
		for (auto &neuron : this->_neurons)
			neuron->resolveLinks(this->_neurons);
		std::sort(this->_neurons.begin(), this->_neurons.end(), [](const std::unique_ptr<Neuron> &n1, const std::unique_ptr<Neuron> &n2){
			return n1->getId() < n2->getId();
		});
		for (int i = 0; i < this->_neurons.size(); i++)
			if (i != this->_neurons[i]->getId())
				throw Exception("Invalid file. Neuron #" + std::to_string(i) + " have id " + std::to_string(this->_neurons[i]->getId()));
	}

	NeuronAI *NeuronAI::mateOnce(const NeuronAI &other, unsigned int id, SokuLib::Character myChar, SokuLib::Character opChar) const
	{
		auto child = new NeuronAI((rand() & 1) == 0 ? this->_palette : other._palette, id, this->_generation + 1);
		auto objs1 = new ObjectsNeuron(*reinterpret_cast<ObjectsNeuron *>(&*((rand() & 1) == 0 ? this->_neurons[0] : other._neurons[0])));
		auto objs2 = new ObjectsNeuron(*reinterpret_cast<ObjectsNeuron *>(&*((rand() & 1) == 0 ? this->_neurons[1] : other._neurons[1])));

		child->_createBaseNeurons();
		child->_neurons[OBJECTS_OFFSET].reset(objs1);
		child->_neurons[OBJECTS_OFFSET + 1].reset(objs2);

		// Copy either parent1's or parent2's neuron
		for (int i = INPUT_NEURONS_COUNT; i < this->_neurons.size() && i < other._neurons.size(); i++)
			if ((rand() & 1) == 0 && i < this->_neurons.size() || i >= other._neurons.size())
				child->_neurons.emplace_back(new Neuron(*this->_neurons[i]));
			else
				child->_neurons.emplace_back(new Neuron(*other._neurons[i]));

		for (auto &neuron : child->_neurons)
			neuron->resolveLinks(child->_neurons);

		// Mutate neurons
		for (auto &neuron : child->_neurons)
			neuron->mutate(child->_neurons);

		// Add some neurons
		while (rand() % NEW_NEURON_CHANCE_DEN < NEW_NEURON_CHANCE_NUM) {
			auto neur = new Neuron(child->_neurons.size());
			auto i = rand() % child->_neurons.size();

			while (i == INPUT_NEURONS_COUNT)
				i = rand() % child->_neurons.size();

			neur->link(&*child->_neurons[i], 2.f * rand() / RAND_MAX);
			for (auto &neuron : child->_neurons) {
				auto deps = neuron->getDependencies();

				if (neuron->getId() != INPUT_NEURONS_COUNT && std::find(deps.begin(), deps.end(), neur->getId()) == deps.end() && rand() % 10 == 0) {
					neur->link(&*neuron, 2.f * rand() / RAND_MAX);
					continue;
				}

				deps = neur->getDependencies();
				if (neuron->getId() >= INPUT_NEURONS_COUNT && std::find(deps.begin(), deps.end(), neuron->getId()) == deps.end() && rand() % 10 == 0)
					neuron->link(neur, 2.f * rand() / RAND_MAX);
			}
			neur->setValue(2.f * rand() / RAND_MAX);
			child->_neurons.emplace_back(neur);
		}

		// And finally we save all this mess
		child->createRequiredPath(myChar, opChar);
		child->save(myChar, opChar);
		return child;
	}

	std::vector<NeuronAI *> NeuronAI::mate(const NeuronAI &other, SokuLib::Character myChar, SokuLib::Character opChar, unsigned int startId, unsigned int nb) const
	{
		std::vector<NeuronAI *> result;

		while (nb--) {
			result.push_back(this->mateOnce(other, startId, myChar, opChar));
			startId++;
		}
		return result;
	}

	const char *NeuronAI::getAction(const GameInstance::GameFrame &frame, bool isLeft)
	{
		this->myObjects->objects = isLeft ? frame.leftObjects : frame.rightObjects;
		this->opObjects->objects = isLeft ? frame.rightObjects : frame.leftObjects;

		// Weather
		this->_neurons[WEATHER_OFFSET + 0]->setValue(1.f * frame.displayedWeather / SokuLib::WEATHER_AURORA);
		this->_neurons[WEATHER_OFFSET + 1]->setValue(1.f * frame.activeWeather / SokuLib::WEATHER_AURORA);
		this->_neurons[WEATHER_OFFSET + 2]->setValue(frame.weatherTimer / 999.f);

		// God please forgive me for I have sinned
		this->_neurons[STATE_OFFSET + 0x00 + 0x00]->setValue((isLeft ? frame.left : frame.right).direction                / DIVISOR_DIRECTION);
		this->_neurons[STATE_OFFSET + 0x01 + 0x00]->setValue((isLeft ? frame.left : frame.right).opponentRelativePos.x    / DIVISOR_OPPONENT_RELATIVE_POS_X);
		this->_neurons[STATE_OFFSET + 0x02 + 0x00]->setValue((isLeft ? frame.left : frame.right).opponentRelativePos.y    / DIVISOR_OPPONENT_RELATIVE_POS_Y);
		this->_neurons[STATE_OFFSET + 0x03 + 0x00]->setValue((isLeft ? frame.left : frame.right).distToBackCorner         / DIVISOR_DIST_TO_LEFT_CORNER);
		this->_neurons[STATE_OFFSET + 0x04 + 0x00]->setValue((isLeft ? frame.left : frame.right).distToFrontCorner        / DIVISOR_DIST_TO_RIGHT_CORNER);
		this->_neurons[STATE_OFFSET + 0x05 + 0x00]->setValue((isLeft ? frame.left : frame.right).distToGround             / DIVISOR_DIST_TO_GROUND);
		this->_neurons[STATE_OFFSET + 0x06 + 0x00]->setValue((isLeft ? frame.left : frame.right).action                   / DIVISOR_ACTION);
		this->_neurons[STATE_OFFSET + 0x07 + 0x00]->setValue((isLeft ? frame.left : frame.right).actionBlockId            / DIVISOR_ACTION_BLOCK_ID);
		this->_neurons[STATE_OFFSET + 0x08 + 0x00]->setValue((isLeft ? frame.left : frame.right).animationCounter         / DIVISOR_ANIMATION_COUNTER);
		this->_neurons[STATE_OFFSET + 0x09 + 0x00]->setValue((isLeft ? frame.left : frame.right).animationSubFrame        / DIVISOR_ANIMATION_SUBFRAME);
		this->_neurons[STATE_OFFSET + 0x0A + 0x00]->setValue((isLeft ? frame.left : frame.right).frameCount               / DIVISOR_FRAME_COUNT);
		this->_neurons[STATE_OFFSET + 0x0B + 0x00]->setValue((isLeft ? frame.left : frame.right).comboDamage              / DIVISOR_COMBO_DAMAGE);
		this->_neurons[STATE_OFFSET + 0x0C + 0x00]->setValue((isLeft ? frame.left : frame.right).comboLimit               / DIVISOR_COMBO_LIMIT);
		this->_neurons[STATE_OFFSET + 0x0D + 0x00]->setValue((isLeft ? frame.left : frame.right).airBorne                 / DIVISOR_AIRBORNE);
		this->_neurons[STATE_OFFSET + 0x0E + 0x00]->setValue((isLeft ? frame.left : frame.right).hp                       / DIVISOR_HP);
		this->_neurons[STATE_OFFSET + 0x0F + 0x00]->setValue((isLeft ? frame.left : frame.right).airDashCount             / DIVISOR_AIR_DASH_COUNT);
		this->_neurons[STATE_OFFSET + 0x10 + 0x00]->setValue((isLeft ? frame.left : frame.right).spirit                   / DIVISOR_SPIRIT);
		this->_neurons[STATE_OFFSET + 0x11 + 0x00]->setValue((isLeft ? frame.left : frame.right).maxSpirit                / DIVISOR_MAX_SPIRIT);
		this->_neurons[STATE_OFFSET + 0x12 + 0x00]->setValue((isLeft ? frame.left : frame.right).untech                   / DIVISOR_UNTECH);
		this->_neurons[STATE_OFFSET + 0x13 + 0x00]->setValue((isLeft ? frame.left : frame.right).healingCharm             / DIVISOR_HEALING_CHARM);
		this->_neurons[STATE_OFFSET + 0x14 + 0x00]->setValue((isLeft ? frame.left : frame.right).swordOfRapture           / DIVISOR_SWORD_OF_RAPTURE);
		this->_neurons[STATE_OFFSET + 0x15 + 0x00]->setValue((isLeft ? frame.left : frame.right).score                    / DIVISOR_SCORE);
		this->_neurons[STATE_OFFSET + 0x16 + 0x00]->setValue((isLeft ? frame.left : frame.right).hand[0]                  / DIVISOR_HAND);
		this->_neurons[STATE_OFFSET + 0x17 + 0x00]->setValue((isLeft ? frame.left : frame.right).hand[1]                  / DIVISOR_HAND);
		this->_neurons[STATE_OFFSET + 0x18 + 0x00]->setValue((isLeft ? frame.left : frame.right).hand[2]                  / DIVISOR_HAND);
		this->_neurons[STATE_OFFSET + 0x19 + 0x00]->setValue((isLeft ? frame.left : frame.right).hand[3]                  / DIVISOR_HAND);
		this->_neurons[STATE_OFFSET + 0x1A + 0x00]->setValue((isLeft ? frame.left : frame.right).hand[4]                  / DIVISOR_HAND);
		this->_neurons[STATE_OFFSET + 0x1B + 0x00]->setValue((isLeft ? frame.left : frame.right).cardGauge                / DIVISOR_CARD_GAUGE);
		this->_neurons[STATE_OFFSET + 0x1C + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[0].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x1D + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[1].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x1E + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[2].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x1F + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[3].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x20 + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[4].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x21 + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[5].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x22 + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[6].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x23 + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[7].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x24 + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[8].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x25 + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[9].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x26 + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[10].level         / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x27 + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[11].level         / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x28 + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[12].level         / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x29 + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[13].level         / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x2A + 0x00]->setValue((isLeft ? frame.left : frame.right).skills[14].level         / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x2B + 0x00]->setValue((isLeft ? frame.left : frame.right).fanLevel                 / DIVISOR_FAN_LEVEL);
		this->_neurons[STATE_OFFSET + 0x2C + 0x00]->setValue((isLeft ? frame.left : frame.right).dropInvulTimeLeft        / DIVISOR_DROP_INVUL_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x2D + 0x00]->setValue((isLeft ? frame.left : frame.right).superArmorTimeLeft       / DIVISOR_SUPER_ARMOR_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x2E + 0x00]->setValue((isLeft ? frame.left : frame.right).superArmorHp             / DIVISOR_SUPER_ARMOR_HP);
		this->_neurons[STATE_OFFSET + 0x2F + 0x00]->setValue((isLeft ? frame.left : frame.right).milleniumVampireTimeLeft / DIVISOR_MILLENIUM_VAMPIRE_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x30 + 0x00]->setValue((isLeft ? frame.left : frame.right).philosoferStoneTimeLeft  / DIVISOR_PHILOSOFER_STONE_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x31 + 0x00]->setValue((isLeft ? frame.left : frame.right).sakuyasWorldTimeLeft     / DIVISOR_SAKUYAS_WORLD_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x32 + 0x00]->setValue((isLeft ? frame.left : frame.right).privateSquareTimeLeft    / DIVISOR_PRIVATE_SQUARE_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x33 + 0x00]->setValue((isLeft ? frame.left : frame.right).orreriesTimeLeft         / DIVISOR_ORRERIES_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x34 + 0x00]->setValue((isLeft ? frame.left : frame.right).mppTimeLeft              / DIVISOR_MPP_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x35 + 0x00]->setValue((isLeft ? frame.left : frame.right).kanakoCooldown           / DIVISOR_KANAKO_COOLDOWN);
		this->_neurons[STATE_OFFSET + 0x36 + 0x00]->setValue((isLeft ? frame.left : frame.right).suwakoCooldown           / DIVISOR_SUWAKO_COOLDOWN);

		this->_neurons[STATE_OFFSET + 0x00 + 0x37]->setValue((isLeft ? frame.right : frame.left).direction                / DIVISOR_DIRECTION);
		this->_neurons[STATE_OFFSET + 0x01 + 0x37]->setValue((isLeft ? frame.right : frame.left).opponentRelativePos.x    / DIVISOR_OPPONENT_RELATIVE_POS_X);
		this->_neurons[STATE_OFFSET + 0x02 + 0x37]->setValue((isLeft ? frame.right : frame.left).opponentRelativePos.y    / DIVISOR_OPPONENT_RELATIVE_POS_Y);
		this->_neurons[STATE_OFFSET + 0x03 + 0x37]->setValue((isLeft ? frame.right : frame.left).distToBackCorner         / DIVISOR_DIST_TO_LEFT_CORNER);
		this->_neurons[STATE_OFFSET + 0x04 + 0x37]->setValue((isLeft ? frame.right : frame.left).distToFrontCorner        / DIVISOR_DIST_TO_RIGHT_CORNER);
		this->_neurons[STATE_OFFSET + 0x05 + 0x37]->setValue((isLeft ? frame.right : frame.left).distToGround             / DIVISOR_DIST_TO_GROUND);
		this->_neurons[STATE_OFFSET + 0x06 + 0x37]->setValue((isLeft ? frame.right : frame.left).action                   / DIVISOR_ACTION);
		this->_neurons[STATE_OFFSET + 0x07 + 0x37]->setValue((isLeft ? frame.right : frame.left).actionBlockId            / DIVISOR_ACTION_BLOCK_ID);
		this->_neurons[STATE_OFFSET + 0x08 + 0x37]->setValue((isLeft ? frame.right : frame.left).animationCounter         / DIVISOR_ANIMATION_COUNTER);
		this->_neurons[STATE_OFFSET + 0x09 + 0x37]->setValue((isLeft ? frame.right : frame.left).animationSubFrame        / DIVISOR_ANIMATION_SUBFRAME);
		this->_neurons[STATE_OFFSET + 0x0A + 0x37]->setValue((isLeft ? frame.right : frame.left).frameCount               / DIVISOR_FRAME_COUNT);
		this->_neurons[STATE_OFFSET + 0x0B + 0x37]->setValue((isLeft ? frame.right : frame.left).comboDamage              / DIVISOR_COMBO_DAMAGE);
		this->_neurons[STATE_OFFSET + 0x0C + 0x37]->setValue((isLeft ? frame.right : frame.left).comboLimit               / DIVISOR_COMBO_LIMIT);
		this->_neurons[STATE_OFFSET + 0x0D + 0x37]->setValue((isLeft ? frame.right : frame.left).airBorne                 / DIVISOR_AIRBORNE);
		this->_neurons[STATE_OFFSET + 0x0E + 0x37]->setValue((isLeft ? frame.right : frame.left).hp                       / DIVISOR_HP);
		this->_neurons[STATE_OFFSET + 0x0F + 0x37]->setValue((isLeft ? frame.right : frame.left).airDashCount             / DIVISOR_AIR_DASH_COUNT);
		this->_neurons[STATE_OFFSET + 0x10 + 0x37]->setValue((isLeft ? frame.right : frame.left).spirit                   / DIVISOR_SPIRIT);
		this->_neurons[STATE_OFFSET + 0x11 + 0x37]->setValue((isLeft ? frame.right : frame.left).maxSpirit                / DIVISOR_MAX_SPIRIT);
		this->_neurons[STATE_OFFSET + 0x12 + 0x37]->setValue((isLeft ? frame.right : frame.left).untech                   / DIVISOR_UNTECH);
		this->_neurons[STATE_OFFSET + 0x13 + 0x37]->setValue((isLeft ? frame.right : frame.left).healingCharm             / DIVISOR_HEALING_CHARM);
		this->_neurons[STATE_OFFSET + 0x14 + 0x37]->setValue((isLeft ? frame.right : frame.left).swordOfRapture           / DIVISOR_SWORD_OF_RAPTURE);
		this->_neurons[STATE_OFFSET + 0x15 + 0x37]->setValue((isLeft ? frame.right : frame.left).score                    / DIVISOR_SCORE);
		this->_neurons[STATE_OFFSET + 0x16 + 0x37]->setValue((isLeft ? frame.right : frame.left).hand[0]                  / DIVISOR_HAND);
		this->_neurons[STATE_OFFSET + 0x17 + 0x37]->setValue((isLeft ? frame.right : frame.left).hand[1]                  / DIVISOR_HAND);
		this->_neurons[STATE_OFFSET + 0x18 + 0x37]->setValue((isLeft ? frame.right : frame.left).hand[2]                  / DIVISOR_HAND);
		this->_neurons[STATE_OFFSET + 0x19 + 0x37]->setValue((isLeft ? frame.right : frame.left).hand[3]                  / DIVISOR_HAND);
		this->_neurons[STATE_OFFSET + 0x1A + 0x37]->setValue((isLeft ? frame.right : frame.left).hand[4]                  / DIVISOR_HAND);
		this->_neurons[STATE_OFFSET + 0x1B + 0x37]->setValue((isLeft ? frame.right : frame.left).cardGauge                / DIVISOR_CARD_GAUGE);
		this->_neurons[STATE_OFFSET + 0x1C + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[0].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x1D + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[1].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x1E + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[2].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x1F + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[3].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x20 + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[4].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x21 + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[5].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x22 + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[6].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x23 + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[7].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x24 + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[8].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x25 + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[9].level          / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x26 + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[10].level         / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x27 + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[11].level         / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x28 + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[12].level         / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x29 + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[13].level         / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x2A + 0x37]->setValue((isLeft ? frame.right : frame.left).skills[14].level         / DIVISOR_SKILLS);
		this->_neurons[STATE_OFFSET + 0x2B + 0x37]->setValue((isLeft ? frame.right : frame.left).fanLevel                 / DIVISOR_FAN_LEVEL);
		this->_neurons[STATE_OFFSET + 0x2C + 0x37]->setValue((isLeft ? frame.right : frame.left).dropInvulTimeLeft        / DIVISOR_DROP_INVUL_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x2D + 0x37]->setValue((isLeft ? frame.right : frame.left).superArmorTimeLeft       / DIVISOR_SUPER_ARMOR_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x2E + 0x37]->setValue((isLeft ? frame.right : frame.left).superArmorHp             / DIVISOR_SUPER_ARMOR_HP);
		this->_neurons[STATE_OFFSET + 0x2F + 0x37]->setValue((isLeft ? frame.right : frame.left).milleniumVampireTimeLeft / DIVISOR_MILLENIUM_VAMPIRE_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x30 + 0x37]->setValue((isLeft ? frame.right : frame.left).philosoferStoneTimeLeft  / DIVISOR_PHILOSOFER_STONE_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x31 + 0x37]->setValue((isLeft ? frame.right : frame.left).sakuyasWorldTimeLeft     / DIVISOR_SAKUYAS_WORLD_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x32 + 0x37]->setValue((isLeft ? frame.right : frame.left).privateSquareTimeLeft    / DIVISOR_PRIVATE_SQUARE_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x33 + 0x37]->setValue((isLeft ? frame.right : frame.left).orreriesTimeLeft         / DIVISOR_ORRERIES_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x34 + 0x37]->setValue((isLeft ? frame.right : frame.left).mppTimeLeft              / DIVISOR_MPP_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x35 + 0x37]->setValue((isLeft ? frame.right : frame.left).kanakoCooldown           / DIVISOR_KANAKO_COOLDOWN);
		this->_neurons[STATE_OFFSET + 0x36 + 0x37]->setValue((isLeft ? frame.right : frame.left).suwakoCooldown           / DIVISOR_SUWAKO_COOLDOWN);

		auto value = this->_neurons[INPUT_NEURONS_COUNT]->getValue();
		int result = round((value + 1) * BaseAI::actions.size() / 2.f - 0.5f);
		try {
			return BaseAI::actions.at(min(BaseAI::actions.size() - 1, max(0, result)));
		} catch (std::exception &e) {
			printf("%u %f %i\n", min(BaseAI::actions.size() - 1, max(0, result)), value, result);
			throw;
		}
	}

	std::string NeuronAI::toString() const
	{
		return std::string("NAI ") + chrNames[this->_character] + " gen" + std::to_string(this->_generation) + "-" + std::to_string(this->_id);
	}
}