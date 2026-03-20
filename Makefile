SOURCE_FILES = src/main.c src/controller_management.c

WARNING_FLAGS = -Wall -Wextra -Werror

OPTIMIZATION_FLAGS = -O2

switch2xbox: $(SOURCE_FILES)
	$(CC) $(WARNING_FLAGS) $(OPTIMIZATION_FLAGS) $(SOURCE_FILES) -levdev -lpthread -o switch2xbox

clean:
	rm switch2xbox

.PHONY: clean
