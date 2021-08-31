.POSIX:
.PHONY: all clean profile

WARNINGS = -Wall -Wextra -Wstrict-prototypes -Wconversion -pedantic -pedantic-errors -Wstrict-aliasing
DEBUG = -O0 -ftrapv -fsanitize=address,undefined -fstack-protector-all -fstrict-aliasing -fno-omit-frame-pointer -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2
RELEASE = -O3 -flto -DNDEBUG
CC = clang
CFLAGS = $(WARNINGS) $(DEBUG) -std=c17 -march=native -mtune=native -g

all: main.c csv.c
	$(CC) $(CFLAGS) csv.c main.c -o main
clean:
	rm -f main.o csv.o
profile: main
	xcrun xctrace record --template "Time Profiler" --output $(*).trace --launch -- $*
