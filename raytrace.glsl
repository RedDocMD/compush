#version 430
layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba8ui, binding = 0) uniform uimage2D img_output;

void main() {
	uvec4 pixel = uvec4(0, 0, 0, 255);
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

	pixel.x = pixel_coords.x % 255;
	pixel.y = pixel_coords.y % 255;

	imageStore(img_output, pixel_coords, pixel);
}
