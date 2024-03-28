#version 320 es

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer dataBuffer {
	double buf[];
} data;

layout(std430, binding = 1) buffer queriesBuffer {
	double buf[];
} queries;

layout(std430, binding = 2) buffer outBuffer {
	float data[];
} dist;

uniform int dim;
uniform int queries_cnt;
uniform int data_cnt;

void main() {
}
