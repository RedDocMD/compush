CPP := "clang++-14"

raytrace:
	{{CPP}} -I/usr/include/libpng16 -Wall -std=c++17 -o raytrace raytrace.cpp -lglfw -lpng16 -L/usr/lib64 -lGLEW -lGL -lX11 -lGLU
	./raytrace
