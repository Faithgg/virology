# s0P0wn3d — Contrôleur C2
# Compilation: make
# Nettoyage:  make clean

CC       = gcc
CFLAGS   = -Wall -Wextra -Werror -std=c11 -O2 -g \
           -D_POSIX_C_SOURCE=200809L \
           -I../shared -I. -pthread
LDFLAGS  = -lcrypto -lssl -lpthread

SRC = main.c http_server.c sessions.c task_queue.c crypto.c \
      ../shared/base64.c ../shared/json.c

OBJ = $(SRC:.c=.o)
TARGET = s0p0wn3d_controller

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@echo "[*] Test de démarrage..."
	-./$(TARGET) 9999 &
	@sleep 1
	@curl -s http://localhost:9999/health || echo "[-] Serveur non joignable"
	@kill %1 2>/dev/null || true

clean:
	rm -f $(OBJ) $(TARGET)

# Dépendances
main.o: main.c crypto.h sessions.h task_queue.h http_server.h ../shared/protocol.h
http_server.o: http_server.c http_server.h crypto.h sessions.h task_queue.h \
               ../shared/protocol.h ../shared/base64.h ../shared/json.h
sessions.o: sessions.c sessions.h ../shared/protocol.h
task_queue.o: task_queue.c task_queue.h ../shared/protocol.h
crypto.o: crypto.c crypto.h ../shared/protocol.h ../shared/base64.h
base64.o: ../shared/base64.c ../shared/base64.h
json.o: ../shared/json.c ../shared/json.h ../shared/protocol.h
