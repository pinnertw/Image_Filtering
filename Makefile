SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj

CC=gcc
CFLAGS=-O3 -I$(HEADER_DIR)
LDFLAGS=-lm

SRC= dgif_lib.c \
	egif_lib.c \
	gif_err.c \
	gif_font.c \
	gif_hash.c \
	gifalloc.c \
	openbsd-reallocarray.c \
	quantize.c

OBJ= $(OBJ_DIR)/dgif_lib.o \
	$(OBJ_DIR)/egif_lib.o \
	$(OBJ_DIR)/gif_err.o \
	$(OBJ_DIR)/gif_font.o \
	$(OBJ_DIR)/gif_hash.o \
	$(OBJ_DIR)/gifalloc.o \
	$(OBJ_DIR)/openbsd-reallocarray.o \
	$(OBJ_DIR)/quantize.o

all: $(OBJ_DIR) sobelf sobelf_openmp

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

sobelf:$(OBJ)
	$(CC) $(CFLAGS) -DCLASSIC -o $@ src/main.c $^ $(LDFLAGS)

sobelf_openmp:$(OBJ) 
	$(CC) $(CFLAGS) -DOPENMP -fopenmp -o $@ src/main.c $^ $(LDFLAGS)

clean:
	rm -f sobelf $(OBJ) sobelf_openmp
