#ifndef _MAIN_H
#define _MAIN_H

static void init(int argc, char **argv);
static void idle(void);
static void display(void);
static void keyboard(unsigned char key, int x, int y);
static void mouse(int button, int state, int x, int y);

#endif /* !defined _MAIN_H */
