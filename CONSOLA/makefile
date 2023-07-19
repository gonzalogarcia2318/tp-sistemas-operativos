PROCESS_NAME=CONSOLA
IDIR=../CONSOLA/include
SHARED_IDIR=../shared/include
CC=gcc
CFLAGS=-I$(IDIR) -I$(SHARED_IDIR) -g -Wall -std=c11 -D_POSIX_C_SOURCE=200112L
ODIR=./obj
SRCDIR=./src
LIBS=-lcommons -lpthread -lreadline -lcunit -lrt -lresolv

LDFLAGS=-L../shared -Wl,-rpath,../shared

SHARED_SRCDIR=../shared/src
SHARED_ODIR=../shared/obj
SHARED_DEPS=$(wildcard $(SHARED_IDIR)/*.h)
SHARED_SRC=$(wildcard $(SHARED_SRCDIR)/*.c)
SHARED_OBJ=$(patsubst $(SHARED_SRCDIR)/%.c,$(SHARED_ODIR)/%.o,$(SHARED_SRC))

DEPS=$(wildcard $(IDIR)/*.h)
SRC=$(wildcard $(SRCDIR)/*.c)
OBJ=$(patsubst $(SRCDIR)/%.c,$(ODIR)/%.o,$(SRC))

$(ODIR)/%.o: $(SRCDIR)/%.c $(DEPS) $(SHARED_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(SHARED_ODIR)/%.o: $(SHARED_SRCDIR)/%.c $(SHARED_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(PROCESS_NAME): $(OBJ) $(SHARED_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

shared: $(SHARED_OBJ)
	$(CC) -shared -o ../shared/libcommons.so $^

.PHONY: all clean run

all: $(PROCESS_NAME)

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ $(PROCESS_NAME) $(SHARED_ODIR)/*.o ../shared/libcommons.so

run: all
	./$(PROCESS_NAME) config/consola.config ./base1.txt

runb2: all
	./$(PROCESS_NAME) config/consola.config ./base2.txt

runb3: all
	./$(PROCESS_NAME) config/consola.config ./base3.txt

rund1: all
	./$(PROCESS_NAME) config/consola.config ./deadlock1.txt

rund2: all
	./$(PROCESS_NAME) config/consola.config ./deadlock2.txt

rund3: all
	./$(PROCESS_NAME) config/consola.config ./deadlock3.txt

rund4: all
	./$(PROCESS_NAME) config/consola.config ./deadlock4.txt

rune1: all
	./$(PROCESS_NAME) config/consola.config ./error1.txt

rune2: all
	./$(PROCESS_NAME) config/consola.config ./error2.txt

runs1: all
	./$(PROCESS_NAME) config/consola.config ./create_segment.txt

runfs: all
	./$(PROCESS_NAME) config/consola.config ./fileSystem.txt

runm1: all
	./$(PROCESS_NAME) config/consola.config ./MEMORIA_1.txt

runm2: all
	./$(PROCESS_NAME) config/consola.config ./MEMORIA_2.txt
	
runm3: all
	./$(PROCESS_NAME) config/consola.config ./MEMORIA_3.txt

runfs1: all
	./$(PROCESS_NAME) config/consola.config ./FS_1.txt

runfs2: all
	./$(PROCESS_NAME) config/consola.config ./FS_2.txt

runfs3: all
	./$(PROCESS_NAME) config/consola.config ./FS_3.txt

run-args: all
	./${PROCESS_NAME} $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))