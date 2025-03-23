//
// Created by PinkySmile on 22/03/25.
//

#ifndef SOKUAI_NETWORK_HPP
#define SOKUAI_NETWORK_HPP


#include <SokuLib.hpp>

void swapDisplay(bool enabled);
void stopNetworkThread();
bool isStopped();
int setupNetwork(int argc, char **argv);
void sendFault(PEXCEPTION_POINTERS ptr);
void sendState(const SokuLib::BattleManager &mgr);
void sendGameResult(const SokuLib::BattleManager &mgr);


#endif //SOKUAI_NETWORK_HPP
