
CC := gcc
CFLAGS := -g -Wall -Werror -Wextra -std=c11 -I ./logproto $(pkg-config --cflags)
LDFLAGS := $(shell pkg-config --libs 'libprotobuf-c >= 1.0.0, snappy, libcurl')
TARGET := lokicat

LOKI_REF := master

PROTO_FILES := $(shell find . -name '*.proto')
PROTO_C_FILES := $(subst .proto,.pb-c.c,$(PROTO_FILES))
PROTO_OBJECTS := $(subst .proto,.pb-c.o,$(PROTO_FILES))

SOURCES := $(wildcard ./*.c)
SOURCES_OBJECTS := $(subst .c,.o,$(SOURCES))

TEST_SOURCES := $(wildcard ./test/*.c)

OBJECTS := $(SOURCES_OBJECTS) $(PROTO_OBJECTS)

# from https://suva.sh/posts/well-documented-makefiles/
.PHONY: help
help:  ## Display this help
	@awk 'BEGIN {FS = ":.*##"; printf "\nUsage:\n  make \033[36m<target>\033[0m\n\nTargets:\n"} /^[a-zA-Z_-]+:.*?##/ { printf "  \033[36m%-20s\033[0m %s\n", $$1, $$2 }' $(MAKEFILE_LIST)

.PHONY: build
build: $(TARGET) ## Build lokicat

.PHONY: update-proto
update-proto: ## Update vendored .proto files from the upstream repositories
	mkdir -p logproto logproto/google/protobuf logproto/github.com/gogo/protobuf/gogoproto
	curl --fail -L -o logproto/logproto.proto -L https://raw.githubusercontent.com/grafana/loki/$(LOKI_REF)/pkg/logproto/logproto.proto
	curl --fail -L -o logproto/google/protobuf/descriptor.proto -L https://raw.githubusercontent.com/grafana/loki/$(LOKI_REF)/vendor/github.com/golang/protobuf/protoc-gen-go/descriptor/descriptor.proto
	curl --fail -L -o logproto/google/protobuf/timestamp.proto -L https://raw.githubusercontent.com/grafana/loki/$(LOKI_REF)/vendor/github.com/golang/protobuf/ptypes/timestamp/timestamp.proto
	curl --fail -L -o logproto/github.com/gogo/protobuf/gogoproto/gogo.proto https://raw.githubusercontent.com/grafana/loki/$(LOKI_REF)/vendor/github.com/gogo/protobuf/gogoproto/gogo.proto

.PHONY: generate-proto
generate-proto: $(PROTO_C_FILES) ## Generate C code from .proto files

%.pb-c.c: %.proto
	protoc -I ./logproto --c_out ./logproto $<

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LDFLAGS) -o $@

test: input_logread.o $(PROTO_OBJECTS)
	$(CC) input_logread.o $(PROTO_OBJECTS) -Wall $(LDFLAGS) -o test

fmt: ## Format all non generated C files
	astyle \
	--style=1tbs \
	--lineend=linux \
	--convert-tabs \
	--preserve-date \
	--pad-header \
	--indent-switches \
	--align-pointer=name \
	--align-reference=name \
	--pad-oper \
	--suffix=none \
	$(SOURCES) $(TEST_SOURCES)

.PHONY: clean
clean:
	rm -rf $(TARGET)
	find . \( -name '*.pb-c.?' -o -name '*.o' \) -delete -print
