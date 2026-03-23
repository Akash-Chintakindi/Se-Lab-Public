# Definitions

include ../../config.mk

CC_FLAGS += -I../../include -I../../include/base -I../../include/pipe -I../../include/cache
CC_FLAGS += $(EXTRA_FLAGS)

SRCS := \
forward.c \
hazard_control.c \
instr_base.c \
instr_Fetch.c \
instr_Decode.c \
instr_Decode_extractregs.c \
instr_Execute.c \
instr_Memory.c \
instr_Writeback.c

OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

# Targets

all: se clean

se: ${OBJS}

test : ${OBJS}

depend:
	${MD} -- ${CC_OPTIONS} ${CC_FLAGS} -- ${SRCS}

clean:
	${RM} -r $(BUILD_DIR) *.so *.bak
