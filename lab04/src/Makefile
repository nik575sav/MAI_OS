CC = cc
CFLAGS = -Wall -Wextra -lm -ldl
TARGET = main alloc.so buddy.so

build: ${TARGET}

main: main.c
	${CC} ${CFLAGS} -o $@ $<

buddy.so: buddy.c
	${CC} ${CFLAGS} -shared -fPIC -o $@ $<

alloc.so: allocator.c
	${CC} ${CFLAGS} -shared -fPIC -o $@ $<


clean:
	@rm -f ${TARGET}
	@rm -rf ./*.dSYM
