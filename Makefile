GLEW_INCLUDE = /opt/local/include
GLEW_LIB = /opt/local/lib

stone: stone.o
	gcc -o stone $^ -framework GLUT -framework OpenGL -L$(GLEW_LIB) -lGLEW

.c.o:
	gcc -c -o $@ $< -I$(GLEW_INCLUDE)

clean:
	rm stone stone.o

run: stone
	./stone
