SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj

CC=gcc
GPUCC=nvcc
MPICC=mpicc
CFLAGS=-O3 -I$(HEADER_DIR)
LDFLAGS=-lm
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
	$(OBJ_DIR)/classic_filter.o \
	$(OBJ_DIR)/openmp_filter.o \
	$(OBJ_DIR)/mpi_filter.o \
	$(OBJ_DIR)/cuda_filter.o

all: $(OBJ_DIR) sobelf

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/cuda_filter.o: $(SRC_DIR)/cuda_filter.cu
	$(GPUCC) -Iinclude -c -o $@ $^

$(OBJ_DIR)/openmp_filter.o : $(SRC_DIR)/openmp_filter.c
	$(CC) $(CFLAGS) -fopenmp -c -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/mpi_filter.o : $(SRC_DIR)/mpi_filter.c
	$(MPICC) $(CFLAGS) -fopenmp -c -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

sobelf: $(OBJ) 
	$(MPICC) $(CFLAGS) -fopenmp -o $@ src/main.c $^ $(LDGPUFLAGS)

clean:
	rm -f sobelf sobelf_serial sobelf_mpi $(OBJ)
