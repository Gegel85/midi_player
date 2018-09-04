NAME =	midi_player

SRC = 	display_from_events.c	\
	display_from_notes.c	\
	disp_keys.c		\
	globals.c		\
	main.c			\
	sounds.c		\

OBJ =	$(SRC:%.c=src/%.o)

INC =	-Iinclude			\
	-Ilib/midi_parser/include	\

CSFML = -lcsfml-audio		\
	-lcsfml-graphics	\
	-lcsfml-network		\
	-lcsfml-system		\
	-lcsfml-window		\


LDFLAGS =		\
	-lm		\

CFLAGS= $(INC)		\
	-W		\
	-Wall		\
	-Wextra		\

CC =	gcc

RULE =	all

LIBS =	lib/midi_parser/midiparser.a	\

all:	$(LIBS) $(NAME)

lib/midi_parser/midiparser.a:
	$(MAKE) -C lib/midi_parser $(RULE)


$(NAME):$(OBJ)
	$(CC) -o $(NAME) $(OBJ) $(LDFLAGS) $(CSFML) $(LIBS)

clean:
	$(MAKE) -C lib/midi_parser clean
	$(RM) $(OBJ)

fclean:	clean
	$(MAKE) -C lib/midi_parser fclean
	$(RM) $(NAME) $(NAME).exe

ffclean:fclean

re:	fclean all

dbg:	CFLAGS += -g -O0
dbg:	RULE = dbg
dbg:	fclean all

epi:	CSFML = -lc_graph_prog
epi:	CFLAGS += -g -O0
epi:	RULE = dbg
epi:	re
