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

namespace Trainer
{
	const char *NeuronAI::chrNames[] = {
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
		"Suwako",
		"Random"
	};

	const std::string NeuronAI::_path = std::string(__argv[0]).substr(0, std::string(__argv[0]).find_last_of('\\')) + "\\generated\\";

	void NeuronAI::_createBaseNeurons()
	{
		this->_neurons.clear();
		this->_neurons.reserve(STATE_NEURONS_COUNT);
		this->_myObjects = new ObjectsNeuron(0);
		this->_opObjects = new ObjectsNeuron(1);
		this->_neurons.emplace_back(this->_myObjects);
		this->_neurons.emplace_back(this->_opObjects);
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
		stream << static_cast<int>(this->_palette) << std::endl;
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
		this->loadFile(
			myChar,
			NeuronAI::_path + chrNames[myChar] + " vs " + chrNames[opChar] + "/" +
			std::to_string(this->_generation) + "/" +
			std::to_string(this->_id) + ".neur"
		);
	}

	NeuronAI *NeuronAI::mateOnce(const NeuronAI &other, unsigned int id, SokuLib::Character myChar, SokuLib::Character opChar, unsigned currentLatestGen) const
	{
		auto child = new NeuronAI((rand() & 1) == 0 ? this->_palette : other._palette, id, currentLatestGen + 1);
		auto objs1 = new ObjectsNeuron(*reinterpret_cast<ObjectsNeuron *>(&*((rand() & 1) == 0 ? this->_neurons[0] : other._neurons[0])));
		auto objs2 = new ObjectsNeuron(*reinterpret_cast<ObjectsNeuron *>(&*((rand() & 1) == 0 ? this->_neurons[1] : other._neurons[1])));

		child->_createBaseNeurons();
		child->_myObjects = objs1;
		child->_opObjects = objs2;
		child->_neurons[OBJECTS_OFFSET].reset(objs1);
		child->_neurons[OBJECTS_OFFSET + 1].reset(objs2);
		child->_character = myChar;
		if ((rand() & 0xFF) == 0)
			child->_palette = rand() % 72;

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

	std::vector<NeuronAI *> NeuronAI::mate(const NeuronAI &other, SokuLib::Character myChar, SokuLib::Character opChar, unsigned int startId, unsigned int nb, unsigned currentLatestGen) const
	{
		std::vector<NeuronAI *> result;

		while (nb--) {
			result.push_back(this->mateOnce(other, startId, myChar, opChar, currentLatestGen));
			startId++;
		}
		return result;
	}

	const char *NeuronAI::getAction(const GameInstance::GameFrame &frame, bool isLeft)
	{
		this->_myObjects->objects = isLeft ? frame.leftObjects : frame.rightObjects;
		this->_opObjects->objects = isLeft ? frame.rightObjects : frame.leftObjects;

		// Weather
		this->_neurons[WEATHER_OFFSET + 0]->setValue(frame.displayedWeather <= 19 ? frame.displayedWeather / 19.f : -1);
		this->_neurons[WEATHER_OFFSET + 1]->setValue(frame.activeWeather <= 19 ? frame.activeWeather / 19.f : -1);
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
		for (int i = 0; i < 5; i++) {
			auto card = (isLeft ? frame.left : frame.right).hand[i];

			this->_neurons[STATE_OFFSET + 0x16 + 0x00 + i]->setValue(card == 0xFF ? -1 : (card / DIVISOR_HAND));
		}
		this->_neurons[STATE_OFFSET + 0x1B + 0x00]->setValue((isLeft ? frame.left : frame.right).cardGauge                / DIVISOR_CARD_GAUGE);
		for (int i = 0; i < 15; i++) {
			auto skill = (isLeft ? frame.left : frame.right).skills[i];

			this->_neurons[STATE_OFFSET + 0x1C + 0x00 + i]->setValue(skill.notUsed ? -1 : (skill.level / DIVISOR_SKILLS));
		}
		this->_neurons[STATE_OFFSET + 0x2B + 0x00]->setValue((isLeft ? frame.left : frame.right).fanLevel                 / DIVISOR_FAN_LEVEL);
		this->_neurons[STATE_OFFSET + 0x2C + 0x00]->setValue((isLeft ? frame.left : frame.right).dropInvulTimeLeft        / DIVISOR_DROP_INVUL_TIME_LEFT);

		auto t = (isLeft ? frame.left : frame.right).superArmorTimeLeft;
		auto h = (isLeft ? frame.left : frame.right).superArmorHp;

		this->_neurons[STATE_OFFSET + 0x2D + 0x00]->setValue(t < 0 ? t : t / DIVISOR_SUPER_ARMOR_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x2E + 0x00]->setValue(h < 0 ? h : h / DIVISOR_SUPER_ARMOR_HP);
		this->_neurons[STATE_OFFSET + 0x2F + 0x00]->setValue((isLeft ? frame.left : frame.right).milleniumVampireTimeLeft / DIVISOR_MILLENIUM_VAMPIRE_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x30 + 0x00]->setValue((isLeft ? frame.left : frame.right).philosoferStoneTimeLeft  / DIVISOR_PHILOSOFER_STONE_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x31 + 0x00]->setValue((isLeft ? frame.left : frame.right).sakuyasWorldTimeLeft     / DIVISOR_SAKUYAS_WORLD_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x32 + 0x00]->setValue((isLeft ? frame.left : frame.right).privateSquareTimeLeft    / DIVISOR_PRIVATE_SQUARE_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x33 + 0x00]->setValue((isLeft ? frame.left : frame.right).orreriesTimeLeft         / DIVISOR_ORRERIES_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x34 + 0x00]->setValue((isLeft ? frame.left : frame.right).mppTimeLeft              / DIVISOR_MPP_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x35 + 0x00]->setValue((isLeft ? frame.left : frame.right).kanakoCooldown           / DIVISOR_KANAKO_COOLDOWN);
		this->_neurons[STATE_OFFSET + 0x36 + 0x00]->setValue((isLeft ? frame.left : frame.right).suwakoCooldown           / DIVISOR_SUWAKO_COOLDOWN);



		this->_neurons[STATE_OFFSET + 0x00 + 0x37]->setValue((!isLeft ? frame.left : frame.right).direction                / DIVISOR_DIRECTION);
		this->_neurons[STATE_OFFSET + 0x01 + 0x37]->setValue((!isLeft ? frame.left : frame.right).opponentRelativePos.x    / DIVISOR_OPPONENT_RELATIVE_POS_X);
		this->_neurons[STATE_OFFSET + 0x02 + 0x37]->setValue((!isLeft ? frame.left : frame.right).opponentRelativePos.y    / DIVISOR_OPPONENT_RELATIVE_POS_Y);
		this->_neurons[STATE_OFFSET + 0x03 + 0x37]->setValue((!isLeft ? frame.left : frame.right).distToBackCorner         / DIVISOR_DIST_TO_LEFT_CORNER);
		this->_neurons[STATE_OFFSET + 0x04 + 0x37]->setValue((!isLeft ? frame.left : frame.right).distToFrontCorner        / DIVISOR_DIST_TO_RIGHT_CORNER);
		this->_neurons[STATE_OFFSET + 0x05 + 0x37]->setValue((!isLeft ? frame.left : frame.right).distToGround             / DIVISOR_DIST_TO_GROUND);
		this->_neurons[STATE_OFFSET + 0x06 + 0x37]->setValue((!isLeft ? frame.left : frame.right).action                   / DIVISOR_ACTION);
		this->_neurons[STATE_OFFSET + 0x07 + 0x37]->setValue((!isLeft ? frame.left : frame.right).actionBlockId            / DIVISOR_ACTION_BLOCK_ID);
		this->_neurons[STATE_OFFSET + 0x08 + 0x37]->setValue((!isLeft ? frame.left : frame.right).animationCounter         / DIVISOR_ANIMATION_COUNTER);
		this->_neurons[STATE_OFFSET + 0x09 + 0x37]->setValue((!isLeft ? frame.left : frame.right).animationSubFrame        / DIVISOR_ANIMATION_SUBFRAME);
		this->_neurons[STATE_OFFSET + 0x0A + 0x37]->setValue((!isLeft ? frame.left : frame.right).frameCount               / DIVISOR_FRAME_COUNT);
		this->_neurons[STATE_OFFSET + 0x0B + 0x37]->setValue((!isLeft ? frame.left : frame.right).comboDamage              / DIVISOR_COMBO_DAMAGE);
		this->_neurons[STATE_OFFSET + 0x0C + 0x37]->setValue((!isLeft ? frame.left : frame.right).comboLimit               / DIVISOR_COMBO_LIMIT);
		this->_neurons[STATE_OFFSET + 0x0D + 0x37]->setValue((!isLeft ? frame.left : frame.right).airBorne                 / DIVISOR_AIRBORNE);
		this->_neurons[STATE_OFFSET + 0x0E + 0x37]->setValue((!isLeft ? frame.left : frame.right).hp                       / DIVISOR_HP);
		this->_neurons[STATE_OFFSET + 0x0F + 0x37]->setValue((!isLeft ? frame.left : frame.right).airDashCount             / DIVISOR_AIR_DASH_COUNT);
		this->_neurons[STATE_OFFSET + 0x10 + 0x37]->setValue((!isLeft ? frame.left : frame.right).spirit                   / DIVISOR_SPIRIT);
		this->_neurons[STATE_OFFSET + 0x11 + 0x37]->setValue((!isLeft ? frame.left : frame.right).maxSpirit                / DIVISOR_MAX_SPIRIT);
		this->_neurons[STATE_OFFSET + 0x12 + 0x37]->setValue((!isLeft ? frame.left : frame.right).untech                   / DIVISOR_UNTECH);
		this->_neurons[STATE_OFFSET + 0x13 + 0x37]->setValue((!isLeft ? frame.left : frame.right).healingCharm             / DIVISOR_HEALING_CHARM);
		this->_neurons[STATE_OFFSET + 0x14 + 0x37]->setValue((!isLeft ? frame.left : frame.right).swordOfRapture           / DIVISOR_SWORD_OF_RAPTURE);
		this->_neurons[STATE_OFFSET + 0x15 + 0x37]->setValue((!isLeft ? frame.left : frame.right).score                    / DIVISOR_SCORE);
		for (int i = 0; i < 5; i++) {
			auto card = (!isLeft ? frame.left : frame.right).hand[i];

			this->_neurons[STATE_OFFSET + 0x16 + 0x37 + i]->setValue(card == 0xFF ? -1 : (card / DIVISOR_HAND));
		}
		this->_neurons[STATE_OFFSET + 0x1B + 0x00]->setValue((isLeft ? frame.left : frame.right).cardGauge                / DIVISOR_CARD_GAUGE);
		for (int i = 0; i < 15; i++) {
			auto skill = (!isLeft ? frame.left : frame.right).skills[i];

			this->_neurons[STATE_OFFSET + 0x1C + 0x37 + i]->setValue(skill.notUsed ? -1 : (skill.level / DIVISOR_SKILLS));
		}
		this->_neurons[STATE_OFFSET + 0x2B + 0x37]->setValue((!isLeft ? frame.left : frame.right).fanLevel                 / DIVISOR_FAN_LEVEL);
		this->_neurons[STATE_OFFSET + 0x2C + 0x37]->setValue((!isLeft ? frame.left : frame.right).dropInvulTimeLeft        / DIVISOR_DROP_INVUL_TIME_LEFT);

		t = (!isLeft ? frame.left : frame.right).superArmorTimeLeft;
		h = (!isLeft ? frame.left : frame.right).superArmorHp;

		this->_neurons[STATE_OFFSET + 0x2D + 0x37]->setValue(t < 0 ? t : t / DIVISOR_SUPER_ARMOR_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x2E + 0x37]->setValue(h < 0 ? h : h / DIVISOR_SUPER_ARMOR_HP);
		this->_neurons[STATE_OFFSET + 0x2F + 0x37]->setValue((!isLeft ? frame.left : frame.right).milleniumVampireTimeLeft / DIVISOR_MILLENIUM_VAMPIRE_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x30 + 0x37]->setValue((!isLeft ? frame.left : frame.right).philosoferStoneTimeLeft  / DIVISOR_PHILOSOFER_STONE_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x31 + 0x37]->setValue((!isLeft ? frame.left : frame.right).sakuyasWorldTimeLeft     / DIVISOR_SAKUYAS_WORLD_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x32 + 0x37]->setValue((!isLeft ? frame.left : frame.right).privateSquareTimeLeft    / DIVISOR_PRIVATE_SQUARE_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x33 + 0x37]->setValue((!isLeft ? frame.left : frame.right).orreriesTimeLeft         / DIVISOR_ORRERIES_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x34 + 0x37]->setValue((!isLeft ? frame.left : frame.right).mppTimeLeft              / DIVISOR_MPP_TIME_LEFT);
		this->_neurons[STATE_OFFSET + 0x35 + 0x37]->setValue((!isLeft ? frame.left : frame.right).kanakoCooldown           / DIVISOR_KANAKO_COOLDOWN);
		this->_neurons[STATE_OFFSET + 0x36 + 0x37]->setValue((!isLeft ? frame.left : frame.right).suwakoCooldown           / DIVISOR_SUWAKO_COOLDOWN);

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

	unsigned int NeuronAI::getId() const
	{
		return this->_id;
	}

	int NeuronAI::getGeneration() const
	{
		return this->_generation;
	}

	void NeuronAI::loadFile(SokuLib::Character myChar, const std::string &path)
	{
		std::ifstream stream{path};
		std::string line;

		this->_character = myChar;
		this->_createBaseNeurons();
		printf("Loading %s\n", path.c_str());
		std::getline(stream, line);
		this->_palette = std::stoul(line);
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

	GameInstance::PlayerParams NeuronAI::getParams() const
	{
		GameInstance::PlayerParams params{
			this->_character,
			this->_palette
		};

		buildDeck(this->_character, params.deck);
		strcpy_s(params.name, (std::string("NAI ") + chrNames[this->_character] + " gen" + std::to_string(this->_generation) + "-" + std::to_string(this->_id)).c_str());
		return params;
	}
}