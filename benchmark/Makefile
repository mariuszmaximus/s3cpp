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

AWS_SDK_CPP_PREFIX := $(shell brew --prefix aws-sdk-cpp)
AWS_SDK_CPP_DEPS := aws-sdk-cpp $(shell brew deps aws-sdk-cpp)
AWS_SDK_CPP_INCLUDES := $(foreach dep,$(AWS_SDK_CPP_DEPS),-I$(shell brew --prefix $(dep))/include)

GOOGLE_BENCHMARK_PREFIX := $(shell brew --prefix google-benchmark)

$(CPP_LIB): | $(BUILD_DIR)
	c++ -std=c++26 -shared -fPIC \
		$(AWS_SDK_CPP_INCLUDES) \
		aws-sdk-cpp/task.cpp \
		-o $@ \
		-L$(AWS_SDK_CPP_PREFIX)/lib \
		-laws-cpp-sdk-s3 -laws-cpp-sdk-core

$(RUST_LIB): | $(BUILD_DIR)
	cargo build --release --manifest-path ./aws-sdk-rust/Cargo.toml
	mv ./aws-sdk-rust/target/release/librust$(so_ext) $(BUILD_DIR)/.

$(S3CPP_LIB): | $(BUILD_DIR)
	cmake -S .. -B ../build-release -DCMAKE_BUILD_TYPE=Release
	cmake --build ../build-release
	c++ -std=c++26 -shared -fPIC -I../src s3cpp/task.cpp -o $(BUILD_DIR)/libs3cpp$(so_ext) \
		../build-release/lib/libs3cpp.a $(shell pkg-config --libs libcurl libcrypto)

$(IMPLEMENTATIONS_INC): $(GO_LIB) $(CPP_LIB) $(RUST_LIB) $(S3CPP_LIB) | $(BUILD_DIR)
	printf '"./%s",\n' $(GO_LIB) $(CPP_LIB) $(RUST_LIB) $(S3CPP_LIB) > $@

bench: $(IMPLEMENTATIONS_INC) $(GO_LIB) $(CPP_LIB) $(RUST_LIB) $(S3CPP_LIB)  | $(BUILD_DIR)
	c++ -std=c++26 -I../src benchmark.cpp -o $(BUILD_DIR)/bench \
	    -I$(GOOGLE_BENCHMARK_PREFIX)/include -L$(GOOGLE_BENCHMARK_PREFIX)/lib \
	    ../build/lib/libs3cpp.a -lbenchmark $(shell pkg-config --libs libcurl libcrypto)
	./$(BUILD_DIR)/bench


test: $(IMPLEMENTATIONS_INC) $(GO_LIB) $(S3CPP_LIB) $(RUST_LIB) | $(BUILD_DIR)
	c++ -std=c++26 -I../src test.cpp -o $(BUILD_DIR)/test ../build-release/lib/libs3cpp.a $(shell pkg-config --libs libcurl libcrypto)
	./$(BUILD_DIR)/test


.PHONY: all clean aws-sdk-cpp
aws-sdk-cpp: $(CPP_LIB)

clean:
	rm -rf $(BUILD_DIR)
	rm -rf ./aws-sdk-rust/target
