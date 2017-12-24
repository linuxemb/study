include gmd

#CWD= $(warning Call to shell)$(shell pwd)  # many times
CWD:=$(warning call to shell)$(shell pwd)
SRC_DIR = $(CWD)/src/
OBJ_DIR = $(CWD)/obj/
OBJS= $(OBJ_DIR)foo.o $(OBJ_DIR)bar.o $(OBJ_DIR)baz.o
vpath %.c $(SRC_DIR)
$(OBJ_DIR)%.o: $(SRC_DIR)%.c 
	@echo Make $@ from $<
	gcc  -c   $@ $<

exec: $(OBJS)
	@echo $? $(OBJS)
	gcc -o $@  $^
 
print-%:
	@echo $*=$($*) from $(origin $*)
 
#iCWD:=$(waring Call to shell)$(shell pwd)
clean:
	rm -rf $(OBJ_DIR)*.o
	rm -rf exec 
$(__BREAKPOINT)

