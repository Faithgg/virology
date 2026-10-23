# Build system for s0P0wn3d C2 Framework
# Supports native (controller) and cross-compilation (implant Windows)

# Native compiler (controller for Linux/macOS)
CC_NATIVE = gcc
CFLAGS = -Wall -Wextra -g -O2 -fPIC
LDFLAGS_NATIVE = -lm -lpthread -lssl -lcrypto

# Cross compiler (implant for Windows x64)
CC_WINDOWS = x86_64-w64-mingw32-gcc
CFLAGS_WIN = -Wall -Wextra -g -O2 -D_WIN32_WINNT=0x0601
LDFLAGS_WIN = -lws2_32 -static

# Controller (C2 Server)
CONTROLLER_SRC = controller/main.c controller/crypto.c controller/sessions.c \
                 controller/task_queue.c controller/http_server.c controller/cli.c \
                 shared/base64.c shared/json.c
CONTROLLER_OBJ = $(CONTROLLER_SRC:.c=.o)
CONTROLLER_BIN = s0p0wn3d_controller

# Implant (Windows Client)
IMPLANT_SRC = implant/main.c implant/beacon.c implant/crypto.c implant/transport.c \
              implant/task_runner.c implant/modules.c shared/base64.c shared/json.c
IMPLANT_OBJ_WIN = $(IMPLANT_SRC:.c=.w64.o)
IMPLANT_BIN = s0p0wn3d_implant.exe

.PHONY: all controller implant clean help

help:
	@echo "s0P0wn3d Build System"
	@echo "  make controller  - Build C2 server (Linux/macOS)"
	@echo "  make implant     - Cross-compile Windows implant (.exe)"
	@echo "  make all         - Build both (if possible)"
	@echo "  make clean       - Clean all builds"

all: controller implant

controller: $(CONTROLLER_BIN)

$(CONTROLLER_BIN): $(CONTROLLER_OBJ)
	@echo "[*] Linking controller..."
	@$(CC_NATIVE) $(CFLAGS) -o $@ $^ $(LDFLAGS_NATIVE)
	@echo "[+] Controller built: $@"

implant: $(IMPLANT_BIN)

$(IMPLANT_BIN): $(IMPLANT_OBJ_WIN)
	@echo "[*] Cross-compiling Windows implant..."
	@$(CC_WINDOWS) $(CFLAGS_WIN) -o $@ $^ $(LDFLAGS_WIN)
	@echo "[+] Implant built: $@"

%.o: %.c
	$(CC_NATIVE) $(CFLAGS) -c $< -o $@

%.w64.o: %.c
	$(CC_WINDOWS) $(CFLAGS_WIN) -c $< -o $@

clean:
	@echo "[*] Cleaning..."
	@rm -f $(CONTROLLER_OBJ) $(IMPLANT_OBJ_WIN) $(CONTROLLER_BIN) $(IMPLANT_BIN)
	@echo "[+] Clean done"
