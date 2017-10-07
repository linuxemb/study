AR=ar
CC=gcc
CFLAGS = -g
export AR CC CFLAGS 

OBJS=main.o
LIBS= lib/lib.a
LIBDIRS=lib

exec: $(OBJS) $(LIBDIRS)
	gcc -o$@ $(OBJS) $(LIBS)




.PHONY : $(LIBDIRS) clean distclean

$(LIBDIRS):
	$(MAKE) -C $@

clean:
	@rm -f *.o
	$(MAKE) -C $(LIBDIRS) clean

distclean: clean
	@rm -f exec

