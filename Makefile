CC=gcc
CFLAGS=-Wall -Wextra -g $$(curl-config --cflags)
LFLAGS=$$(curl-config --libs)
LIBS=find_course_codes.c

TARGET=scraper

all : $(TARGET)

% : %.c $(LIBS)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)