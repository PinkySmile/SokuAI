//
// Created by PinkySmile on 22/05/2021.
//

#ifndef SOKUAI_NEURONAI_HPP
#define SOKUAI_NEURONAI_HPP


#include <string>
#include "BaseAi.hpp"
#include "ObjectsNeuron.hpp"

namespace Trainer
{
	class NeuronAI : public BaseAI {
	private:
		ObjectsNeuron *_myObjects;
		ObjectsNeuron *_opObjects;
		std::vector<std::unique_ptr<Neuron>> _neurons;
		unsigned _id;
		int _generation;
		unsigned _inputDelay;

		void _createBaseNeurons();

	public:
		static const char *chrNames[21];
		static const std::string _path;
		static int getLatestGen(SokuLib::Character myChar, SokuLib::Character opChar);

		NeuronAI(unsigned char palette, unsigned index, int generation);
		void createRequiredPath(SokuLib::Character myChar, SokuLib::Character opChar);
		void init(SokuLib::Character myChar, SokuLib::Character opChar);
		void save(SokuLib::Character myChar, SokuLib::Character opChar) const;
		void onGameStart(SokuLib::Character myChar, SokuLib::Character opChar, unsigned inputDelay);
		void loadFile(SokuLib::Character myChar, const std::string &path);
		void loadFile(SokuLib::Character myChar, SokuLib::Character opChar);
		NeuronAI *mateOnce(const NeuronAI &other, unsigned id,SokuLib::Character myChar, SokuLib::Character opChar, unsigned currentLatestGen) const;
		std::vector<NeuronAI *> mate(const NeuronAI &other, SokuLib::Character myCharId, SokuLib::Character opCharId, unsigned startId, unsigned currentLatestGen, unsigned nb) const;
		const char *getAction(const GameInstance::GameFrame &frame, bool isLeft) override;
		std::string toString() const override;
		unsigned int getId() const;
		int getGeneration() const;
		GameInstance::PlayerParams getParams() const override;
	};
}


#endif //SOKUAI_NEURONAI_HPP
