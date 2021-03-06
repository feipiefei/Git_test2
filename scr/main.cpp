#include <gl/glew.h>
#include <GL/glut.h>

#include "displayer.h"

#include <iostream>


// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;


auto displayer = Displayer::getInstance();

float deltaTime = 0.03;

void f_w(int, int)
{
    displayer.camera.ProcessKeyboard(FORWARD, deltaTime);
}

void f_s(int, int)
{
    displayer.camera.ProcessKeyboard(BACKWARD, deltaTime);
}

void f_a(int, int)
{
    displayer.camera.ProcessKeyboard(LEFT, deltaTime);
}

void f_d(int, int)
{
    displayer.camera.ProcessKeyboard(RIGHT, deltaTime);
}

void f_z(int, int)
{
    displayer.camera.ProcessKeyboard(UPWARD, deltaTime);
}

void f_x(int, int)
{
    displayer.camera.ProcessKeyboard(DOWNWARD, deltaTime);
}

int main(int argc,char* argv[])
{
    displayer.init(argc, argv, "hello", SCR_WIDTH, SCR_HEIGHT);
    displayer.bindKey('w', f_w);
    displayer.bindKey('s', f_s);
    displayer.bindKey('a', f_a);
    displayer.bindKey('d', f_d);
    displayer.bindKey('z', f_z);
    displayer.bindKey('x', f_x);

    displayer.runLoop();

    return 0;
}



