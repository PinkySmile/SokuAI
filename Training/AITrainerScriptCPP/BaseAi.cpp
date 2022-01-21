//
// Created by PinkySmile on 21/05/2021.
//

#include "BaseAi.hpp"

#define DOWN 1
#define UP -1
#define BACK -1
#define FRONT 1
#define NEUTRAL 0

namespace Trainer
{
	const std::vector<const char *> BaseAI::actions = {
		"nothing",
		"forward",
		"backward",
		"block_low",
		"crouch",
		"forward_jump",
		"backward_jump",
		"neutral_jump",
		"forward_highjump",
		"backward_highjump",
		"neutral_highjump",
		"forward_dash",
		"backward_dash",
		"fly1",
		"fly2",
		"fly3",
		"fly4",
		"fly6",
		"fly7",
		"fly8",
		"fly9",
		"switch_to_card_1",
		"switch_to_card_2",
		"switch_to_card_3",
		"switch_to_card_4",
		"use_current_card",
		"upgrade_skillcard",
		"236b",
		"236c",
		"623b",
		"623c",
		"214b",
		"214c",
		"421b",
		"421c",
		"22b",
		"22c",
		"be1",
		"be2",
		"be4",
		"be6",
		"2b",
		"3b",
		"4b",
		"5b",
		"6b",
		"66b",
		"1c",
		"2c",
		"3c",
		"4c",
		"5c",
		"6c",
		"66c",
		"1a",
		"2a",
		"3a",
		"5a",
		"6a",
		"8a",
		"66a",
		"4a",
	};
	const std::map<std::string, std::vector<Input>> BaseAI::actionsBuffers = {
		{"1a", {
			{ true, false, false, false, false, false, BACK, DOWN }
		}},
		{"2a", {
			{ true, false, false, false, false, false, NEUTRAL, DOWN }
		}},
		{"3a", {
			{ true, false, false, false, false, false, FRONT, DOWN }
		}},
		{"4a", {
			{ true, false, false, false, false, false, BACK, NEUTRAL }
		}},
		{"5a", {
			{ true, false, false, false, false, false, NEUTRAL, NEUTRAL }
		}},
		{"6a", {
			{ true, false, false, false, false, false, FRONT, NEUTRAL }
		}},
		{"8a", {
			{ true, false, false, false, false, false, NEUTRAL, UP }
		}},
		{"66a", {
			{ true,  false, false, false, false, false, FRONT,   NEUTRAL },
			{ false, false, false, false, false, false, FRONT,   NEUTRAL },
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, false, FRONT,   NEUTRAL },
		}},
		{"2b", {
			{ false, true, false, false, false, false, NEUTRAL, DOWN }
		}},
		{"3b", {
			{ false, true, false, false, false, false, FRONT, DOWN }
		}},
		{"4b", {
			{ false, true, false, false, false, false, BACK, NEUTRAL }
		}},
		{"5b", {
			{ false, true, false, false, false, false, NEUTRAL, NEUTRAL }
		}},
		{"6b", {
			{ false, true, false, false, false, false, FRONT, NEUTRAL }
		}},
		{"66b", {
			{ false, true,  false, false, false, false, FRONT,   NEUTRAL },
			{ false, false, false, false, false, false, FRONT,   NEUTRAL },
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, false, FRONT,   NEUTRAL },
		}},
		{"1c", {
			{ false, false, true, false, false, false, BACK, DOWN }
		}},
		{"2c", {
			{ false, false, true, false, false, false, NEUTRAL, DOWN }
		}},
		{"3c", {
			{ false, false, true, false, false, false, FRONT, DOWN }
		}},
		{"4c", {
			{ false, false, true, false, false, false, BACK, NEUTRAL }
		}},
		{"5c", {
			{ false, false, true, false, false, false, NEUTRAL, NEUTRAL }
		}},
		{"6c", {
			{ false, false, true, false, false, false, FRONT, NEUTRAL }
		}},
		{"66c", {
			{ false, false, true,  false, false, false, FRONT,   NEUTRAL },
			{ false, false, false, false, false, false, FRONT,   NEUTRAL },
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, false, FRONT,   NEUTRAL },
		}},
		{"236b", {
			{ false, true,  false, false, false, false, FRONT,   NEUTRAL },
			{ false, false, false, false, false, false, FRONT,   DOWN },
			{ false, false, false, false, false, false, NEUTRAL, DOWN },
		}},
		{"236c", {
			{ false, false, true,  false, false, false, FRONT,   NEUTRAL },
			{ false, false, false, false, false, false, FRONT,   DOWN },
			{ false, false, false, false, false, false, NEUTRAL, DOWN },
		}},
		{"623b", {
			{ false, true,  false, false, false, false, FRONT,   DOWN },
			{ false, false, false, false, false, false, NEUTRAL, DOWN },
			{ false, false, false, false, false, false, FRONT,   NEUTRAL },
		}},
		{"623c", {
			{ false, false, true,  false, false, false, FRONT,   DOWN },
			{ false, false, false, false, false, false, NEUTRAL, DOWN },
			{ false, false, false, false, false, false, FRONT,   NEUTRAL },
		}},
		{"214b", {
			{ false, true,  false, false, false, false, BACK,    NEUTRAL },
			{ false, false, false, false, false, false, BACK,    DOWN },
			{ false, false, false, false, false, false, NEUTRAL, DOWN },
		}},
		{"214c", {
			{ false, false, true,  false, false, false, BACK,    NEUTRAL },
			{ false, false, false, false, false, false, BACK,    DOWN },
			{ false, false, false, false, false, false, NEUTRAL, DOWN },
		}},
		{"421b", {
			{ false, true,  false, false, false, false, BACK,    DOWN },
			{ false, false, false, false, false, false, NEUTRAL, DOWN },
			{ false, false, false, false, false, false, BACK,    NEUTRAL },
		}},
		{"421c", {
			{ false, false, true,  false, false, false, BACK,    DOWN },
			{ false, false, false, false, false, false, NEUTRAL, DOWN },
			{ false, false, false, false, false, false, BACK,    NEUTRAL },
		}},
		{"22b", {
			{ false, true,  false, false, false, false, NEUTRAL, DOWN },
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, false, NEUTRAL, DOWN },
		}},
		{"22c", {
			{ false, false, true,  false, false, false, NEUTRAL, DOWN },
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, false, NEUTRAL, DOWN },
		}},
		{"nothing", {
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL }
		}},
		{"forward", {
			{ false, false, false, false, false, false, FRONT, NEUTRAL }
		}},
		{"backward", {
			{ false, false, false, false, false, false, BACK, NEUTRAL }
		}},
		{"block_low", {
			{ false, false, false, false, false, false, BACK, DOWN }
		}},
		{"crouch", {
			{ false, false, false, false, false, false, NEUTRAL, DOWN }
		}},
		{"forward_jump", {
			{ false, false, false, false, false, false, FRONT, UP }
		}},
		{"backward_jump", {
			{ false, false, false, false, false, false, BACK, UP }
		}},
		{"neutral_jump", {
			{ false, false, false, false, false, false, NEUTRAL, UP }
		}},
		{"forward_highjump", {
			{ false, false, false, true, false, false, FRONT, UP }
		}},
		{"backward_highjump", {
			{ false, false, false, true, false, false, BACK, UP }
		}},
		{"neutral_highjump", {
			{ false, false, false, true, false, false, NEUTRAL, UP }
		}},
		{"forward_dash", {
			{ false, false, false, false, false, false, FRONT,   NEUTRAL },
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, false, FRONT,   NEUTRAL },
		}},
		{"backward_dash", {
			{ false, false, false, false, false, false, BACK,    NEUTRAL },
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, false, BACK,    NEUTRAL },
		}},
		{"fly1", {
			{ false, false, false, true, false, false, BACK, UP }
		}},
		{"fly2", {
			{ false, false, false, true, false, false, NEUTRAL, UP }
		}},
		{"fly3", {
			{ false, false, false, true, false, false, FRONT, UP }
		}},
		{"fly4", {
			{ false, false, false, true, false, false, BACK, NEUTRAL }
		}},
		{"fly6", {
			{ false, false, false, true, false, false, FRONT, NEUTRAL }
		}},
		{"fly7", {
			{ false, false, false, true, false, false, BACK, DOWN }
		}},
		{"fly8", {
			{ false, false, false, true, false, false, NEUTRAL, DOWN }
		}},
		{"fly9", {
			{ false, false, false, true, false, false, FRONT, DOWN }
		}},
		{"be1", {
			{ false, false, false, true,  false, false, BACK, DOWN },
			{ false, false, false, false, false, false, BACK, DOWN },
			{ false, false, false, true,  false, false, BACK, DOWN },
		}},
		{"be2", {
			{ false, false, false, true, false,  false, NEUTRAL, DOWN },
			{ false, false, false, false, false, false, NEUTRAL, DOWN },
			{ false, false, false, true, false,  false, NEUTRAL, DOWN },
		}},
		{"be4", {
			{ false, false, false, true, false,  false, BACK, NEUTRAL },
			{ false, false, false, false, false, false, BACK, NEUTRAL },
			{ false, false, false, true, false,  false, BACK, NEUTRAL },
		}},
		{"be6", {
			{ false, false, false, true, false,  false, FRONT, NEUTRAL },
			{ false, false, false, false, false, false, FRONT, NEUTRAL },
			{ false, false, false, true, false,  false, FRONT, NEUTRAL },
		}},
		{"switch_to_card_1", {
			{ false, false, false, false, false, true, NEUTRAL, NEUTRAL },
		}},
		{"switch_to_card_2", {
			{ false, false, false, false, false, true,  NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, true,  NEUTRAL, NEUTRAL },
		}},
		{"switch_to_card_3", {
			{ false, false, false, false, false, true,  NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, true,  NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, true,  NEUTRAL, NEUTRAL },
		}},
		{"switch_to_card_4", {
			{ false, false, false, false, false, true,  NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, true,  NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, true,  NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, false, NEUTRAL, NEUTRAL },
			{ false, false, false, false, false, true,  NEUTRAL, NEUTRAL },
		}},
		{"use_current_card", {
			{ false, false, false, false, true, false, NEUTRAL, NEUTRAL }
		}},
		{"upgrade_skillcard", {
			{ false, false, false, false, true, false, BACK, NEUTRAL }
		}}
	};

	BaseAI::BaseAI(SokuLib::Character character, unsigned char palette) :
		_character(character),
		_palette(palette)
	{
	}

	const char *BaseAI::getAction(const GameInstance::GameFrame &frame, bool isLeft)
	{
		return this->getAction(frame, isLeft, -1);
	}

	Input BaseAI::getInputs(const GameInstance::GameFrame &frame, bool isLeft)
	{
		return this->getInputs(frame, isLeft, -1);
	}

	Input BaseAI::getInputs(const GameInstance::GameFrame &frame, bool isLeft, unsigned int frameId)
	{
		if (this->_bufferedInput.empty())
			this->_bufferedInput = BaseAI::actionsBuffers.at(this->getAction(frame, isLeft, frameId));

		Input input = this->_bufferedInput.back();

		input.H *= isLeft ? frame.left.direction : frame.right.direction;
		this->_bufferedInput.pop_back();
		return input;
	}

	const char *BaseAI::getAction(const GameInstance::GameFrame &frame, bool isLeft, unsigned int frameId)
	{
		return BaseAI::actions[rand() % BaseAI::actions.size()];
	}
}