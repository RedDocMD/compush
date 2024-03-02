CPP := if os() == "macos" { "clang++" } else { "clang++-14" }
CFLAGS := "-Wall -std=c++17 " + `pkg-config --cflags glew` + " " + `pkg-config --cflags glfw3` + " " + `pkg-config --cflags libpng`
LIBFLAGS := `pkg-config --libs glew` + " " + `pkg-config --libs glfw3` + " " + `pkg-config --libs libpng`
OPENGL := if os() == "macos" { " -framework OpenGL" } else { "" }
LDFLAGS := LIBFLAGS + OPENGL

raytrace:
	{{CPP}} {{CFLAGS}} -o raytrace raytrace.cpp {{LDFLAGS}}
	./raytrace
