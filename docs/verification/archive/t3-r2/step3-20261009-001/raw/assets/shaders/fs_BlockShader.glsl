#version 460 core

layout(early_fragment_tests) in; //Built-in Optimization

in vec3 o_tex_coord; //z coord is the layer index for the texture array, starting from 0

out vec4 o_frag_color;

uniform sampler2DArray u_texture_array;

void main()
{
	vec4 texcolor = texture(u_texture_array, o_tex_coord);
	if (texcolor.a < 0.01f)
		discard;
	o_frag_color = texcolor;
}
