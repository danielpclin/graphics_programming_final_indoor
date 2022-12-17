#pragma once

#include "GL/glew.h"

// forward declaration of functions
void printGLContextInfo();

// print OpenGL context related information
void printGLContextInfo()
{
	printf("GL_VENDOR: %s\n", glGetString (GL_VENDOR));
	printf("GL_RENDERER: %s\n", glGetString (GL_RENDERER));
	printf("GL_VERSION: %s\n", glGetString (GL_VERSION));
	printf("GL_SHADING_LANGUAGE_VERSION: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));
}
