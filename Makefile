CC = gcc
OUT_DIR =bin
CFLAGS = -I include/ -I $(MBEDTLS_INCLUDE_DIR) -Wextra -Werror -g
CLIBS = -pthread -L./Dependencies/mbedtls/library -lmbedtls -lmbedcrypto -lmbedx509
		

CLANG_FORMAT := $(shell command -v clang-format 2> /dev/null)
TARGET = https_server

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,$(OUT_DIR)/%.o,$(SRCS))

MBEDTLS_INCLUDE_DIR = ./Dependencies/mbedtls/include/
EMBED_DEP = ./Dependencies/mbedtls/library/libmbedcrypto.a \
			./Dependencies/mbedtls/library/libmbedx509.a \
			./Dependencies/mbedtls/library/libmbedtls.a

all: format $(TARGET)

$(EMBED_DEP):
	$(MAKE) -C ./Dependencies/mbedtls/library

$(TARGET): $(OBJS) $(EMBED_DEP)
	$(CC) $(CFLAGS) -o $(OUT_DIR)/$(TARGET) $(OBJS) $(CLIBS)

$(OUT_DIR)/%.o: src/%.c | $(OUT_DIR) $(EMBED_DEP)
	$(CC) $(CFLAGS) $(CLIBS) -c $< -o $@

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

format:
ifdef CLANG_FORMAT
	cd src/ && clang-format -i -style=Google *.c
	cd include/ && clang-format -i -style=Google *.h
else
	@echo "clang-format not found. Skipping formatting."
endif

servertest:
	cd test/ &&\
	python3 generage_file.py 100005000 &&\
	sh apache-test.sh &&\
	python3 access_to_outer_scope.py &&\
	python3 largeconnection.py 50 100005000 &&\
	python3 receiver.py 2000

cleantest:
	cd Static/ &&\
	rm test_string.txt &&\
	rm large_data.txt

clean:
	rm -rf $(OUT_DIR)

