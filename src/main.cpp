// src/main.cpp (stub for Project 6 - Specular)
// Project 6 — Specular Lighting Grid (Linux, C++/OpenGL 3.3 Core)
// Keny: renders 8 cubes (2,4,8,16 | 32,64,128,256) + on-screen labels.
// Deps (Ubuntu): sudo apt install build-essential libglfw3-dev libglew-dev libglm-dev
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static std::string readTextFile(const std::string& path){
    std::ifstream f(path); if(!f){ std::cerr<<"Can't open "<<path<<"\n"; std::exit(1); }
    std::ostringstream ss; ss<<f.rdbuf(); return ss.str();
}
static GLuint compile(GLenum type, const std::string& src){
    GLuint s = glCreateShader(type); const char* c = src.c_str();
    glShaderSource(s,1,&c,nullptr); glCompileShader(s);
    GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){ GLint n=0; glGetShaderiv(s,GL_INFO_LOG_LENGTH,&n);
        std::string log(n,'\0'); glGetShaderInfoLog(s,n,nullptr,log.data());
        std::cerr<<(type==GL_VERTEX_SHADER?"[VERT] ":"[FRAG] ")<<log<<std::endl; std::exit(1); }
    return s;
}
static GLuint link(GLuint vs, GLuint fs){
    GLuint p = glCreateProgram(); glAttachShader(p,vs); glAttachShader(p,fs); glLinkProgram(p);
    GLint ok=0; glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){ GLint n=0; glGetProgramiv(p,GL_INFO_LOG_LENGTH,&n);
        std::string log(n,'\0'); glGetProgramInfoLog(p,n,nullptr,log.data());
        std::cerr<<"[LINK] "<<log<<std::endl; std::exit(1); }
    glDetachShader(p,vs); glDetachShader(p,fs); glDeleteShader(vs); glDeleteShader(fs); return p;
}

// ---------- simple 2D text program (seven-segment triangles, no textures) ----------
static GLuint make2DProgram(){
    const char* vsrc = R"(#version 330 core
        layout(location=0) in vec2 aPos;
        uniform mat4 ortho;
        void main(){ gl_Position = ortho * vec4(aPos,0.0,1.0); }
    )";
    const char* fsrc = R"(#version 330 core
        uniform vec3 uColor; out vec4 FragColor;
        void main(){ FragColor = vec4(uColor,1.0); }
    )";
    return link(compile(GL_VERTEX_SHADER,vsrc), compile(GL_FRAGMENT_SHADER,fsrc));
}
static void addRect(std::vector<float>& V, float x,float y,float w,float h){
    // two triangles (x,y is bottom-left) in pixel space
    float x2=x+w, y2=y+h;
    float t[] = { x,y,  x2,y,  x2,y2,  x,y,  x2,y2,  x,y2 };
    V.insert(V.end(), t, t+12);
}
// map digit -> segments (a,b,c,d,e,f,g)
static void addDigit(std::vector<float>& V, float bx,float by,float s, char d){
    bool a=0,b=0,c=0,dn=0,e=0,f=0,g=0;
    switch(d){
        case '0': a=b=c=dn=e=f=1; break;
        case '1': b=c=1; break;
        case '2': a=b=g=e=dn=1; break;
        case '3': a=b=g=c=dn=1; break;
        case '4': f=g=b=c=1; break;
        case '5': a=f=g=c=dn=1; break;
        case '6': a=f=g=c=dn=e=1; break;
        case '7': a=b=c=1; break;
        case '8': a=b=c=dn=e=f=g=1; break;
        case '9': a=b=c=dn=f=g=1; break;
    }
    float w = s, h = s*1.6f, t = s*0.18f; // thickness
    // segment positions inside the digit box (origin at bx,by)
    if(a)  addRect(V, bx+t,     by+h-t, w-2*t, t);
    if(b)  addRect(V, bx+w-t,   by+h/2+t/2, t, h/2-t);
    if(c)  addRect(V, bx+w-t,   by+t,       t, h/2-t);
    if(dn) addRect(V, bx+t,     by,        w-2*t, t);
    if(e)  addRect(V, bx,       by+t,       t, h/2-t);
    if(f)  addRect(V, bx,       by+h/2+t/2, t, h/2-t);
    if(g)  addRect(V, bx+t,     by+h/2-t/2, w-2*t, t);
}
static void addNumber(std::vector<float>& V, float x,float y,float size, const std::string& txt){
    float advance = size*1.05f;
    for(char ch : txt){
        addDigit(V, x, y, size, ch);
        x += advance;
    }
}
// -----------------------------------------------------------------------------------

