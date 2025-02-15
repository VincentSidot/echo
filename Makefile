# Compiler
C_OPTS = -Wall -pedantic -std=c99 -D _DEFAULT_SOURCE -D _DEBUG -g
CC = gcc ${C_OPTS}

# Directories
BUILD_DIR = build
SRC_DIR = src
BIN_NAME = echo

# Targets
${BIN_NAME}: ${BUILD_DIR}/main.o
	${CC} -o ${BUILD_DIR}/${BIN_NAME} ${BUILD_DIR}/main.o

${BUILD_DIR}/main.o: .build
	${CC} -o ${BUILD_DIR}/main.o -c ${SRC_DIR}/main.c

test: ${BUILD_DIR}/array_test
	@${BUILD_DIR}/array_test

${BUILD_DIR}/array_test: ${BUILD_DIR}/array_test.o
	@${CC} -o ${BUILD_DIR}/array_test ${BUILD_DIR}/array_test.o

${BUILD_DIR}/array_test.o: .build
	@${CC} -o ${BUILD_DIR}/array_test.o -c ${SRC_DIR}/array_test.c

clean:
	@rm -rf ${BUILD_DIR}

.build:
	@mkdir -p ${BUILD_DIR}

.PHONY: clean .build