//
// Created by PinkySmile on 22/03/25.
//

#ifndef SOKUAI_STATE_HPP
#define SOKUAI_STATE_HPP


#include <SokuLib.hpp>
#include "Packet.hpp"

void fillState(const SokuLib::v2::Player &source, Trainer::CharacterState &destination);
void fillState(const SokuLib::v2::GameObject &object, Trainer::Object &destination);


#endif //SOKUAI_STATE_HPP
