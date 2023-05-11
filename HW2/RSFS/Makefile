CC = gcc
LDLIBS = -lpthread
CFLAGS = -g -Wall

objects = api.o application_4.o data_block.o dir.o inode.o open_file_table.o
App = app

all: $(App)

$(App): $(objects)
	$(CC) -o $(App) $(objects) $(CFLAGS) $(LDLIBS) -g -Wall

$(objects): %.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o app