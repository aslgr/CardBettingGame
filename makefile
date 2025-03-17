jogador1_name=jogador1
jogador2_name=jogador2
jogador3_name=jogador3
jogador4_name=jogador4

jogador1_objs=libJogo.o jogador1.o
jogador2_objs=libJogo.o jogador2.o
jogador3_objs=libJogo.o jogador3.o
jogador4_objs=libJogo.o jogador4.o

CFLAGS := -Wall
LDFLAGS := -lncurses

all: $(jogador1_name) $(jogador2_name) $(jogador3_name) $(jogador4_name)

$(jogador1_name): $(jogador1_objs)
	gcc -o $(jogador1_name) $(jogador1_objs) $(LDFLAGS)

$(jogador2_name): $(jogador2_objs)
	gcc -o $(jogador2_name) $(jogador2_objs) $(LDFLAGS)

$(jogador3_name): $(jogador3_objs)
	gcc -o $(jogador3_name) $(jogador3_objs) $(LDFLAGS)

$(jogador4_name): $(jogador4_objs)
	gcc -o $(jogador4_name) $(jogador4_objs) $(LDFLAGS)

libJogo.o: libJogo.h libJogo.c
	gcc -c libJogo.c $(CFLAGS)

jogador1.o: jogador1.c
	gcc -c jogador1.c $(CFLAGS)

jogador2.o: jogador2.c
	gcc -c jogador2.c $(CFLAGS)

jogador3.o: jogador3.c
	gcc -c jogador3.c $(CFLAGS)

jogador4.o: jogador4.c
	gcc -c jogador4.c $(CFLAGS)

clean:
	rm -f $(jogador1_objs) $(jogador2_objs) $(jogador3_objs) $(jogador4_objs)

purge: clean
	-rm -f $(jogador1_name) $(jogador2_name) $(jogador3_name) $(jogador4_name)

