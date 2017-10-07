exec:main.o view.o algm.o
	gcc -oexec main.o view.o algm.o

main.o:main.c

view.o:view.c view.h

algm.o:algm.c algm.h

.PHONY : clean distclean

clean:
	@rm -f *.o

distclean: clean
	@rm exec
