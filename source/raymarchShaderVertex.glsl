#version 330 core



layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
precision mediump float;

out vec2 rect_uv;

void main()
{
    gl_Position = vec4(a_pos, 0.0, 1.0);

    rect_uv = a_uv;
    
}
