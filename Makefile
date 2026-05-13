# Simple CMake wrapper (Linux/macOS convenience)
#
# Usage:
#   make            # builds tetris (Release)
#   make build      # same as above
#   make run        # runs tetris
#   make test       # builds + runs logic_smoke
#   make clean      # removes build dir

BUILD_DIR := build
DEBUG_BUILD_DIR := build-debug
CMAKE := cmake

# Keep `make test` deterministic by default so it is reliable.
# Override as needed:
#   make test SEED=123 PIECES=200
SEED ?= 1
PIECES ?= 20

.PHONY: all configure configure_debug build debug run run_debug test clean
.PHONY: test_rotate

all: build

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

configure_debug:
	$(CMAKE) -S . -B $(DEBUG_BUILD_DIR) -DCMAKE_BUILD_TYPE=RelWithDebInfo

build: configure
	$(CMAKE) --build $(BUILD_DIR) --target tetris

debug: configure_debug
	$(CMAKE) --build $(DEBUG_BUILD_DIR) --target tetris

run: build
	./$(BUILD_DIR)/tetris

run_debug: debug
	./$(DEBUG_BUILD_DIR)/tetris

test: configure
	$(CMAKE) --build $(BUILD_DIR) --target logic_smoke
	./$(BUILD_DIR)/logic_smoke $(SEED) $(PIECES)

test_rotate: build
	./$(BUILD_DIR)/tetris --rotate-demo

clean:
	rm -rf $(BUILD_DIR) $(DEBUG_BUILD_DIR)
