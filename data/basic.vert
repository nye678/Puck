#version 430

layout (location = 0) in vec3 vertex;
layout (location = 1) in mat4 transform;

layout (location = 5) uniform mat4 camera;

void main()
{
	gl_Position = camera * transform * vec4(vertex, 1.0);
}