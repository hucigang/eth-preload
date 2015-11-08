all: vidyoE.so

CFLAGS = -Wall -fPIC -shared

vidyoE.so: vidyoE.c
	${CC} ${CFLAGS} -o $@ $^

clean:
	rm -fr *.o
