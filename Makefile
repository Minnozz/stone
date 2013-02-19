GLEW_INCLUDE = /usr/local/include
GLEW_LIB = /usr/local/lib
CXX = clang
EFLAGS = -g -Weverything -Werror -Wno-c++-compat -Wno-error=unused-parameter -Wno-error=unused-function -Wno-error=unreachable-code -Wno-error=padded

stone: main.o world.o shader.o util.o occlusion.o
	$(CXX) -o stone $^ -framework GLUT -framework OpenGL -L$(GLEW_LIB) -lGLEW $(EFLAGS) -lm

%.o: %.c %.h
	$(CXX) -c -o $@ $< -I$(GLEW_INCLUDE) $(EFLAGS)

clean:
	rm -rf stone *.o stone.dSYM

run: stone
	exec ./stone
