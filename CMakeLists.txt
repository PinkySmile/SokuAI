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
target_compile_options(SokuAITrainer PRIVATE /Zi)
target_compile_definitions("SokuAITrainer" PRIVATE DIRECTINPUT_VERSION=0x0800 CURL_STATICLIB _CRT_SECURE_NO_WARNINGS $<$<CONFIG:Debug>:_DEBUG>)
target_include_directories("SokuAITrainer" PRIVATE include include/directx "modules/AdvancedPracticeMode/" "${SFML_DIR}/include" "${TGUI_DIR}/include")
target_link_libraries(SokuAITrainer SokuLib shlwapi ws2_32)