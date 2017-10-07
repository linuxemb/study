OBJS := view.o algm.o
DEPS:= view.d algm.d
CFLAGS +=-I../include
lib.a:$(OBJS)
	$(AR) rcs $@ $(OBJS)

-include $(DEPS)

.PHONY : clean distclean

clean:
	@rm -f *.o
	@rm -f *.d
	@rm -f lib.a

