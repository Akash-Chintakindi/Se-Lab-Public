# STUDENTS: DO NOT MODIFY.
#
# Definitions

include ../config.mk

CC_FLAGS += -I../include -I../include/base -I../include/pipe -I../include/cache
CC_FLAGS += $(EXTRA_FLAGS)

# Targets

all: se test clean

se: 
	(cd base && make $@)
	(cd pipe && make $@)
	(cd cache && make $@)

test:
	(cd base && make $@)
	(cd pipe && make $@)
	(cd cache && make $@)
	(cd testbench && make $@)

depend:
	${MD} -- ${CC_OPTIONS} ${CC_FLAGS} -- ${SRCS}

clean:
	(cd base && make $@)
	(cd pipe && make $@)
	(cd cache && make $@)
	(cd testbench && make $@)
	${RM} -r build *.so *.bak
