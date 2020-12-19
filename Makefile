all: btracker.ssd btrack

btrack: $(wildcard src/*) Makefile
	beebasm -i src/main.asm -w

btracker.ssd: $(wildcard src/*) Makefile
	beebasm -i src/main.asm -do btracker.ssd -opt 3 -w
