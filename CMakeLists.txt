cmake_minimum_required(VERSION 3.15)
project(SokuAI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# SokuLib
add_subdirectory(SokuLib)

add_library(
	SokuAITrainer
	MODULE
	Training/TrainingMod/Packet.hpp
	Training/TrainingMod/main.cpp
)
target_compile_definitions("SokuAITrainer" PRIVATE DIRECTINPUT_VERSION=0x0800 CURL_STATICLIB _CRT_SECURE_NO_WARNINGS $<$<CONFIG:Debug>:_DEBUG>)
target_link_libraries(SokuAITrainer SokuLib shlwapi ws2_32)

add_library(
	SokuAIOfflinePlayer
	MODULE
	Training/OfflineVersusMod/Packet.hpp
	Training/OfflineVersusMod/main.cpp
)
target_compile_definitions("SokuAIOfflinePlayer" PRIVATE DIRECTINPUT_VERSION=0x0800 CURL_STATICLIB _CRT_SECURE_NO_WARNINGS $<$<CONFIG:Debug>:_DEBUG>)
target_link_libraries(SokuAIOfflinePlayer SokuLib shlwapi ws2_32)


add_library(
	NeuronAILib
	Training/AITrainerScriptCPP/GameInstance.cpp
	Training/AITrainerScriptCPP/GameInstance.hpp
	Training/AITrainerScriptCPP/Exceptions.hpp
	Training/AITrainerScriptCPP/GameManager.cpp
	Training/AITrainerScriptCPP/GameManager.hpp
	Training/AITrainerScriptCPP/BaseAi.cpp
	Training/AITrainerScriptCPP/BaseAi.hpp
	Training/AITrainerScriptCPP/SwissTournamentManager.cpp
	Training/AITrainerScriptCPP/SwissTournamentManager.hpp
	Training/AITrainerScriptCPP/Neuron.cpp
	Training/AITrainerScriptCPP/Neuron.hpp
	Training/AITrainerScriptCPP/ObjectsNeuron.cpp
	Training/AITrainerScriptCPP/ObjectsNeuron.hpp
	Training/AITrainerScriptCPP/NeuronAi.cpp
	Training/AITrainerScriptCPP/NeuronAi.hpp
	Training/AITrainerScriptCPP/Exceptions.cpp
	Training/AITrainerScriptCPP/GeneticAI.cpp Training/AITrainerScriptCPP/GeneticAI.hpp Training/AITrainerScriptCPP/GenNeuron.cpp Training/AITrainerScriptCPP/GenNeuron.hpp)
target_link_libraries(NeuronAILib ws2_32 SokuLib)

add_executable(
	NeuronAITrainer
	Training/AITrainerScriptCPP/main.cpp
)
target_link_libraries(NeuronAITrainer NeuronAILib)

add_executable(
	NeuronAIPlayer
	Training/AITrainerScriptCPP/player.cpp
)
target_link_libraries(NeuronAIPlayer NeuronAILib)
