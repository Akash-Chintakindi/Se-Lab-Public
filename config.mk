# Common build configuration
# Include this file in all Makefiles for consistent settings

# Compiler and tools
CC = gcc
RM = /bin/rm -f
MV = mv -f
LD = gcc
MD = gccmakedep

# Compiler flags
CC_FLAGS = -Wall -Wextra -Werror -Wno-unused-parameter -g3 -pthread -Wno-unused-function
CC_OPTIONS = -c
CC_SO_OPTIONS = -shared -fpic
CC_DL_OPTIONS = -rdynamic
LIBS = -ldl

BUILD_DIR = build

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	${CC} ${CC_OPTIONS} ${CC_FLAGS} -o $@ $<
