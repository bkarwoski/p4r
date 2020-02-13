CFLAGS = -ggdb3 -std=c11 -Wall -Wunused-parameter -Wstrict-prototypes -Werror -Wextra -Wshadow
CFLAGS += -fsanitize=signed-integer-overflow
CFLAGS += -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable
#CFLAGS += -fsanitize=address -fsanitize=undefined

calc: calc.c
	gcc -o $@ $^ $(CFLAGS) -std=c11 -lm -lpthread

chase: chase.c bmp.c image_server.c graphics.c vector_xy_t.c collision.c
	gcc -o $@ $^ $(CFLAGS) -std=c11 -lm -lpthread -O3