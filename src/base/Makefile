# STUDENTS: DO NOT MODIFY.
#
# Definitions

include ../../config.mk

CC_FLAGS += -I../../include -I../../include/base -I../../include/pipe -I../../include/cache
CC_FLAGS += $(EXTRA_FLAGS)

SRCS := \
archsim.c \
elf_loader.c \
err_handler.c \
handle_args.c hw_elts.c \
interface.c \
machine.c mem.c \
proc.c ptable.c stage_ordering.c

OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

TEST_SRCS := \
elf_loader.c \
err_handler.c \
handle_args.c hw_elts.c \
interface.c \
machine.c mem.c \
proc.c ptable.c stage_ordering.c

TEST_OBJS := $(TEST_SRCS:%.c=$(BUILD_DIR)/%.o)

# Targets

all: se clean

se: ${OBJS}

test: ${TEST_OBJS}

depend:
	${MD} -- ${CC_OPTIONS} ${CC_FLAGS} -- ${SRCS}

clean:
	${RM} -r $(BUILD_DIR) *.so *.bak
