#version 460 core

layout (location = 0) in vec3 i_pos_coord;
layout (location = 1) in vec3 i_tex_coord; // ** z coord is the layer index for the texture array, starting from 0 **
layout (location = 2) in float i_normal;

out vec3 o_tex_coord;

uniform mat4 u_combo_mat; // combo_mat = projection_mat * view_mat

void main()
{
	gl_Position = u_combo_mat * vec4(i_pos_coord, 1.0f);

    o_tex_coord = i_tex_coord;
}