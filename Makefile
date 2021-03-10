SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj

CC=gcc
GPUCC=nvcc
CFLAGS=-O3 -I$(HEADER_DIR)
LDFLAGS=-lm
#LDGPUFLAGS=-lm -L/usr/local/cuda-11.0.3/targets/x86_64-linux/lib -lcudart -lstdc++
LDGPUFLAGS=-lm -L/usr/local/cuda/lib64 -lcudart -lstdc++

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

obj/cuda_filter.o: src/cuda_filter.cu
	$(GPUCC) -Iinclude -c -o $@ $^


$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(OBJ_OPENMP) : $(SRC_DIR)/$(SRC_OPENMP)
	$(CC) $(CFLAGS) -fopenmp -c -o $@ $^ $(LDFLAGS)

sobelf:$(OBJ_OPENMP) $(OBJ)
	$(CC) $(CFLAGS) -fopenmp -o $@ src/main.c $^ $(LDFLAGS)

sobelf_gpu:$(OBJ_OPENMP) $(OBJ) obj/cuda_filter.o
	$(CC) $(CFLAGS) -fopenmp -o $@ src/main.c $^ $(LDGPUFLAGS)

clean:
	rm -f sobelf $(OBJ) sobelf_openmp $(OBJ_OPENMP) $(OBJ_CLASSIC)
