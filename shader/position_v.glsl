#version 330 core
layout (location = 0) in vec3 aPos;

out vec4 pos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.);
    gl_Position = gl_Position/gl_Position.w;
    pos = (gl_Position+vec4(1.,1.,1.,1.))/2.;
}