CFLAGS := -Wall -std=gnu99 -D_GNU_SOURCE -Os -s -I./ -I./ -fPIC

OBJ = mtdlib.o main.o flashmap.o

all: mtd_tool install

mtd_tool : $(OBJ)
	$(CC) -fPIC $(CFLAGS) $(LDFLAGS) -o $@ $^
	
%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@

install:
	cp mtd_tool /tmp/

clean:
	-rm -rf $(OBJ) mtd_tool

