#pragma once

void dump_gl_error (const char* file, int line);

#define check_gl_error() dump_gl_error (__FILE__, __LINE__)
