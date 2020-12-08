all: btracker.ssd

btracker.ssd: src/main.asm Makefile
	beebasm -i src/main.asm -do $@ -opt 2 -w
