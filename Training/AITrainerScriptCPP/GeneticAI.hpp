//
// Created by PinkySmile on 06/12/2021.
//

#ifndef SOKUAI_GENETICAI_HPP
#define SOKUAI_GENETICAI_HPP


#include "BaseAi.hpp"
#include "Neuron.hpp"
#include "ObjectsNeuron.hpp"

namespace Trainer
{
#pragma pack(push, 1)
	union Gene {
		struct {
			bool isInput : 1;
			unsigned short neuronIdIn: 15;
			bool isOutput : 1;
			unsigned short neuronIdOut: 15;
			short weight;
			short add;
		};
		unsigned short data[5];
	};
#pragma pack(pop)

	class GeneticAI : public BaseAI {
	private:
		bool _noWire = true;
		ObjectsNeuron *_myObjects;
		ObjectsNeuron *_opObjects;
		std::vector<Gene> _genome;
		std::vector<std::unique_ptr<Neuron>> _inNeurons;
		std::vector<std::unique_ptr<Neuron>> _neurons;
		std::vector<std::unique_ptr<Neuron>> _outNeurons;
		unsigned _generation;
		unsigned _id;

		void _generateLinks();
		void _createNeurons(unsigned middleLayerSize);
		int _calculatePalette();
		Neuron *getInNeuron(const Gene &gene) const;
		Neuron *getOutNeuron(const Gene &gene);

	public:
		GeneticAI(unsigned generation, unsigned id, unsigned middleLayerSize, unsigned genCount, const std::string &path);
		GeneticAI(unsigned generation, unsigned id, unsigned middleLayerSize, unsigned geneCount);
		GeneticAI(unsigned generation, unsigned id, unsigned middleLayerSize, const GeneticAI &parent1, const GeneticAI &parent2);
		~GeneticAI() override = default;
		std::string toString() const override;
		void onWin(unsigned char myScore, unsigned char opponentScore) override;
		void onLose(unsigned char myScore, unsigned char opponentScore) override;
		void onTimeout(unsigned char myScore, unsigned char opponentScore) override;
		void onGameStart(SokuLib::Character myChr, SokuLib::Character opponentChr, unsigned int inputDelay) override;
		GameInstance::PlayerParams getParams() const override;
		const char *getAction(const GameInstance::GameFrame &frame, bool isLeft, unsigned int frameId) override;
		unsigned int getId() const;
		int getGeneration() const;
		void save(const std::string &path) const;
	};
}


#endif //SOKUAI_GENETICAI_HPP
