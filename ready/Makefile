all:
	mkdir bin
	gcc source/main.c -o bin/main -Wall -Werror -lm -fsanitize=undefined -fsanitize=address
	cpplint --filter=-legal/copyright source/main.c
%: %.c
	gcc $@.c -o $@ -Wall -Werror -lm -fsanitize=undefined -fsanitize=address
	cpplint --filter=-legal/copyright $@.c
clear:
	rm bin/main
	rmdir bin
