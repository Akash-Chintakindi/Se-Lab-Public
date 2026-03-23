# STUDENTS: DO NOT MODIFY.
#
# Definitions

.SILENT: clean

include ../../config.mk

CC_FLAGS += -I../../include -I../../include/base -I../../include/pipe -I../../include/cache
CC_FLAGS += $(EXTRA_FLAGS)

SRCS := \
test-csim.c \
test-se.c \
test-hw.c

OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

# Targets

all: se test clean

se: ${OBJS}

test: ${OBJS}

depend:
	${MD} -- ${CC_OPTIONS} ${CC_FLAGS} -- ${SRCS}

clean:
	${RM} -r $(BUILD_DIR)
