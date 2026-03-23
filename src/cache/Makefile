include ../../config.mk

CC_FLAGS += -I../../include -I../../include/base -I../../include/pipe -I../../include/cache
CC_FLAGS += -O0

##################################################
# You shouldn't need to modify anything below here
##################################################
SRCS := \
csim.c \
cache.c

OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)


all: csim

se: all

test: all

csim: ${OBJS}
	$(CC) $(CC_FLAGS) -o ../../bin/$@ ${OBJS} -lm

clean:
	${RM} -r $(BUILD_DIR) test-csim *.exe *~ 
