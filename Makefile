

MAINOBJS = lsmain.o
TESTOBJS = apitest.o
OBJS = lightscript.yy.o  tokenstream.o parser.o symtab.o schedule.o playback.o
TESTOBJS += lightscript_api.o

#CFLAGS = -fsanitize=address -O1 -Wall -Werror -target x86_64-apple-macos10.13 
CFLAGS =  -g -Wall -Werror
#CFLAGS += -O1 -fsanitize=address -fno-omit-frame-pointer
#CFLAGS += -target x86_64-apple-macos10.13 


%.o : %.c
	clang $(CFLAGS) -c -o $@ $<

%.o : %.cpp
	clang $(CFLAGS) -c -o $@ $<

%.o : %.mm
	clang $(CFLAGS) -c -o $@ $<

all : lightscript apitest
	echo done

lightscript : $(MAINOBJS) $(OBJS)
	clang $(CFLAGS) -o $@ $(MAINOBJS) $(OBJS) -lstdc++ -framework Foundation -framework AVFoundation
	codesign -s mlichtenberg@me.com lightscript

apitest : $(TESTOBJS) $(OBJS)
	clang $(CFLAGS) -o $@ $(TESTOBJS) $(OBJS) -lstdc++ -framework Foundation -framework AVFoundation
	codesign -s mlichtenberg@me.com apitest

lightscript.yy.c : lightscript.lex lstokens.h lsinternal.h
	flex -DECHO -o lightscript.yy.c lightscript.lex

lsmain.o : lsmain.cpp lsinternal.h lstokens.h tokenstream.hpp symtab.hpp schedule.hpp

lightscript_api.o : lightscript_api.cpp lsinternal.h lstokens.h tokenstream.hpp symtab.hpp schedule.hpp

tokenstream.o : tokenstream.cpp lstokens.h tokenstream.hpp lsinternal.h

parser.o : parser.cpp lsinternal.h parser.hpp symtab.hpp

playback.o : playback.mm lsinternal.h schedule.hpp parser.hpp symtab.hpp playback.h

symtab.o : symtab.cpp lsinternal.h symtab.hpp

schedule.o : schedule.cpp symtab.hpp schedule.hpp

# musicplayer.o : musicplayer.mm musicplayer.h


clean :
	rm -f lightscript $(OBJS) $(MAINOBJS) $(TESTOBJS) lightscript.yy.c 

zip :
	zip ../lightscript$(shell date "+%Y%m%d") lightscript lightscript.cfg panel.cfg laser.cfg lasertest*.ls2
