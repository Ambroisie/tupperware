CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -Werror
CPPFLAGS = -Iinclude/ -Itests/ -D_DEFAULT_SOURCE
VPATH = src/ tests/

SRC = \
    src/avl.c \
    src/list.c \
    src/vector.c \

OBJS = $(SRC:.c=.o)


.PHONY: all
all: $(OBJS)

.PHONY: check
check: testsuite
	./testsuite --verbose

TEST_SRC = \
    tests/avl.c \
    tests/list.c \
    tests/testsuite.c \

TEST_OBJS = $(TEST_SRC:.c=.o)

testsuite: LDFLAGS+=-lcriterion -fsanitize=address
testsuite: CFLAGS+=-fsanitize=address
testsuite: CFLAGS+=-g
testsuite: $(OBJS) $(TEST_OBJS)

.PHONY: clean
clean:
	$(RM) $(OBJS)
	$(RM) $(TEST_OBJS)
	$(RM) testsuite
