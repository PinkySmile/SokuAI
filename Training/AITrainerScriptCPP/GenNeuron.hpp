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

			Link(Neuron &neuron, float weight) :
				neuron(neuron),
				weight(weight)
			{}
		};

		float _val = 0;
		bool _computed = false;
		std::vector<Link> _links;

	public:
		void startComputed();
		void addLink(float weight, Neuron &neuron);
		float getValue() override;
	};
}


#endif //SOKUAI_GENNEURON_HPP
