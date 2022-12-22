# build testio executable when user executes "make"
APPNAME=kitedtu
SRCFILES := $(wildcard *.c)
OBJFILES := $(SRCFILES:.c=.o)

all: default

default: $(OBJFILES)
	$(CC) $(LDFLAGS) $(OBJFILES) -o $(APPNAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# remove object files and executable when user executes "make clean"
clean:
	rm $(OBJFILES) $(APPNAME)
