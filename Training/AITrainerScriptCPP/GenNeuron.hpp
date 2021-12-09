//
// Created by PinkySmile on 08/12/2021.
//

#ifndef SOKUAI_GENNEURON_HPP
#define SOKUAI_GENNEURON_HPP


#include "Neuron.hpp"

namespace Trainer
{
	class GenNeuron : public Neuron {
	private:
		struct Link {
			Neuron &neuron;
			float weight;
			float add;

			Link(Neuron &neuron, float weight, float add) :
				neuron(neuron),
				weight(weight),
				add(add)
			{}
		};

		float _val = 0;
		bool _computed = false;
		std::vector<Link> _links;

	public:
		void startComputed();
		void addLink(float weight, float add, Neuron &neuron);
		float getValue() override;
	};
}


#endif //SOKUAI_GENNEURON_HPP
