TARGETS = minesweeper_server minesweeper_client

all: $(TARGETS)

LIBS=-lm -lpthread
DIRS=
FLAGS=-std=gnu99 -Wall -g

clean:
	for f in $(TARGETS); do \
		if [ -f $$f ]; then rm $$f; fi; \
		if [ -f $$f.exe ]; then rm $$f.exe; fi; \
done

rebuild: clean all

minesweeper_server: minesweeper_server.c
			gcc $(FLAGS) $@.c -o $@ $(DIRS) $(LIBS)

minesweeper_client: minesweeper_client.c
		gcc $(FLAGS) $@.c -o $@ $(DIRS) $(LIBS)
