//
// Created by PinkySmile on 22/05/2021.
//

#include <sstream>
#include <iostream>
#include <fstream>
#include <windows.h>
#include "ObjectsNeuron.hpp"

namespace Trainer
{
#define DIVISOR_DIRECTION               1.f
#define DIVISOR_RELATIVE_POS_ME_X       1500.f
#define DIVISOR_RELATIVE_POS_ME_Y       1500.f
#define DIVISOR_RELATIVE_POS_OPPONENT_X 1500.f
#define DIVISOR_RELATIVE_POS_OPPONENT_Y 1500.f
#define DIVISOR_ACTION                  65535.f
#define DIVISOR_IMAGEID                 65535.f

	ObjectsNeuron::ObjectsNeuron(unsigned int i) :
		Neuron(i)
	{
		for (auto &weight : this->_weights)
			weight = 2.f * rand() / RAND_MAX - 1;
	}

	void ObjectsNeuron::loadFromLine(const std::string &line)
	{
		std::string token;
		std::stringstream stream{line};

		std::getline(stream, token, ',');
		this->_id = std::stoul(token);
		for (auto &weight : this->_weights) {
			std::getline(stream, token, ',');
			weight = std::stof(token);
		}
	}

	void ObjectsNeuron::mutate(const std::vector<std::unique_ptr<Neuron>> &others)
	{
		for (auto &weight : this->_weights) {
			// Mutate weights
			if ((rand() & 1) == 0)
				weight += std::exp(rand() / -2.f / RAND_MAX) / 4;
			else
				weight -= std::exp(rand() / -2.f / RAND_MAX) / 4;
			weight = max(min(1, weight), -1);
		}
		if ((rand() & 1) == 0)
			this->_value += std::exp(rand() / -2.f / RAND_MAX) / 4;
		else
			this->_value -= std::exp(rand() / -2.f / RAND_MAX) / 4;
	}

	float ObjectsNeuron::getValue()
	{
		float result = 0;

		for (auto &obj : this->objects) {
			result += this->_weights[0] * obj.direction             / DIVISOR_DIRECTION;
			result += this->_weights[1] * obj.relativePosMe.x       / DIVISOR_RELATIVE_POS_ME_X;
			result += this->_weights[2] * obj.relativePosMe.y       / DIVISOR_RELATIVE_POS_ME_Y;
			result += this->_weights[3] * obj.relativePosOpponent.x / DIVISOR_RELATIVE_POS_OPPONENT_X;
			result += this->_weights[4] * obj.relativePosOpponent.y / DIVISOR_RELATIVE_POS_OPPONENT_Y;
			result += this->_weights[5] * obj.action                / DIVISOR_ACTION;
			result += this->_weights[6] * obj.imageID               / DIVISOR_IMAGEID;
		}
		return result;
	}

	std::set<unsigned> ObjectsNeuron::getDependencies()
	{
		return {this->_id};
	}

	void ObjectsNeuron::save(std::ofstream &stream)
	{
		stream << this->_id;
		for (auto weight : this->_weights)
			stream << ',' << weight;
		stream << std::endl;
	}

	ObjectsNeuron::ObjectsNeuron(const ObjectsNeuron &other) :
		Neuron(other)
	{
		for (int i = 0; i < this->_weights.size(); i++)
			this->_weights[i] = other._weights[i];
	}
}