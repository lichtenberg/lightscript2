

OBJS = lsmain.o  lightscript.yy.o  tokenstream.o

CFLAGS = -Wall -Werror -target x86_64-apple-macos10.13 
#CFLAGS =

%.o : %.c
	clang $(CFLAGS) -c -o $@ $<

%.o : %.cpp
	clang $(CFLAGS) -c -o $@ $<

%.o : %.mm
	clang $(CFLAGS) -c -o $@ $<

lightscript : $(OBJS)
	clang $(CFLAGS) -o $@ $(OBJS) -lstdc++ -framework Foundation -framework AVFoundation
	codesign -s mlichtenberg@me.com lightscript

lightscript.yy.c : lightscript.lex lstokens.h lightscript.h
	flex -DECHO -o lightscript.yy.c lightscript.lex

lsmain.o : lsmain.cpp lightscript.h lstokens.h


clean :
	rm -f lightscript $(OBJS) lightscript.yy.c 

zip :
	zip ../lightscript$(shell date "+%Y%m%d") lightscript *.cfg