int main(){
    // --------------- window/context ----------------
    if(!glfwInit()){ std::cerr<<"glfwInit failed\n"; return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    const int W=1280, H=720;
    GLFWwindow* win = glfwCreateWindow(W,H,"Specular Grid",nullptr,nullptr);
    if(!win){ glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win); glfwSwapInterval(1);
    glewExperimental = GL_TRUE; if(glewInit()!=GLEW_OK){ std::cerr<<"glewInit failed\n"; return 1; }
    glEnable(GL_DEPTH_TEST);

    // --------------- shaders -----------------------
    GLuint prog3D = link(
        compile(GL_VERTEX_SHADER, readTextFile("shaders/phong.vert")),
        compile(GL_FRAGMENT_SHADER, readTextFile("shaders/phong.frag"))
    );
    GLuint prog2D = make2DProgram();

    // --------------- cube geometry -----------------
    float cube[] = {
        // +X
         0.5f,-0.5f,-0.5f,  1,0,0,  0.5f, 0.5f,-0.5f,  1,0,0,  0.5f, 0.5f, 0.5f,  1,0,0,
         0.5f,-0.5f,-0.5f,  1,0,0,  0.5f, 0.5f, 0.5f,  1,0,0,  0.5f,-0.5f, 0.5f,  1,0,0,
        // -X
        -0.5f,-0.5f,-0.5f, -1,0,0, -0.5f, 0.5f, 0.5f, -1,0,0, -0.5f, 0.5f,-0.5f, -1,0,0,
        -0.5f,-0.5f,-0.5f, -1,0,0, -0.5f,-0.5f, 0.5f, -1,0,0, -0.5f, 0.5f, 0.5f, -1,0,0,
        // +Y
        -0.5f, 0.5f,-0.5f,  0,1,0,  0.5f, 0.5f, 0.5f,  0,1,0,  0.5f, 0.5f,-0.5f,  0,1,0,
        -0.5f, 0.5f,-0.5f,  0,1,0, -0.5f, 0.5f, 0.5f,  0,1,0,  0.5f, 0.5f, 0.5f,  0,1,0,
        // -Y
        -0.5f,-0.5f,-0.5f,  0,-1,0,  0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f, 0.5f,  0,-1,0,
        -0.5f,-0.5f,-0.5f,  0,-1,0,  0.5f,-0.5f, 0.5f, 0,-1,0, -0.5f,-0.5f, 0.5f,  0,-1,0,
        // +Z
        -0.5f,-0.5f, 0.5f,  0,0,1,  0.5f, 0.5f, 0.5f,  0,0,1, -0.5f, 0.5f, 0.5f,  0,0,1,
        -0.5f,-0.5f, 0.5f,  0,0,1,  0.5f,-0.5f, 0.5f,  0,0,1,  0.5f, 0.5f, 0.5f,  0,0,1,
        // -Z
        -0.5f,-0.5f,-0.5f, 0,0,-1,  0.5f, 0.5f,-0.5f, 0,0,-1,  0.5f,-0.5f,-0.5f, 0,0,-1,
        -0.5f,-0.5f,-0.5f, 0,0,-1, -0.5f, 0.5f,-0.5f, 0,0,-1,  0.5f, 0.5f,-0.5f, 0,0,-1
    };
    GLuint vao,vbo; glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(cube),cube,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
    glBindVertexArray(0);

    // ---------- uniforms (3D) ----------
    glUseProgram(prog3D);
    GLint uModel   = glGetUniformLocation(prog3D,"model");
    GLint uView    = glGetUniformLocation(prog3D,"view");
    GLint uProj    = glGetUniformLocation(prog3D,"projection");
    GLint uViewPos = glGetUniformLocation(prog3D,"viewPos");
    GLint uMatAmb  = glGetUniformLocation(prog3D,"material.ambient");
    GLint uMatDiff = glGetUniformLocation(prog3D,"material.diffuse");
    GLint uMatSpec = glGetUniformLocation(prog3D,"material.specular");
    GLint uMatSh   = glGetUniformLocation(prog3D,"material.shininess");
    GLint uLightPos= glGetUniformLocation(prog3D,"light.position");
    GLint uLA      = glGetUniformLocation(prog3D,"light.ambient");
    GLint uLD      = glGetUniformLocation(prog3D,"light.diffuse");
    GLint uLS      = glGetUniformLocation(prog3D,"light.specular");

    // Material: warm orange; Lighting: neutral white
    glUniform3f(uMatAmb,  0.20f, 0.10f, 0.06f);
    glUniform3f(uMatDiff, 1.00f, 0.50f, 0.31f);
    glUniform3f(uMatSpec, 0.55f, 0.55f, 0.55f);

    // Camera
    glm::vec3 cam(0.0f, 0.0f, 8.8f);
    glm::mat4 view = glm::lookAt(cam, glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)W/(float)H, 0.1f, 100.0f);
    glUniformMatrix4fv(uView,1,GL_FALSE,glm::value_ptr(view));
    glUniformMatrix4fv(uProj,1,GL_FALSE,glm::value_ptr(proj));
    glUniform3f(uViewPos, cam.x, cam.y, cam.z);

    // Single point light—slightly left and above camera so the hotspot sits a bit left-inside
    glm::vec3 lightPos(-2.2f, 1.9f, 3.6f);
    glUniform3f(uLightPos, lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(uLA, 0.18f, 0.18f, 0.18f);
    glUniform3f(uLD, 0.92f, 0.92f, 0.92f);
    glUniform3f(uLS, 1.00f, 1.00f, 1.00f);

    // Cube transforms (fixed slight tilt to match look)
    glm::mat4 base(1.0f);
    base = glm::rotate(base, glm::radians(-14.0f), glm::vec3(1,0,0)); // tip back a little
    base = glm::rotate(base, glm::radians(-28.0f), glm::vec3(0,1,0)); // yaw left

    const float cols[4] = {-4.0f, -1.3f, 1.3f, 4.0f};
    const float rows[2] = { 1.9f, -1.9f};
    const float scale   = 1.36f;

    // Row-major labels & shininess
    const char* labels[8] = {"2","4","8","16","32","64","128","256"};
    const float shininess[8] = {2,4,8,16,32,64,128,256};

    // ---------- 2D label resources ----------
    GLuint vao2D, vbo2D; glGenVertexArrays(1,&vao2D); glGenBuffers(1,&vbo2D);
    glBindVertexArray(vao2D); glBindBuffer(GL_ARRAY_BUFFER,vbo2D);
    glBufferData(GL_ARRAY_BUFFER, 1, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
    glBindVertexArray(0);
    GLint uOrtho = glGetUniformLocation(prog2D,"ortho");
    GLint uColor = glGetUniformLocation(prog2D,"uColor");
    glm::mat4 ortho = glm::ortho(0.0f,(float)W, 0.0f,(float)H);

    // Pre-chosen screen positions for labels (centered under cubes)
    const float cx[4] = {160, 480, 800, 1120}; // pixel centers per column
    const float yTop  = 330; // y for top row labels
    const float yBot  =  48; // y for bottom row labels
    const float sizeTop = 28.0f, sizeBot = 34.0f; // bottom row slightly larger like ref

    // --------------- loop --------------------
    while(!glfwWindowShouldClose(win)){
        glfwPollEvents();
        glViewport(0,0,W,H);
        glClearColor(0.18f,0.20f,0.22f,1.0f); // dark gray background matching the sample
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        // draw cubes
        glUseProgram(prog3D);
        glBindVertexArray(vao);
        for(int i=0;i<8;++i){
            int r=(i<4)?0:1, c=(i&3);
            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3(cols[c], rows[r], 0));
            model = model * base;
            model = glm::scale(model, glm::vec3(scale));
            glUniformMatrix4fv(uModel,1,GL_FALSE,glm::value_ptr(model));
            glUniform1f(uMatSh, shininess[i]); // << only difference per cube
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glBindVertexArray(0);

        // draw labels (2D overlay)
        glDisable(GL_DEPTH_TEST);
        glUseProgram(prog2D);
        glUniformMatrix4fv(uOrtho,1,GL_FALSE,glm::value_ptr(ortho));
        glUniform3f(uColor, 0.93f,0.93f,0.93f); // white text

        glBindVertexArray(vao2D);
        for(int i=0;i<8;++i){
            int r=(i<4)?0:1, c=(i&3);
            float size = (r==0)? sizeTop : sizeBot;
            // approximate text width so we can center it
            float approxWidth = size * (std::string(labels[i]).size()) * 1.05f;
            float x = cx[c] - approxWidth*0.5f;
            float y = (r==0? yTop : yBot);
            std::vector<float> V; V.reserve(6*6*8);
            addNumber(V, x, y, size, labels[i]);
            glBindBuffer(GL_ARRAY_BUFFER, vbo2D);
            glBufferData(GL_ARRAY_BUFFER, V.size()*sizeof(float), V.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(V.size()/2));
        }
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(win);
    }

    glDeleteBuffers(1,&vbo); glDeleteVertexArrays(1,&vao);
    glDeleteBuffers(1,&vbo2D); glDeleteVertexArrays(1,&vao2D);
    glDeleteProgram(prog3D); glDeleteProgram(prog2D);
    glfwDestroyWindow(win); glfwTerminate();
    return 0;
}
