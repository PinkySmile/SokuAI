//
// Created by PinkySmile on 22/05/2021.
//

#ifndef SOKUAI_NEURON_HPP
#define SOKUAI_NEURON_HPP


#include <vector>
#include <memory>
#include <set>

namespace Trainer
{
	struct Link {
		unsigned id;
		float weight;
		class Neuron *neuron;
	};

	class Neuron {
	protected:
		std::vector<Link> _links;
		unsigned _id;
		float _value;

	public:
		Neuron(unsigned i = 0);
		Neuron(const Neuron &);
		virtual void loadFromLine(const std::string &line);
		virtual void resolveLinks(const std::vector<std::unique_ptr<Neuron>> &elems);
		virtual void mutate(const std::vector<std::unique_ptr<Neuron>> &others);
		virtual float getValue();
		virtual std::set<unsigned> getDependencies();
		virtual void save(std::ofstream &stream);
		unsigned int getId() const;
		void link(Neuron *neuron, float weight);
		void setValue(float value);
	};
}


#endif //SOKUAI_NEURON_HPP
