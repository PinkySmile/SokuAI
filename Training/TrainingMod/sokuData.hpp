//
// Created by PinkySmile on 22/03/25.
//

#ifndef SOKUAI_SOKUDATA_HPP
#define SOKUAI_SOKUDATA_HPP


#include <map>
#include <SokuLib.hpp>
#include "Packet.hpp"

extern std::vector<unsigned short> moveBlacklist;
extern bool startRequested;
extern volatile bool inputsReceived;
extern unsigned char decks[40];
extern std::pair<Trainer::Input, Trainer::Input> inputs;
extern std::pair<SokuLib::KeyInput, SokuLib::KeyInput> lastInputs;
extern bool setWeather;
extern bool freezeWeather;
extern SokuLib::Weather weather;
extern unsigned short weatherTimer;
extern bool gameFinished;
extern bool displayed;
extern bool cancel;
extern unsigned tps;
extern std::map<SokuLib::Character, std::vector<unsigned short>> characterSpellCards;

void loadCharacterData();
void resetGameState();
void setDisplayMode(bool enabled);


#endif //SOKUAI_SOKUDATA_HPP
