cmake_minimum_required(VERSION 3.15)
project(SokuAI)

set(CMAKE_CXX_STANDARD 20)

include_directories(SokuLib/directx)
add_compile_definitions("$<$<CONFIG:Debug>:_DEBUG>$<$<CONFIG:Release>:NDEBUG>")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

	# SokuLib
	add_subdirectory(SokuLib)

	message("${TRAINING_FOLDER}")
	add_library(
		SokuAITrainer
		MODULE
		Training/TrainingMod/Packet.hpp
		Training/TrainingMod/main.cpp
		Training/TrainingMod/state.cpp
		Training/TrainingMod/state.hpp
		Training/TrainingMod/network.cpp
		Training/TrainingMod/network.hpp
		Training/TrainingMod/sokuData.cpp
		Training/TrainingMod/sokuData.hpp
	)
	target_compile_definitions("SokuAITrainer" PRIVATE DIRECTINPUT_VERSION=0x0800 CURL_STATICLIB _CRT_SECURE_NO_WARNINGS $<$<CONFIG:Debug>:_DEBUG>)
	target_link_libraries(SokuAITrainer SokuLib shlwapi ws2_32)

	add_library(
		SokuOnlinePlayer
		MODULE
		OnlineHostMod/main.cpp
		OnlineHostMod/Network/Socket.cpp
		OnlineHostMod/Network/Socket.hpp
		Training/AITrainerScriptCPP/GeneticAI.cpp
		Training/AITrainerScriptCPP/GeneticAI.hpp
		Training/AITrainerScriptCPP/GenNeuron.cpp
		Training/AITrainerScriptCPP/GenNeuron.hpp
		Training/AITrainerScriptCPP/Neuron.cpp
		Training/AITrainerScriptCPP/Neuron.hpp
		Training/AITrainerScriptCPP/BaseAi.cpp
		Training/AITrainerScriptCPP/BaseAi.hpp
		Training/AITrainerScriptCPP/ObjectsNeuron.cpp
		Training/AITrainerScriptCPP/ObjectsNeuron.hpp
	)
	target_include_directories("SokuOnlinePlayer" PRIVATE Training/AITrainerScriptCPP)
	target_compile_definitions("SokuOnlinePlayer" PRIVATE DIRECTINPUT_VERSION=0x0800 CURL_STATICLIB _CRT_SECURE_NO_WARNINGS $<$<CONFIG:Debug>:_DEBUG> TRAINING_FOLDER="${TRAINING_FOLDER}")
	target_link_libraries(SokuOnlinePlayer SokuLib shlwapi ws2_32)

	add_library(
		SokuAIOfflinePlayer
		MODULE
		Training/OfflineVersusMod/Packet.hpp
		Training/OfflineVersusMod/main.cpp
	)
	target_compile_definitions("SokuAIOfflinePlayer" PRIVATE DIRECTINPUT_VERSION=0x0800 CURL_STATICLIB _CRT_SECURE_NO_WARNINGS $<$<CONFIG:Debug>:_DEBUG>)
	target_link_libraries(SokuAIOfflinePlayer SokuLib shlwapi ws2_32)
endif()


add_library(
	NeuronAILib
	Training/AITrainerScriptCPP/GameInstance.cpp
	Training/AITrainerScriptCPP/GameInstance.hpp
	Training/AITrainerScriptCPP/Exceptions.hpp
	Training/AITrainerScriptCPP/GameManager.cpp
	Training/AITrainerScriptCPP/GameManager.hpp
	Training/AITrainerScriptCPP/BaseAi.cpp
	Training/AITrainerScriptCPP/BaseAi.hpp
	Training/AITrainerScriptCPP/TournamentManager.cpp
	Training/AITrainerScriptCPP/TournamentManager.hpp
	Training/AITrainerScriptCPP/Neuron.cpp
	Training/AITrainerScriptCPP/Neuron.hpp
	Training/AITrainerScriptCPP/ObjectsNeuron.cpp
	Training/AITrainerScriptCPP/ObjectsNeuron.hpp
	Training/AITrainerScriptCPP/NeuronAi.cpp
	Training/AITrainerScriptCPP/NeuronAi.hpp
	Training/AITrainerScriptCPP/Exceptions.cpp
	Training/AITrainerScriptCPP/GeneticAI.cpp
	Training/AITrainerScriptCPP/GeneticAI.hpp
	Training/AITrainerScriptCPP/GenNeuron.cpp
	Training/AITrainerScriptCPP/GenNeuron.hpp
	Training/AITrainerScriptCPP/winDefines.hpp
)
if (NOT WIN32)
	target_compile_definitions(NeuronAILib PRIVATE SOKULIB_NO_MOD)
endif()
target_include_directories(NeuronAILib PRIVATE SokuLib/src)
if (WIN32)
	target_link_libraries(NeuronAILib ws2_32)
endif()

add_executable(
	NeuronAITrainer
	Training/AITrainerScriptCPP/main.cpp
)
if (NOT WIN32)
	target_compile_definitions(NeuronAITrainer PRIVATE SOKULIB_NO_MOD)
endif()
target_include_directories(NeuronAITrainer PRIVATE SokuLib/src)
target_link_libraries(NeuronAITrainer NeuronAILib)

add_executable(
	NeuronAIPlayer
	Training/AITrainerScriptCPP/player.cpp
)
if (NOT WIN32)
	target_compile_definitions(NeuronAIPlayer PRIVATE SOKULIB_NO_MOD)
endif()
target_include_directories(NeuronAIPlayer PRIVATE SokuLib/src)
target_link_libraries(NeuronAIPlayer NeuronAILib)
