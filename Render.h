//
// Created by MacBook on 13/04/2026.
//

#ifndef FLUIDSIMULATION_RENDER_H
#define FLUIDSIMULATION_RENDER_H

#include <glad/glad.h>
#include <string>
#include "Common.h"

extern GLuint vao, vbo_pos, vbo_speed, vbo_quad;

void InitRenderData(int numParticles);
GLuint CreateShaderProgram(const std::string& vsPath, const std::string& fsPath);

#endif //FLUIDSIMULATION_RENDER_H
