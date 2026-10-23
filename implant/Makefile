# s0P0wn3d — Implant (cross-compile MinGW pour Windows)
#
# Prérequis: mingw-w64 et OpenSSL pour MinGW
#   sudo apt install gcc-mingw-w64-x86-64
#   OpenSSL: compiler avec MinGW ou utiliser les libs précompilées
#
# Compilation: make
# Nettoyage:  make clean

CC       = x86_64-w64-mingw32-gcc
CFLAGS   = -Wall -Wextra -std=c11 -O2 -g \
           -I../shared -I. \
           -D_WIN32_WINNT=0x0601
LDFLAGS  = -lwinhttp -lcrypto -lws2_32 -ladvapi32

SRC = main.c beacon.c transport.c crypto.c task_runner.c modules.c \
      ../shared/base64.c ../shared/json.c

OBJ = $(SRC:.c=.o)
TARGET = s0p0wn3d_implant.exe

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	wine $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

# Dépendances
main.o: main.c beacon.h ../shared/protocol.h
beacon.o: beacon.c beacon.h transport.h crypto.h task_runner.h \
          ../shared/protocol.h ../shared/json.h ../shared/base64.h
transport.o: transport.c transport.h ../shared/protocol.h ../shared/json.h
crypto.o: crypto.c crypto.h ../shared/protocol.h ../shared/base64.h
task_runner.o: task_runner.c task_runner.h modules.h ../shared/protocol.h
modules.o: modules.c modules.h ../shared/protocol.h
base64.o: ../shared/base64.c ../shared/base64.h
json.o: ../shared/json.c ../shared/json.h ../shared/protocol.h
