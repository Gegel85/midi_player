cmake_minimum_required(VERSION 3.13)
project(midi_player C)

set(CMAKE_C_STANDARD 11)

include_directories(include)
include_directories(lib/concatf/include)
include_directories(lib/midi_parser/include)

add_library(
	concatf
	lib/concatf/include/concatf.h
	lib/concatf/include/flags.h
	lib/concatf/sources/concat.c
	lib/concatf/sources/concatf.c
	lib/concatf/sources/float_to_str.c
	lib/concatf/sources/get_nbrlen.c
	lib/concatf/sources/int_to_str.c
	lib/concatf/sources/my_compute_power_rec_long.c
	lib/concatf/sources/my_putbase.c
	lib/concatf/sources/my_putnbr_base.c
	lib/concatf/sources/my_putstar.c
	lib/concatf/sources/my_putstr_stderr.c
	lib/concatf/sources/my_showstr.c
	lib/concatf/sources/my_strlen_rec.c
	lib/concatf/sources/str.c
	lib/concatf/sources/sub_strings.c
	lib/concatf/sources/uint_to_str.c
)

add_library(
	midi_parser
	lib/midi_parser/include/midi_parser.h
	lib/midi_parser/src/midi.c
	lib/midi_parser/main.c
)

add_executable(
	midi_player
	include/header.h
	src/disp_keys.c
	src/disp_msg.c
	src/display_from_events.c
	src/display_from_notes.c
	src/globals.c
	src/main.c
	src/sounds.c
)

target_link_libraries(
	${PROJECT_NAME}
	midi_parser
	concatf
	m
	csfml-graphics
	csfml-audio
	csfml-system
)