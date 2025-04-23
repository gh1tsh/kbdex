# Корневой CMake-файл проекта kbdex.

cmake_minimum_required(VERSION 3.2)

project(
	kbdex
	DESCRIPTION "Программа для автоматического переключения раскладки клавиатуры на основе набираемого текста."
	VERSION 0.1.2
	LANGUAGES CXX C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Генерация основного кофигурационного файла программы config.h из config.h.in
configure_file(cmake/kbdex_config.h.in ${CMAKE_BINARY_DIR}/kbdex_config.h @ONLY)

# Включаем директорию с артефактами CMake в список include-директорий
include_directories(${CMAKE_BINARY_DIR})