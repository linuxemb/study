CC=gcc
CFLAGS = -g
CPPFLAGS += -MD
OBJS := main.o view.o algm.o
DEPS:= main.d view.d algm.d



exec: $(OBJS)
	gcc -o$@ $^

-include $(DEPS)

.PHONY : clean distclean

clean:
	@rm -f *.o
	@rm -f *.d

distclean: clean
	@rm exec

