#version 430

in vec2 uv;

uniform sampler2D tex;

out vec4 final_color;

void main()
{
	vec4 texColor = texture(tex, uv);
	final_color = vec4(texColor.rgb, 1.0);
}