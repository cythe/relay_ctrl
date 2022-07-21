.PHYON: all clean
TARGET=single_dog relay_ctrl relay_hub

all: $(TARGET)

Cross_compile=
CC=gcc

SRCS=${wildcard *.c}

OBJS:=${patsubst %.c, %.o, $(SRCS)}
%.o:%.c
	$(Cross_compile)$(CC) -g -c $< -o $@

single_dog: dictionary.o iniparser.o single_dog.o
	$(Cross_compile)$(CC) $^ -o $@

relay_hub: dictionary.o iniparser.o relay_hub.o
	$(Cross_compile)$(CC) $^ -o $@ -lpthread

relay_ctrl: dictionary.o iniparser.o relay_ctrl.o
	$(Cross_compile)$(CC) $^ -o $@

dist: relay_hub relay_ctrl
	rm -rf dist && mkdir dist
	mv $(TARGET) dist/
	cp -f *.ini dist/

clean:
	rm -rf dist $(OBJS) $(TARGET) 

distclean: clean
	rm -f GPATH GRTAGS GTAGS tags
