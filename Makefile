GLEW_INCLUDE = /opt/local/include
GLEW_LIB = /opt/local/lib
CXX = clang
EFLAGS = -g -Weverything -Wno-unused -Wno-unreachable-code -Wno-padded -Wno-c++-compat -Werror

stone: main.o world.o shader.o util.o occlusion.o
	$(CXX) -o stone $^ -framework GLUT -framework OpenGL -L$(GLEW_LIB) -lGLEW $(EFLAGS) -lm

%.o: %.c %.h
	$(CXX) -c -o $@ $< -I$(GLEW_INCLUDE) $(EFLAGS)

clean:
	rm -rf stone *.o stone.dSYM

run: stone
	exec ./stone
