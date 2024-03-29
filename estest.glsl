#version 320 es

uniform int dim;
uniform int data_cnt;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer dataBuffer {
	double buf[];
} data;

layout(std430, binding = 1) buffer queriesBuffer {
	double buf[];
} queries;

layout(std430, binding = 2) buffer outBuffer {
	float buf[];
} dist;

void main() {
	int query_idx = int(gl_GlobalInvocationID.x);
	int data_idx = int(gl_GlobalInvocationID.y);
	float sum = 0.0f;
	for (int i = 0; i < dim; i++) {
		float q = float(queries.buf[query_idx * dim + i]);
		float d = float(data.buf[data_idx * dim + i]);
		float diff = abs(q - d);
		sum += diff * diff;
	}
	float val = sqrt(sum);
	dist.buf[query_idx * data_cnt + data_idx] = val;
}
