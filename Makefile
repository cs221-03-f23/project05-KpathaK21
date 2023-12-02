OBJS = project05.o 
PROG = project05

%.o : %.c
	gcc -c -g -o $@ $<

$(PROG) : $(OBJS)
	gcc -g -o $@ $^

clean:
	rm -rf $(OBJS) $(PROG)
