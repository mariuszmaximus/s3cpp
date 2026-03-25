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
CPP_LIB := $(BUILD_DIR)/libawscpp$(so_ext)
RUST_LIB := $(BUILD_DIR)/librust$(so_ext)
S3CPP_LIB := $(BUILD_DIR)/libs3cpp$(so_ext)

all: $(GO_LIB) $(CPP_LIB) $(RUST_LIB) $(S3CPP_LIB) $(IMPLEMENTATIONS_INC)

$(BUILD_DIR):
	mkdir -p $@
	echo "" > $(IMPLEMENTATIONS_INC)

$(GO_LIB): | $(BUILD_DIR)
	cd ./aws-sdk-go-v2 && go build -buildmode=c-shared -o ../$(BUILD_DIR)/libgo$(so_ext) .

AWS_SDK_CPP_INSTALL := $(BUILD_DIR)/.aws-sdk-cpp-installed

$(CPP_LIB): $(AWS_SDK_CPP_INSTALL) | $(BUILD_DIR)
	c++ -std=c++26 -shared -fPIC \
		-I$(BUILD_DIR)/aws-sdk-cpp-install/include \
		aws-sdk-cpp/task.cpp \
		-o $@ \
		-L$(BUILD_DIR)/aws-sdk-cpp-install/lib \
		-laws-cpp-sdk-s3 -laws-cpp-sdk-core \
		-Wl,-rpath,$(BUILD_DIR)/aws-sdk-cpp-install/lib

$(AWS_SDK_CPP_INSTALL): | $(BUILD_DIR)
	# Build from source (perhaps we should use a prebuilt binary)
	[ -d $(BUILD_DIR)/aws-sdk-cpp ] || \
		git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp $(BUILD_DIR)/aws-sdk-cpp
	cmake -S $(BUILD_DIR)/aws-sdk-cpp -B $(BUILD_DIR)/aws-sdk-cpp-build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(BUILD_DIR)/aws-sdk-cpp-install -DBUILD_ONLY="s3" -DBUILD_TESTING=OFF
	cmake --build $(BUILD_DIR)/aws-sdk-cpp-build --config Release
	cmake --install $(BUILD_DIR)/aws-sdk-cpp-build --config Release
	touch $@

$(RUST_LIB): | $(BUILD_DIR)
	cargo build --release --manifest-path ./aws-sdk-rust/Cargo.toml
	mv ./aws-sdk-rust/target/release/librust$(so_ext) $(BUILD_DIR)/.

$(S3CPP_LIB): | $(BUILD_DIR)
	cmake --build ../build
	c++ -std=c++26 -shared -fPIC -I../src s3cpp/task.cpp -o $(BUILD_DIR)/libs3cpp$(so_ext) \
		../build/lib/libs3cpp.a $(shell pkg-config --libs libcurl libcrypto)

$(IMPLEMENTATIONS_INC): $(GO_LIB) $(CPP_LIB) $(RUST_LIB) $(S3CPP_LIB) | $(BUILD_DIR)
	printf '"./%s",\n' $(GO_LIB) $(CPP_LIB) $(RUST_LIB) $(S3CPP_LIB) > $@

bench: $(IMPLEMENTATIONS_INC) $(GO_LIB) $(CPP_LIB) $(RUST_LIB) $(S3CPP_LIB)  | $(BUILD_DIR)
	c++ -std=c++26 main.cpp -o $(BUILD_DIR)/bench

test: $(IMPLEMENTATIONS_INC) $(GO_LIB) $(S3CPP_LIB) $(RUST_LIB) | $(BUILD_DIR)
	c++ -std=c++26 -I../src test.cpp -o $(BUILD_DIR)/test ../build/lib/libs3cpp.a $(shell pkg-config --libs libcurl libcrypto)


.PHONY: all clean aws-sdk-cpp
aws-sdk-cpp: $(CPP_LIB)

clean:
	rm -rf $(BUILD_DIR)
	rm -rf ./aws-sdk-rust/target
	rm -rf $(BUILD_DIR)/aws-sdk-cpp $(BUILD_DIR)/aws-sdk-cpp-build $(BUILD_DIR)/aws-sdk-cpp-install
