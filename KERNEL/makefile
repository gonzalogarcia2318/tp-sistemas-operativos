PROCESS_NAME=KERNEL
IDIR =./include
SHARED_IDIR = ../shared/include
CC=gcc
CFLAGS=-I$(IDIR) -I$(SHARED_IDIR) -g

ODIR=./obj
SRCDIR =./src
LIBS=-lcommons -lpthread -lreadline -lcunit -lrt

SHARED_SRCDIR = ../shared/src
SHARED_ODIR = ../shared/obj

DEPS = $(wildcard $(IDIR)/*.h)

SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c,$(ODIR)/%.o,$(SRC))

$(ODIR)/%.o: $(SRCDIR)/%.c $(DEPS) $(SHARED_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(SHARED_ODIR)/%.o: $(SHARED_SRCDIR)/%.c $(SHARED_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

SHARED_DEPS = $(wildcard $(SHARED_IDIR)/*.h)

SHARED_SRC = $(wildcard $(SHARED_SRCDIR)/*.c)
SHARED_OBJ = $(patsubst $(SHARED_SRCDIR)/%.c,$(SHARED_ODIR)/%.o,$(SHARED_SRC))

$(PROCESS_NAME): $(OBJ) $(SHARED_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: all clean run

all: $(PROCESS_NAME)

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ $(PROCESS_NAME) $(SHARED_ODIR)/*.o ../shared/libcommons.so

run: all
	./$(PROCESS_NAME) config/kernel_base.config

runhrrn: all
	./$(PROCESS_NAME) config/kernel_base_hrrn.config

rund: all
	./$(PROCESS_NAME) config/kernel_deadlock.config

rune: all
	./$(PROCESS_NAME) config/kernel_error.config

runm: all
	./$(PROCESS_NAME) config/kernel_memoria.config

runfs: all
	./$(PROCESS_NAME) config/kernel_fs.config