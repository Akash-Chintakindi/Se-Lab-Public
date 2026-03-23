# Definitions

include config.mk

CC_FLAGS += -Iinclude -Iinclude/base -Iinclude/pipe -Iinclude/cache

export EXTRA_FLAGS

# Targets

week4: tidy pipe all test clean
week3: tidy pipe all test clean
week2: tidy pipeminus all test clean
week1: tidy test clean

all:
	(cd src && make se)
	${CC} ${CC_FLAGS} -I instr -o bin/se `/bin/ls src/base/build/*.o src/pipe/build/*.o src/cache/build/cache.o`

pipe:
	$(eval EXTRA_FLAGS += -DPIPE)

pipeminus:
	$(eval EXTRA_FLAGS += -UPIPE)

parallel:
	$(eval EXTRA_FLAGS += -DPARALLEL -URANDOM)

random:
	$(eval EXTRA_FLAGS += -DRANDOM -UPARALLEL)

ec:
	$(eval EXTRA_FLAGS += -DEC)

test:
	(cd src && make $@)
	${CC} ${CC_FLAGS} -I instr -o bin/test-se src/testbench/build/test-se.o
	${CC} ${CC_FLAGS} -I instr -o bin/test-csim src/testbench/build/test-csim.o
	${CC} ${CC_FLAGS} -I instr -o bin/test-hw `/bin/ls src/base/build/elf_loader.o src/base/build/err_handler.o src/base/build/hw_elts.o src/base/build/interface.o src/base/build/machine.o src/base/build/mem.o src/base/build/proc.o src/base/build/ptable.o src/base/build/stage_ordering.o src/pipe/build/*.o src/cache/build/cache.o src/testbench/build/test-hw.o`

depend:
	(cd src && make $@)

clean:
	(cd src && make $@)
	${RM} -r build *.so *.bak

tidy:
	${RM} bin/test-hw bin/se bin/test-se bin/test-csim bin/csim

count:
	wc -l src/base/*.c src/pipe/*.c src/cache/*.c | tail -n 1
	wc -l include/base/*.h include/pipe/*.h include/cache/*.h | tail -n 1
