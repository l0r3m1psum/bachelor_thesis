.POSIX:
.PHONY: all clean profile debug release

WARNINGS = -Wall -Wextra -Wstrict-prototypes -Wconversion -pedantic -pedantic-errors -Wstrict-aliasing
DEBUG = -O0 -ftrapv -fsanitize=address,undefined -fstack-protector-all -fstrict-aliasing -fno-omit-frame-pointer -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2
RELEASE = -O3 -flto -DNDEBUG
CC = clang
CFLAGS = $(WARNINGS) -std=c17 -march=native -mtune=native -g
LDFLAGS = -lc -lm
CFILES = simulator.c csv.c main.c

all: debug
debug:
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEBUG) $(CFILES) -o main
release:
	$(CC) $(CFLAGS) $(LDFLAGS) $(RELEASE) $(CFILES) -o main
clean:
	rm -f main.o csv.o simulator.o simulator csv
profile:
	caffeinate -d time main ../../res/general_params.csv ../../res/turano_cells.csv ../../res/initial_state.csv ../../res/
	xcrun xctrace record --template MyTemplate --time-limit 20s --window 10s --attach main
