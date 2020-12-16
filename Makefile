all: btracker.ssd

btracker.ssd: $(wildcard src/*) Makefile
	beebasm -i src/main.asm -do btracker.ssd -opt 2 -w
