# Корневой CMake-файл проекта kbdex.

cmake_minimum_required(VERSION 3.2)

project(
	kbdex
	DESCRIPTION "Программа для автоматического переключения раскладки клавиатуры на основе набираемого текста."
	VERSION 0.1.5
	LANGUAGES CXX C
)

option(MODE_COMMUNICATION_CHECK "Активация опции проверки связи между демонами. Все остальные функции программы отключены!" OFF)

# Генерация основного кофигурационного файла программы config.h из config.h.in
configure_file(cmake/kbdex_config.h.in ${CMAKE_BINARY_DIR}/kbdex_config.h @ONLY)

add_subdirectory(SourceCode)