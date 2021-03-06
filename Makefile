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
	gif_io.c \
	quantize.c \
	classic_filter.c

OBJ= $(OBJ_DIR)/dgif_lib.o \
	$(OBJ_DIR)/egif_lib.o \
	$(OBJ_DIR)/gif_err.o \
	$(OBJ_DIR)/gif_font.o \
	$(OBJ_DIR)/gif_hash.o \
	$(OBJ_DIR)/gifalloc.o \
	$(OBJ_DIR)/openbsd-reallocarray.o \
	$(OBJ_DIR)/gif_io.o \
	$(OBJ_DIR)/quantize.o \
	$(OBJ_DIR)/classic_filter.o

SRC_OPENMP = openmp_filter.c
OBJ_OPENMP = $(OBJ_DIR)/openmp_filter.o

all: $(OBJ_DIR) sobelf

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(OBJ_OPENMP) : $(SRC_DIR)/$(SRC_OPENMP)
	$(CC) $(CFLAGS) -fopenmp -c -o $@ $^ $(LDFLAGS)

sobelf:$(OBJ_OPENMP) $(OBJ)
	$(CC) $(CFLAGS) -fopenmp -o $@ src/main.c $^ $(LDFLAGS)

clean:
	rm -f sobelf $(OBJ) sobelf_openmp $(OBJ_OPENMP) $(OBJ_CLASSIC)
