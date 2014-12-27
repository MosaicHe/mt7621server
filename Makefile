
EXEC = server
MODULESET = module_set
MODULEGET = module_get

#CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/char
#CFLAGS	+= -I$(ROOTDIR)/lib/libnvram
#LDLIBS	+= -lnvram

all: $(EXEC) $(MODULESET) $(MODULEGET)  moduletest

$(EXEC): $(EXEC).c  table.c tool.c handleweb.c workthread.c pingthread.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lpthread

$(MODULESET): moduleset.c module.c
	$(CC) -o module_set moduleset.c module.c

$(MODULEGET): moduleget.c module.c
	$(CC) -o module_get moduleget.c module.c

moduletest:	test.c module.c
	$(CC) -o moduletest test.c module.c

romfs:
	$(ROMFSINST) /bin/$(EXEC)
	$(ROMFSINST) /bin/$(MODULEGET)
	$(ROMFSINST) /bin/$(MODULESET)
	$(ROMFSINST) /bin/moduletest	

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o *~ $(MODULESET) $(MODULEGET)
