# Корневой CMake-файл проекта kbdex.

cmake_minimum_required(VERSION 3.2)

project(
	kbdex
	DESCRIPTION "Программа для автоматического переключения раскладки клавиатуры на основе набираемого текста."
	VERSION 0.1.2
	LANGUAGES CXX C
)

# Генерация основного кофигурационного файла программы config.h из config.h.in
configure_file(cmake/kbdex_config.h.in ${CMAKE_BINARY_DIR}/kbdex_config.h @ONLY)

add_subdirectory(SourceCode)