EXEC = server
#CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/char
#CFLAGS	+= -I$(ROOTDIR)/lib/libnvram
#LDLIBS	+= -lnvram

all: $(EXEC) module_set module_get

$(EXEC): $(EXEC).c  table.c tool.c handleweb.c workthread.c pingthread.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lpthread

module_set: moduleset.c module.c
	$(CC) -o module_set moduleset.c module.c

module_get: moduleget.c module.c
	$(CC) -o module_get moduleget.c module.c


romfs:
	$(ROMFSINST) /bin/$(EXEC)
	$(ROMFSINST) /bin/module_set
	$(ROMFSINST) /bin/module_get

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o *~
