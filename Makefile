ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

CC := gcc
CFLAGS += -g -Wall -Werror -Wextra -std=c11 -I . -I ./logproto
LDFLAGS += -lprotobuf-c -lsnappy -lcurl

TARGET := lokicat
TEST_TARGET := test/all_tests

LOKI_REF := master

PROTO_FILES := $(shell find . -name '*.proto')
PROTO_C_FILES := $(subst .proto,.pb-c.c,$(PROTO_FILES))
PROTO_OBJECTS := $(subst .proto,.pb-c.o,$(PROTO_FILES))

SOURCES := $(wildcard ./*.c)
SOURCES_OBJECTS := $(subst .c,.o,$(SOURCES))

TEST_SOURCES := $(wildcard ./test/*.c)

OBJECTS := $(SOURCES_OBJECTS) $(PROTO_OBJECTS)

default: build

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


$(TEST_TARGET): test/cu_test/cu_test.o $(TEST_SOURCES) $(filter-out ./lokicat.o,$(OBJECTS))
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: test
test: $(TEST_TARGET) ## Run test suite
	$(TEST_TARGET)

.PHONY: clean
clean:
	rm -rf $(TARGET) $(TEST_TARGET)
	find . \( -name '*.o' \) -delete -print

.PHONY: install

install: $(TARGET) ## Install binary into PREFIX
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/
