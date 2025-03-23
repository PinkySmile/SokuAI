//
// Created by PinkySmile on 22/03/25.
//

#include "state.hpp"

void fillState(const SokuLib::v2::Player &source, Trainer::CharacterState &destination)
{
	destination.direction = source.direction;
	destination.position = source.position;
	destination.action = source.frameState.actionId;
	destination.sequence = source.frameState.sequenceId;
	destination.pose = source.frameState.poseId;
	destination.poseFrame = source.frameState.poseFrame;
	destination.frameCount = source.frameState.currentFrame;
	destination.comboDamage = source.comboDamage;
	destination.comboLimit = source.comboLimit;
	destination.airBorne = source.gameData.frameData->frameFlags.airborne;
	destination.hp = source.hp;
	destination.airDashCount = source.airDashCount;
	destination.spirit = source.currentSpirit;
	destination.maxSpirit = source.maxSpirit;
	destination.hitStop = source.hitStop;
	destination.timeStop = source.timeStop;
	destination.untech = source.untech;
	destination.healingCharm = source.healCharmTimer;
	destination.confusion = source.confusionDebuffTimer;
	destination.swordOfRapture = source.SORDebuffTimer;
	destination.score = source.score;
	for (int i = 0; i < 5; i++) {
		if (source.handInfo.hand.size() > i)
			destination.hand[i] = { source.handInfo.hand[i].id, source.handInfo.hand[i].cost };
		else
			destination.hand[i] = { 0xFF, 0xFF };
	}
	destination.cardGauge = source.handInfo.cardGauge;
	memcpy(destination.skills, source.skilledSkillLevel, sizeof(destination.skills));
	destination.fanLevel = source.tenguFans;
	destination.rodsLevel = source.controlRod;
	destination.booksLevel = source.grimoireCount;
	destination.dollLevel = source.sacrificialDolls;
	destination.dropLevel = source.drops;
	destination.dropInvulTimeLeft = source.dropInvulTimeLeft;
	destination.superArmorHp = source.superArmorDamageTaken;
	destination.imageID = source.frameData->texIndex;
	destination.objectCount = source.objectList->getList().size();
	memset(destination.characterSpecificData, 0, sizeof(destination.characterSpecificData));
}

void fillState(const SokuLib::v2::GameObject &object, Trainer::Object &destination)
{
	destination.direction = object.direction;
	destination.position = object.position;
	destination.action = object.frameState.actionId;
	destination.sequence = object.frameState.sequenceId;
	destination.imageID = object.frameData->texIndex;
}