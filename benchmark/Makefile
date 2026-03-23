SHELL = bash

os := $(shell uname -s)

ifeq ($(os),Linux)
so_ext = .so
endif
ifeq ($(os),Darwin)
so_ext = .dylib
endif

BUILD_DIR := build
IMPLEMENTATIONS_INC := $(BUILD_DIR)/implementations.inc

GO_LIB := $(BUILD_DIR)/libgo$(so_ext)
CPP_LIB := $(BUILD_DIR)/libs3cpp$(so_ext)
RUST_LIB := $(BUILD_DIR)/librust$(so_ext)

all: $(GO_LIB) $(CPP_LIB) $(RUST_LIB) $(IMPLEMENTATIONS_INC)

$(BUILD_DIR):
	mkdir -p $@
	echo "" > $(IMPLEMENTATIONS_INC)

$(GO_LIB): | $(BUILD_DIR)
	cd ./aws-sdk-go-v2 && go build -buildmode=c-shared -o ../$(BUILD_DIR)/libgo$(so_ext) .

$(CPP_LIB): | $(BUILD_DIR)
	c++ -std=c++26 -shared -fPIC s3cpp/task.cpp -o $(BUILD_DIR)/libs3cpp$(so_ext)

$(RUST_LIB): | $(BUILD_DIR)
	cargo build --release --manifest-path ./aws-sdk-rust/Cargo.toml
	mv ./aws-sdk-rust/target/release/librust$(so_ext) $(BUILD_DIR)/.

$(IMPLEMENTATIONS_INC): $(GO_LIB) $(CPP_LIB) $(RUST_LIB) | $(BUILD_DIR)
	printf '"./%s",\n' $(GO_LIB) $(CPP_LIB) $(RUST_LIB) > $@

bench: $(IMPLEMENTATIONS_INC) $(GO_LIB) $(CPP_LIB) $(RUST_LIB) | $(BUILD_DIR)
	c++ -std=c++26 main.cpp -o $(BUILD_DIR)/bench

test: $(IMPLEMENTATIONS_INC) $(GO_LIB) $(CPP_LIB) $(RUST_LIB) | $(BUILD_DIR)
	c++ -std=c++26 test.cpp -o $(BUILD_DIR)/test


.PHONY: all clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf ./aws-sdk-rust/target
