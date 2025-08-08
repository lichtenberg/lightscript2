

OBJS = lsmain.o  lightscript.yy.o  tokenstream.o parser.o symtab.o schedule.o musicplayer.o playback.o

#CFLAGS = -fsanitize=address -O1 -Wall -Werror -target x86_64-apple-macos10.13 
CFLAGS =  -Wall -Werror -target x86_64-apple-macos10.13 


%.o : %.c
	clang $(CFLAGS) -c -o $@ $<

%.o : %.cpp
	clang $(CFLAGS) -c -o $@ $<

%.o : %.mm
	clang $(CFLAGS) -c -o $@ $<

lightscript : $(OBJS)
	clang $(CFLAGS) -o $@ $(OBJS) -lstdc++ -framework Foundation -framework AVFoundation
	codesign -s mlichtenberg@me.com lightscript

lightscript.yy.c : lightscript.lex lstokens.h lsinternal.h
	flex -DECHO -o lightscript.yy.c lightscript.lex

lsmain.o : lsmain.cpp lsinternal.h lstokens.h tokenstream.hpp symtab.hpp schedule.hpp

tokenstream.o : tokenstream.cpp lstokens.h tokenstream.hpp lsinternal.h

parser.o : parser.cpp lsinternal.h parser.hpp symtab.hpp

symtab.o : symtab.cpp lsinternal.h symtab.hpp

schedule.o : schedule.cpp symtab.hpp schedule.hpp

musicplayer.o : musicplayer.mm musicplayer.h


clean :
	rm -f lightscript $(OBJS) lightscript.yy.c 

zip :
	zip ../lightscript$(shell date "+%Y%m%d") lightscript lightscript.cfg panel.cfg laser.cfg lasertest*.ls2
