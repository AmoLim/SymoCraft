#version 460 core

layout (location = 0) in vec3 i_pos_coord;

uniform mat4 u_combo_mat; // combo_mat = projection_mat * view_mat

void main()
{
    gl_Position = u_combo_mat * vec4(i_pos_coord, 1.0f);
}