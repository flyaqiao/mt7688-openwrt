# build testio executable when user executes "make"
APPNAME=kitedtu
SRCFILES := $(wildcard *.c)
SRCFILES += $(wildcard easylogger/src/*.c)
SRCFILES += $(wildcard easylogger/port/*.c)
SRCFILES += $(wildcard easylogger/plugins/file/*.c)
OBJFILES := $(SRCFILES:.c=.o)
CFLAGS_MONGOOSE += -DMG_ENABLE_PACKED_FS=1

INCLUDE = -I./easylogger/inc -I./easylogger/plugins/file

all: default

default: $(OBJFILES)
	$(CC) $(LDFLAGS) $(OBJFILES) -o $(APPNAME) -lmosquitto -lcurl

%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGS_MONGOOSE) $(INCLUDE) -c $< -o $@

# remove object files and executable when user executes "make clean"
clean:
	rm $(OBJFILES) $(APPNAME)
