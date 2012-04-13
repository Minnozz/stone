GLEW_INCLUDE = /opt/local/include
GLEW_LIB = /opt/local/lib
CXX = clang
EFLAGS = -Weverything -Werror

stone: main.o world.o
	$(CXX) -o stone $^ -framework GLUT -framework OpenGL -L$(GLEW_LIB) -lGLEW $(EFLAGS) -lm

.c.o:
	$(CXX) -c -o $@ $< -I$(GLEW_INCLUDE) $(EFLAGS)

clean:
	rm -f stone *.o

run: stone
	exec ./stone
