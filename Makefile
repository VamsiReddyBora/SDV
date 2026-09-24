CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -I./common
LDFLAGS ?= -lm

BIN_DIR = bin
COMMON_SRC = common/can_common.c
COMMON_OBJ = $(COMMON_SRC:.c=.o)

NODES = central_compute pcm_node bms_node bcm_node hmi_node adas_node brake_node eps_node hvac_node telematics_node
BINARIES = $(addprefix $(BIN_DIR)/, $(NODES))

.PHONY: all clean run

all: $(BIN_DIR) $(BINARIES)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

common/%.o: common/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/%: nodes/%.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) $< $(COMMON_OBJ) -o $@ $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR) common/*.o
