//
// Created by PinkySmile on 22/05/2021.
//

#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#ifdef _WIN32
#include <Windows.h>
#else
using std::max;
using std::min;
#endif
#include "Neuron.hpp"

#define LINK_BREAK_CHANCE_NUM 1
#define LINK_BREAK_CHANCE_DEN 1000
#define LINK_CREATE_CHANCE_NUM 4
#define LINK_CREATE_CHANCE_DEN 1000

namespace Trainer
{
	Neuron::Neuron(unsigned int i) :
		_id(i),
		_value(2.f * rand() / RAND_MAX - 1)
	{
	}

	Neuron::Neuron(const Neuron &other)
	{
		this->_id = other._id;
		this->_value = other._value;
		this->_links.reserve(other._links.size());
		for (auto &link : other._links)
			this->_links.push_back({
				link.id,
				link.weight,
				nullptr
			});
	}

	void Neuron::loadFromLine(const std::string &line)
	{
		unsigned i;
		std::string token;
		std::stringstream stream{line};

		std::getline(stream, token, ',');
		this->_id = std::stoul(token);

		std::getline(stream, token, ',');
		i = std::stoul(token);

		std::getline(stream, token, ',');
		this->_value = std::stof(token);

		this->_links.clear();
		this->_links.reserve(i);
		while (i--) {
			std::string weight;
			std::string id;

			std::getline(stream, weight, ',');
			std::getline(stream, id, ',');
			this->_links.push_back({
				(unsigned)std::stoul(id),
				std::stof(weight),
				nullptr
			});
		}
	}

	void Neuron::resolveLinks(const std::vector<std::unique_ptr<Neuron>> &elems)
	{
		for (int i = 0; i < this->_links.size(); i++) {
			auto &link = this->_links[i];

			if (!link.neuron)
				for (auto &neuron : elems)
					if (neuron->getId() == link.id) {
						link.neuron = &*neuron;
						break;
					}
			if (!link.neuron) {
				this->_links.erase(this->_links.begin() + i);
				i--;
			}
		}
	}

	void Neuron::mutate(const std::vector<std::unique_ptr<Neuron>> &others)
	{
		for (int i = 0; i < this->_links.size(); i++) {
			auto &link = this->_links[i];

			// Mutate links
			if ((rand() & 1) == 0)
				link.weight += std::exp(rand() / -2.f / RAND_MAX) / 4;
			else
				link.weight -= std::exp(rand() / -2.f / RAND_MAX) / 4;
			link.weight = max(min(1.f, link.weight), -1.f);
			if (!this->_links.empty() && rand() % LINK_BREAK_CHANCE_DEN < LINK_BREAK_CHANCE_NUM) {
				this->_links.erase(this->_links.begin() + i);
				i--;
			}
		}
		if ((rand() & 1) == 0)
			this->_value += std::exp(rand() / -2.f / RAND_MAX) / 4;
		else
			this->_value -= std::exp(rand() / -2.f / RAND_MAX) / 4;
		// Add some links
		while (rand() % LINK_CREATE_CHANCE_DEN < LINK_CREATE_CHANCE_NUM) {
			std::vector<Neuron *> result;

			for (auto &neur : others) {
				auto deps = neur->getDependencies();

				if (std::find(deps.begin(), deps.end(), this->_id) == deps.end())
					result.push_back(&*neur);
			}
			if (!result.empty()) {
				auto r = result[rand() % result.size()];

				this->_links.push_back({
					r->getId(),
					rand() * 2.f / RAND_MAX - 1,
					r
				});
			} else
				break;
		}
	}

	float Neuron::getValue()
	{
		float result = 0;

		for (auto &link : this->_links)
			result += link.weight * link.neuron->getValue();
		return min(max(result + this->_value, -1.f), 1.f);
	}

	std::set<unsigned> Neuron::getDependencies()
	{
		std::set<unsigned> deps;

		for (auto &link : this->_links) {
			const auto &innerDeps = link.neuron->getDependencies();

			deps.insert(innerDeps.begin(), innerDeps.end());
		}
		deps.insert(this->_id);
		return deps;
	}

	void Neuron::save(std::ofstream &stream)
	{
		stream << this->_id << ',';
		stream << this->_links.size() << ',';
		stream << this->_value;
		for (auto &link : this->_links)
			stream << ',' << link.weight << ',' << link.id;
		stream << std::endl;
	}

	unsigned int Neuron::getId() const
	{
		return this->_id;
	}

	void Neuron::setValue(float value)
	{
		this->_value = value;
	}

	void Neuron::link(Neuron *neuron, float weight)
	{
		this->_links.push_back({
			neuron->getId(),
			weight,
			neuron
		});
	}
}