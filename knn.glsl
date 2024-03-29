#version 430
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(rg32f, binding = 0) uniform image2D data;
layout(rg32f, binding = 1) uniform image2D queries;
layout(rg32f, binding = 2) uniform image2D dist;

double join(in vec2 fv) {
	return double(fv.x) + double(fv.y);
}

vec2 split(in double a) {
	const double SPLITTER = (1 << 29) + 1;
	double t = a * SPLITTER;
	double t_hi = t - (t - a);
	double t_lo = a - t_hi;
	return vec2(float(t_lo), float(t_hi));
}

void main() {
	ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
	int dim = imageSize(data).x;
	double sum = 0;
	for (int i = 0; i < dim; i++) {
		vec2 qv = imageLoad(queries, ivec2(i, coord.x)).xy;
		vec2 dv = imageLoad(data, ivec2(i, coord.y)).xy;
		double qvd = join(qv);
		double dvd = join(dv);
		double diff = abs(qvd - dvd);
		sum += diff * diff;
	}
	double val = sqrt(sum);
	vec2 val_vec = split(val);
	vec4 pixel = vec4(val_vec.x, val_vec.y, 0, 0);
	imageStore(dist, coord, pixel);
}
