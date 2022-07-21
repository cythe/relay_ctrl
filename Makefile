.PHYON: all clean
TARGET=relay_ctrl relay_hub

all: dist

Cross_compile=
CC=gcc
CFLAGS=-g

SRCS=${wildcard src/*.c}

OBJS:=${patsubst %.c, %.o, $(SRCS)}
%.o:%.c
	@echo ${SRCS}
	@echo ${OBJS}
	$(Cross_compile)$(CC) ${CFLAGS} -c $< -o $@

relay_hub: src/dictionary.o src/iniparser.o src/relay_hub.o
	$(Cross_compile)$(CC) $^ -o $@ -lpthread

relay_ctrl: src/dictionary.o src/iniparser.o src/relay_ctrl.o
	$(Cross_compile)$(CC) $^ -o $@

dist: relay_hub relay_ctrl
	rm -rf dist && mkdir dist
	mv $(TARGET) dist/
	cp -f conf/*.ini dist/

clean:
	rm -rf dist $(OBJS) $(TARGET) 

distclean: clean
	rm -f GPATH GRTAGS GTAGS tags
