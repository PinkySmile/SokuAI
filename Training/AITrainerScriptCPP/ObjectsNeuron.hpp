//
// Created by PinkySmile on 22/05/2021.
//

#ifndef SOKUAI_OBJECTSNEURON_HPP
#define SOKUAI_OBJECTSNEURON_HPP


#include <array>
#include "Packet.hpp"
#include "Neuron.hpp"

namespace Trainer
{
	class ObjectsNeuron : public Neuron {
	private:
		std::array<float, 7> _weights;

	public:
		std::vector<Object> objects;

		ObjectsNeuron(unsigned i = 0);
		void loadFromLine(const std::string &line) override;
		void mutate(const std::vector<std::unique_ptr<Neuron>> &others) override;
		float getValue() override;
		std::set<unsigned> getDependencies() override;
		void save(std::ofstream &stream) override;
	};
}


#endif //SOKUAI_OBJECTSNEURON_HPP
