EXEC = server
#CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/char
#CFLAGS	+= -I$(ROOTDIR)/lib/libnvram
#LDLIBS	+= -lnvram

all: $(EXEC)

$(EXEC): $(EXEC).c  table.c tool.c udthread.c workthread.c pingthread.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lpthread

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o *~
