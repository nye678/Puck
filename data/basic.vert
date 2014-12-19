#version 430

uniform mat4 camera;

out vec2 uv;

void main()
{
	const vec4 vertices[4] = vec4[4](vec4(0.0, 0.0, 0.0, 1.0),
									 vec4(1.0, 0.0, 0.0, 1.0),
									 vec4(0.0, -1.0, 0.0, 1.0),
									 vec4(1.0, -1.0, 0.0, 1.0));

	gl_Position = camera * vertices[gl_VertexID];

	const vec2 uv_verts[4] = vec2[4](vec2(0.0, 0.0),
									 vec2(1.0, 0.0),
									 vec2(0.0, 1.0),
									 vec2(1.0, 1.0));

	uv = uv_verts[gl_VertexID];
}