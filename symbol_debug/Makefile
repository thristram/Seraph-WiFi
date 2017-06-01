all:

	gcc -m32 -Wall -g -DAR6002 symbol.c -o symbol
	xt-nm -ln --defined-only ../.output/QCA4004/SP143/image/sdk_shell.out > sdk_shell.map 2>/dev/null

_all:
	@echo "nothing to do with _all"
clean:
	rm symbol *.map
