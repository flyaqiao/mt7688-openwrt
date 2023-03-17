# build testio executable when user executes "make"
APPNAME=kitedtu
SRCFILES := $(wildcard *.c)
SRCFILES += $(wildcard easylogger/src/*.c)
SRCFILES += $(wildcard easylogger/port/*.c)
SRCFILES += $(wildcard easylogger/plugins/file/*.c)
OBJFILES := $(SRCFILES:.c=.o)

INCLUDE = -I./easylogger/inc -I./easylogger/plugins/file

all: default

default: $(OBJFILES)
	$(CC) $(LDFLAGS) $(OBJFILES) -o $(APPNAME) -lmosquitto -lcurl

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

# remove object files and executable when user executes "make clean"
clean:
	rm $(OBJFILES) $(APPNAME)
