#ifndef _SHADER_H
#define _SHADER_H

#include <GL/glew.h>

GLuint make_shader(GLenum type, const char *filename);
GLuint make_program(GLuint vertex_shader, GLuint fragment_shader);

#endif /* !defined _SHADER_H */
