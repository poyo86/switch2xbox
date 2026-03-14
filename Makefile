WARNING_FLAGS = -Wall -Wextra -Werror

OPTIMIZATION_FLAGS = -O2

switch2xbox: src/main.c
	$(CC) $(WARNING_FLAGS) $(OPTIMIZATION_FLAGS) src/main.c -levdev -lpthread -o switch2xbox

clean:
	rm switch2xbox

.PHONY: clean
