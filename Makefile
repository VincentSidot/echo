# Compiler
# C_OPTS = -Wall -pedantic -std=c99 -D _DEFAULT_SOURCE -D _DEBUG -g
C_OPTS = -Wall -pedantic -std=c99 -D _DEFAULT_SOURCE -O3
CC = gcc ${C_OPTS}

# Directories
BUILD_DIR = build
TEST_DIR = tests
SRC_DIR = src

.PHONY: clean .build test echo client

# Targets
client: ${BUILD_DIR}/client
echo: ${BUILD_DIR}/echo

${BUILD_DIR}/client: ${BUILD_DIR}/client.o
	${CC} -o ${BUILD_DIR}/client ${BUILD_DIR}/client.o

${BUILD_DIR}/client.o: .build
	${CC} -o ${BUILD_DIR}/client.o -c ${SRC_DIR}/client.c

${BUILD_DIR}/echo: ${BUILD_DIR}/echo.o
	${CC} -o ${BUILD_DIR}/echo ${BUILD_DIR}/echo.o

${BUILD_DIR}/echo.o: .build
	${CC} -o ${BUILD_DIR}/echo.o -c ${SRC_DIR}/echo.c

test: ${BUILD_DIR}/test_array ${BUILD_DIR}/test_array_thread
	${BUILD_DIR}/test_array
	${BUILD_DIR}/test_array_thread

${BUILD_DIR}/test_array: ${BUILD_DIR}/test_array.o
	@${CC} -o ${BUILD_DIR}/test_array ${BUILD_DIR}/test_array.o

${BUILD_DIR}/test_array.o: .build
	@${CC} -o ${BUILD_DIR}/test_array.o -c ${TEST_DIR}/array.c

${BUILD_DIR}/test_array_thread: ${BUILD_DIR}/test_array_thread.o
	@${CC} -o ${BUILD_DIR}/test_array_thread ${BUILD_DIR}/test_array_thread.o

${BUILD_DIR}/test_array_thread.o: .build
	@${CC} -o ${BUILD_DIR}/test_array_thread.o -c ${TEST_DIR}/array_thread.c


clean:
	@rm -rf ${BUILD_DIR}

.build:
	@mkdir -p ${BUILD_DIR}
