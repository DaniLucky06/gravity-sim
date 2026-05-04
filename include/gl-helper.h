#include <glad/glad.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <SFML/Graphics.hpp>

std::string readFile(const std::string& fileName)
{
    std::ifstream file(fileName);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + fileName);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int createShader(uint &shader, GLenum shaderType, std::string fileName)
{
    std::string shaderString = readFile(fileName); // read string
    const char *shaderStringPtr = shaderString.c_str(); // get the pointer to it

    shader = glCreateShader(shaderType); // initialize shader
    glShaderSource(shader, 1, &shaderStringPtr, NULL); // assign the string to it
    glCompileShader(shader); // compile it

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "--- ERROR COMPILING SHADER ---\n" << infoLog << std::endl;
    }
    return success;
};

int createProgram(uint &program, uint shader)
{
    program = glCreateProgram();
    
    glAttachShader(program, shader);

    glLinkProgram(program);
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "--- ERROR LINKING PROGRAM ---\n" << infoLog << std::endl;
    }
    return success;
};
