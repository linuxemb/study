include ../gmd
CC=gcc
ifeq ($(DEBUG), y)
CFLAGS:=-g
else
CFLAGS:=
endif
CPPFLAGS += -MD
SRC := $(wildcard *.c)
OBJS :=$(patsubst %.c,%.o,$(SRC))
DEPS:= $(patsubst %.o,%.d,$(OBJS))
LIBS:= 
EXEC:=exec


$(EXEC): $(OBJS)
	gcc -o$@ $^ $(addprefix -l,$(LIBS))

-include $(DEPS)

.PHONY : clean distclean

clean:
	@rm -f *.o
	@rm -f *.d

distclean: clean
	@rm exec
$(__BREAKPOINT)
