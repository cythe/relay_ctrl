.PHYON: all clean
TARGET=single_dog relay_ctrl relay_hub

all: $(TARGET)

Cross_compile=
CC=gcc

SRCS=${wildcard *.c}

OBJS:=${patsubst %.c, %.o, $(SRCS)}
%.o:%.c
	$(Cross_compile)$(CC) -c $< -o $@

single_dog: dictionary.o iniparser.o main.o
	$(Cross_compile)$(CC) $^ -o $@

relay_hub: dictionary.o iniparser.o relay_hub.o
	$(Cross_compile)$(CC) $^ -o $@ -lpthread

relay_ctrl: dictionary.o iniparser.o relay_ctrl.o
	$(Cross_compile)$(CC) $^ -o $@


clean:
	rm -f $(OBJS) $(TARGET)

distclean: clean
	rm -f GPATH GRTAGS GTAGS tags
