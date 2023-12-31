#
# Students' Makefile for the Malloc Lab
#
TEAM = bovik
VERSION = 1
HANDINDIR = /afs/cs.cmu.edu/academic/class/15213-f01/malloclab/handin

CC = clang
CFLAGS = -Wall -Wextra -g -m32 -DDRIVER -std=gnu99 -Wno-unused-function -Wno-unused-parameter
# CFLAGS = -Wall -Wextra -O3 -g -DDRIVER -std=gnu99 -Wno-unused-function -Wno-unused-parameter -DDEBUG

OBJS = mdriver.o mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o

mdriver: $(OBJS)
	$(CC) $(CFLAGS) -o mdriver $(OBJS)

# mdriver.o: mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
# memlib.o: memlib.c memlib.h
# mm.o: mm.c mm.h memlib.h
# fsecs.o: fsecs.c fsecs.h config.h
# fcyc.o: fcyc.c fcyc.h
# ftimer.o: ftimer.c ftimer.h config.h
# clock.o: clock.c clock.h
%.o: %.c 
	$(CC) $(CFLAGS) -c $< -MD

handin:
	cp mm.c $(HANDINDIR)/$(TEAM)-$(VERSION)-mm.c

clean:
	rm -f *~ *.o *.d mdriver compile_commands.json



# -include $(OBJS:.o=.d)