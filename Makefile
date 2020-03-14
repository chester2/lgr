CC = gcc
CFLAGS = -Wall -Werror=vla -Wextra -Wpedantic -std=c99 -Iinc -lm -g3 -ggdb
CFLAGS_TEST = $(CFLAGS) -Itest
INC = inc
BIN = bin
OBJ = obj
SRC = src
TEST = test


MODULES = util date hashtable record recordlist recordtree program
MODULES_O = $(MODULES:%=$(OBJ)/%.o)
MODULES_O_TEST = $(MODULES_O) $(TEST)/t_framework.h $(TEST)/t_refrecs.h

$(OBJ)/%.o: $(SRC)/%.c $(INC)/%.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

sb: $(TEST)/sb.c $(MODULES_O)
	mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $(BIN)/$@

t: $(MODULES:%=t%)
t%: $(TEST)/t%.c $(MODULES_O_TEST)
	mkdir -p $(BIN)
	$(CC) $(CFLAGS_TEST) $^ -o $(BIN)/$@


COMMANDS = init log view rm sum plot lim
COMMANDS_C = $(COMMANDS:%=$(SRC)/_lgr_%.c)

lgr: $(SRC)/_lgr.c $(COMMANDS_C) $(MODULES_O)
	mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $(BIN)/$@

.PHONY: clean
clean:
	rm -f obj/*.o bin/*