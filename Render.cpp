//
// Created by MacBook on 13/04/2026.
//

#include "Render.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

GLuint vao, vbo_pos, vbo_speed, vbo_quad;

std::string readFile(const std::string& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "Failed to open shader: " << filePath << std::endl;
        exit(-1);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // Проверка ошибок
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader Compilation Error:\n" << infoLog << std::endl;
        exit(-1);
    }

    return shader;
}


void InitRenderData(int numParticles) {
    float quad[] = { -1, -1,  1, -1,  -1, 1,  1, 1 };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // 1. Квадрат
    glGenBuffers(1, &vbo_quad);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_quad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // 2. Позиции
    glGenBuffers(1, &vbo_pos);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
    glBufferData(GL_ARRAY_BUFFER, numParticles * sizeof(Vec2), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vec2), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    // 3. Скорости
    glGenBuffers(1, &vbo_speed);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_speed);
    glBufferData(GL_ARRAY_BUFFER, numParticles * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0); // Отвязываем
}


GLuint CreateShaderProgram(const std::string& vsPath, const std::string& fsPath) {
    std::string vsSource = readFile(vsPath);
    std::string fsSource = readFile(fsPath);

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader Linking Error:\n" << infoLog << std::endl;
        exit(-1); // ВАЖНО
    }
    if (program == 0) {
        std::cerr << "Shader program is 0!" << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}