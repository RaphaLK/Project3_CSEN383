CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I.
TARGET = simulation
TEST_TARGET = test_lru

SOURCES = main.c lru.c random_pick.c
HEADERS = common.h

OBJECTS = $(SOURCES:.c=.o)

#gcc -o simulation main.c lru.c