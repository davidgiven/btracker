all: btracker.ssd

btracker.ssd: $(wildcard src/*) Makefile
	beebasm -i src/main.asm -opt 2 -w
