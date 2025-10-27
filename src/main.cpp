// CST-310 Project 6 — Specular Grid (final, de-duplicated)
// • All cubes = exact “64” look; only shininess differs.
// • Top row never shows bottom faces (stronger pitch).
// • One selection of shaders, one VAO flag (no redeclarations).

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static void die(const char* m){ std::fprintf(stderr,"FATAL: %s\n",m); std::exit(1); }
static void glfwErr(int c,const char* d){ std::fprintf(stderr,"GLFW %d: %s\n",c,d); }

struct Flavor{int glMajor,glMinor,glsl;}; enum{GLSL_330=330,GLSL_130=130,GLSL_120=120};
static GLFWwindow* tryWin(int W,int H,int M,int m,int core){
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_CLIENT_API,GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,M);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,m);
  if(core) glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_DEPTH_BITS,24); glfwWindowHint(GLFW_STENCIL_BITS,8); glfwWindowHint(GLFW_SAMPLES,0);
  return glfwCreateWindow(W,H,"Specular Grid (CST-310)",nullptr,nullptr);
}
static Flavor makeCtx(GLFWwindow*& w,int W,int H){
  struct T{int M,m,c,glsl;} t[]={{3,3,1,330},{3,3,0,330},{3,0,0,130},{2,1,0,120}};
  for(auto& k:t) if((w=tryWin(W,H,k.M,k.m,k.c))) return {k.M,k.m,k.glsl};
  return {0,0,0};
}

// ---------- seven-seg overlay ----------
static void addRect(std::vector<float>& V,float x,float y,float w,float h){
  float X=x+w,Y=y+h; float t[]={x,y, X,y, X,Y,  x,y, X,Y, x,Y}; V.insert(V.end(),t,t+12);
}
static void addDigit(std::vector<float>& V,float bx,float by,float s,char d){
  bool a=0,b=0,c=0,dn=0,e=0,f=0,g=0;
  switch(d){case '0':a=b=c=dn=e=f=1;break;case '1':b=c=1;break;case '2':a=b=g=e=dn=1;break;
  case '3':a=b=g=c=dn=1;break;case '4':f=g=b=c=1;break;case '5':a=f=g=c=dn=1;break;
  case '6':a=f=g=c=dn=e=1;break;case '7':a=b=c=1;break;case '8':a=b=c=dn=e=f=g=1;break;
  case '9':a=b=c=dn=f=g=1;break;}
  float w=s,h=s*1.6f,t=s*0.18f;
  if(a)  addRect(V,bx+t,by+h-t,w-2*t,t);
  if(b)  addRect(V,bx+w-t,by+h/2+t/2,t,h/2-t);
  if(c)  addRect(V,bx+w-t,by+t,t,h/2-t);
  if(dn) addRect(V,bx+t,by,w-2*t,t);
  if(e)  addRect(V,bx,by+t,t,h/2-t);
  if(f)  addRect(V,bx,by+h/2+t/2,t,h/2-t);
  if(g)  addRect(V,bx+t,by+h/2-t/2,w-2*t,t);
}
static void addNumber(std::vector<float>& V,float x,float y,float s,const std::string& txt){
  float adv=s*1.12f; for(char ch:txt){ addDigit(V,x,y,s,ch); x+=adv; }
}

// ---------- shader utils ----------
static GLuint comp(GLenum type,const std::string& src){
  GLuint s=glCreateShader(type); const char* c=src.c_str(); glShaderSource(s,1,&c,nullptr); glCompileShader(s);
  GLint ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
  if(!ok){ GLint n; glGetShaderiv(s,GL_INFO_LOG_LENGTH,&n); std::string log(n?n:1,'\0'); glGetShaderInfoLog(s,n,nullptr,log.data()); die(log.c_str()); }
  return s;
}
static GLuint link(GLuint vs,GLuint fs,bool bind){
  GLuint p=glCreateProgram(); if(bind){ glBindAttribLocation(p,0,"aPos"); glBindAttribLocation(p,1,"aNormal"); }
  glAttachShader(p,vs); glAttachShader(p,fs); glLinkProgram(p);
  GLint ok; glGetProgramiv(p,GL_LINK_STATUS,&ok);
  if(!ok){ GLint n; glGetProgramiv(p,GL_INFO_LOG_LENGTH,&n); std::string log(n?n:1,'\0'); glGetProgramInfoLog(p,n,nullptr,log.data()); die(log.c_str()); }
  glDeleteShader(vs); glDeleteShader(fs); return p;
}

// ---------- shaders (Phong + overlay, 3 flavors) ----------
struct Sh{std::string v,f; bool bind=false;};

static Sh phong330(){return{R"(#version 330 core
layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal;
uniform mat4 model,view,projection; uniform mat3 normalMatrix;
out vec3 FragPos; out vec3 Normal;
void main(){ FragPos=vec3(model*vec4(aPos,1)); Normal=normalMatrix*aNormal; gl_Position=projection*view*vec4(FragPos,1); })",
R"(#version 330 core
struct Material{vec3 ambient;vec3 diffuse;vec3 specular;float shininess;};
struct Light{vec3 position;vec3 ambient;vec3 diffuse;vec3 specular;};
in vec3 FragPos; in vec3 Normal; out vec4 FragColor;
uniform Material material; uniform Light light; uniform vec3 viewPos;
void main(){
  vec3 N=normalize(Normal), L=normalize(light.position-FragPos), V=normalize(viewPos-FragPos);
  float diff=max(dot(N,L),0.0); vec3 R=reflect(-L,N);
  float spec=pow(max(dot(V,R),0.0), material.shininess);
  vec3 col=light.ambient*material.ambient + light.diffuse*(diff*material.diffuse) + light.specular*(spec*material.specular);
  FragColor=vec4(col,1.0);
})"};}
static Sh phong130(){Sh s; s.bind=true; s.v=R"(#version 130
in vec3 aPos; in vec3 aNormal; uniform mat4 model,view,projection; uniform mat3 normalMatrix;
out vec3 FragPos; out vec3 Normal;
void main(){ FragPos=vec3(model*vec4(aPos,1)); Normal=normalMatrix*aNormal; gl_Position=projection*view*vec4(FragPos,1); })";
s.f=R"(#version 130
struct Material{vec3 ambient;vec3 diffuse;vec3 specular;float shininess;};
struct Light{vec3 position;vec3 ambient;vec3 diffuse;vec3 specular;};
in vec3 FragPos; in vec3 Normal; out vec4 FragColor; uniform Material material; uniform Light light; uniform vec3 viewPos;
void main(){ vec3 N=normalize(Normal), L=normalize(light.position-FragPos), V=normalize(viewPos-FragPos);
 float diff=max(dot(N,L),0.0); vec3 R=reflect(-L,N); float spec=pow(max(dot(V,R),0.0),material.shininess);
 vec3 col=light.ambient*material.ambient + light.diffuse*(diff*material.diffuse) + light.specular*(spec*material.specular);
 FragColor=vec4(col,1); })"; return s;}
static Sh phong120(){Sh s; s.bind=true; s.v=R"(#version 120
attribute vec3 aPos; attribute vec3 aNormal; uniform mat4 model,view,projection; uniform mat3 normalMatrix;
varying vec3 vPos; varying vec3 vN;
void main(){ vPos=vec3(model*vec4(aPos,1)); vN=normalMatrix*aNormal; gl_Position=projection*view*vec4(vPos,1); })";
s.f=R"(#version 120
struct Material{vec3 ambient;vec3 diffuse;vec3 specular;float shininess;};
struct Light{vec3 position;vec3 ambient;vec3 diffuse;vec3 specular;};
varying vec3 vPos; varying vec3 vN; uniform Material material; uniform Light light; uniform vec3 viewPos;
void main(){ vec3 N=normalize(vN), L=normalize(light.position-vPos), V=normalize(viewPos-vPos);
 float diff=max(dot(N,L),0.0); vec3 R=reflect(-L,N); float spec=pow(max(dot(V,R),0.0),material.shininess);
 vec3 col=light.ambient*material.ambient + light.diffuse*(diff*material.diffuse) + light.specular*(spec*material.specular);
 gl_FragColor=vec4(col,1); })"; return s;}

static Sh ov330(){return{R"(#version 330 core
layout(location=0) in vec2 aPos; uniform mat4 ortho; void main(){ gl_Position=ortho*vec4(aPos,0,1); })",
R"(#version 330 core
uniform vec3 uColor; out vec4 FragColor; void main(){ FragColor=vec4(uColor,1); })"};}
static Sh ov130(){Sh s; s.bind=true; s.v=R"(#version 130
in vec2 aPos; uniform mat4 ortho; void main(){ gl_Position=ortho*vec4(aPos,0,1); })";
s.f=R"(#version 130
uniform vec3 uColor; out vec4 FragColor; void main(){ FragColor=vec4(uColor,1); })"; return s;}
static Sh ov120(){Sh s; s.bind=true; s.v=R"(#version 120
attribute vec2 aPos; uniform mat4 ortho; void main(){ gl_Position=ortho*vec4(aPos,0,1); })";
s.f=R"(#version 120
uniform vec3 uColor; void main(){ gl_FragColor=vec4(uColor,1); })"; return s;}

int main(){
  glfwSetErrorCallback(glfwErr);
  if(!glfwInit()) die("glfwInit");
  const int W=1280,H=720;
  GLFWwindow* win=nullptr; Flavor fl=makeCtx(win,W,H); if(!win) die("no GL context");
  glfwMakeContextCurrent(win); glfwSwapInterval(1);
  glewExperimental=GL_TRUE; if(glewInit()!=GLEW_OK) die("glewInit");
  glGetError(); glEnable(GL_DEPTH_TEST);

  // choose shaders ONCE
  Sh P=(fl.glsl>=GLSL_330)?phong330():(fl.glsl>=GLSL_130?phong130():phong120());
  Sh O=(fl.glsl>=GLSL_330)?ov330()    :(fl.glsl>=GLSL_130?ov130()    :ov120());
  GLuint prog3D=link(comp(GL_VERTEX_SHADER,P.v),comp(GL_FRAGMENT_SHADER,P.f),P.bind);
  GLuint prog2D=link(comp(GL_VERTEX_SHADER,O.v),comp(GL_FRAGMENT_SHADER,O.f),O.bind);

  // one VAO flag used for both 3D and 2D
  bool haveVAO=(GLEW_VERSION_3_0||GLEW_ARB_vertex_array_object);

  // cube geometry (pos3, normal3)
  float cube[]={
    0.5f,-0.5f,-0.5f,1,0,0,  0.5f,0.5f,-0.5f,1,0,0,  0.5f,0.5f,0.5f,1,0,0,
    0.5f,-0.5f,-0.5f,1,0,0,  0.5f,0.5f,0.5f,1,0,0,  0.5f,-0.5f,0.5f,1,0,0,
   -0.5f,-0.5f,-0.5f,-1,0,0, -0.5f,0.5f,0.5f,-1,0,0, -0.5f,0.5f,-0.5f,-1,0,0,
   -0.5f,-0.5f,-0.5f,-1,0,0, -0.5f,-0.5f,0.5f,-1,0,0, -0.5f,0.5f,0.5f,-1,0,0,
   -0.5f,0.5f,-0.5f,0,1,0,  0.5f,0.5f,0.5f,0,1,0,  0.5f,0.5f,-0.5f,0,1,0,
   -0.5f,0.5f,-0.5f,0,1,0, -0.5f,0.5f,0.5f,0,1,0,  0.5f,0.5f,0.5f,0,1,0,
   -0.5f,-0.5f,-0.5f,0,-1,0, 0.5f,-0.5f,-0.5f,0,-1,0, 0.5f,-0.5f,0.5f,0,-1,0,
   -0.5f,-0.5f,-0.5f,0,-1,0, 0.5f,-0.5f,0.5f,0,-1,0, -0.5f,-0.5f,0.5f,0,-1,0,
   -0.5f,-0.5f,0.5f,0,0,1,  0.5f,0.5f,0.5f,0,0,1, -0.5f,0.5f,0.5f,0,0,1,
   -0.5f,-0.5f,0.5f,0,0,1,  0.5f,-0.5f,0.5f,0,0,1,  0.5f,0.5f,0.5f,0,0,1,
   -0.5f,-0.5f,-0.5f,0,0,-1, 0.5f,0.5f,-0.5f,0,0,-1,  0.5f,-0.5f,-0.5f,0,0,-1,
   -0.5f,-0.5f,-0.5f,0,0,-1, -0.5f,0.5f,-0.5f,0,0,-1, 0.5f,0.5f,-0.5f,0,0,-1
  };
  GLuint vbo=0, vao=0; glGenBuffers(1,&vbo); glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBufferData(GL_ARRAY_BUFFER,sizeof(cube),cube,GL_STATIC_DRAW);
  if(haveVAO){ glGenVertexArrays(1,&vao); glBindVertexArray(vao); }
  glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
  glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
  if(haveVAO) glBindVertexArray(0);

  // 3D uniforms — single pose/light shared by ALL cubes (the “64” template)
  glUseProgram(prog3D);
  GLint uModel=glGetUniformLocation(prog3D,"model");
  GLint uView =glGetUniformLocation(prog3D,"view");
  GLint uProj =glGetUniformLocation(prog3D,"projection");
  GLint uNrm  =glGetUniformLocation(prog3D,"normalMatrix");
  GLint uVP   =glGetUniformLocation(prog3D,"viewPos");
  GLint uA=glGetUniformLocation(prog3D,"material.ambient");
  GLint uD=glGetUniformLocation(prog3D,"material.diffuse");
  GLint uS=glGetUniformLocation(prog3D,"material.specular");
  GLint uSh=glGetUniformLocation(prog3D,"material.shininess");
  GLint uLP=glGetUniformLocation(prog3D,"light.position");
  GLint uLA=glGetUniformLocation(prog3D,"light.ambient");
  GLint uLD=glGetUniformLocation(prog3D,"light.diffuse");
  GLint uLS=glGetUniformLocation(prog3D,"light.specular");

  glUniform3f(uA,0.20f,0.10f,0.06f);
  glUniform3f(uD,1.00f,0.50f,0.31f);
  glUniform3f(uS,0.55f,0.55f,0.55f);

  glm::vec3 cam(0,0,10.0f);
  glm::mat4 view=glm::lookAt(cam,glm::vec3(0,0,0),glm::vec3(0,1,0));
  glm::mat4 proj=glm::ortho(-7.1f, 7.1f, -4.0f, 4.0f, -100.0f, 100.0f);
  glUniformMatrix4fv(uView,1,GL_FALSE,glm::value_ptr(view));
  glUniformMatrix4fv(uProj,1,GL_FALSE,glm::value_ptr(proj));
  glUniform3f(uVP,cam.x,cam.y,cam.z);

  glUniform3f(uLA,0.16f,0.16f,0.16f);
  glUniform3f(uLD,0.95f,0.95f,0.95f);
  glUniform3f(uLS,1,1,1);

  // compute relative light for "64" look with new pitch
  const float PITCH = 0.0f;
  const float YAW = 27.5f;
  const float ROLL = -1.5f;
  const float scale = 1.62f;
  const float cols[4] = {-4.7f, -1.75f, 1.75f, 4.7f};
  const float rows[2] = {2.12f, -2.35f};
  glm::mat4 new_base(1.0f);
  new_base = glm::rotate(new_base, glm::radians(PITCH), glm::vec3(1, 0, 0));
  new_base = glm::rotate(new_base, glm::radians(YAW), glm::vec3(0, 1, 0));
  new_base = glm::rotate(new_base, glm::radians(ROLL), glm::vec3(0, 0, 1));
  glm::mat4 old_base(1.0f);
  old_base = glm::rotate(old_base, glm::radians(-16.0f), glm::vec3(1, 0, 0));
  old_base = glm::rotate(old_base, glm::radians(YAW), glm::vec3(0, 1, 0));
  old_base = glm::rotate(old_base, glm::radians(ROLL), glm::vec3(0, 0, 1));
  glm::mat4 inv_old = glm::inverse(old_base);
  glm::vec3 old_rel(4.75f, 4.05f, 3.9f);
  glm::mat4 transform = new_base * inv_old;
  glm::vec3 ref_light_rel = glm::vec3(transform * glm::vec4(old_rel, 0.0f));

  const float shininess[8]={2,4,8,16,32,64,128,256};
  const char* labels[8]={"2","4","8","16","32","64","128","256"};

  // 2D overlay resources (created ONCE)
  GLuint vbo2=0, vao2=0; glGenBuffers(1,&vbo2); glBindBuffer(GL_ARRAY_BUFFER,vbo2);
  glBufferData(GL_ARRAY_BUFFER,1,nullptr,GL_DYNAMIC_DRAW);
  if(haveVAO){ glGenVertexArrays(1,&vao2); glBindVertexArray(vao2); }
  glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
  if(haveVAO) glBindVertexArray(0);
  glUseProgram(prog2D);
  GLint uOrtho=glGetUniformLocation(prog2D,"ortho");
  GLint uColor=glGetUniformLocation(prog2D,"uColor");
  glm::mat4 ortho2d=glm::ortho(0.0f,(float)W,0.0f,(float)H);
  const float cx[4]={160,480,800,1120};
  const float yTop=300.0f;     // adjusted for space
  const float yBot=20.0f;      // adjusted for space
  const float sizeTop=26.0f, sizeBot=30.0f;

  // render loop
  while(!glfwWindowShouldClose(win)){
    glfwPollEvents();
    glViewport(0,0,W,H);
    glClearColor(0.19f,0.21f,0.23f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // cubes (ALL share the same base → exact “64” copy; only shininess differs)
    glUseProgram(prog3D);
    if(haveVAO) glBindVertexArray(vao); else glBindBuffer(GL_ARRAY_BUFFER,vbo);
    for(int i=0;i<8;++i){
      int r=(i<4)?0:1, c=(i&3);
      glm::vec3 cube_pos(cols[c],rows[r],0);
      glm::vec3 this_light = cube_pos + ref_light_rel;
      glUniform3f(uLP, this_light.x, this_light.y, this_light.z);

      glm::mat4 M=glm::translate(glm::mat4(1.0f), cube_pos);
      M = M * new_base; M = glm::scale(M, glm::vec3(scale));
      glm::mat3 N=glm::mat3(glm::transpose(glm::inverse(M)));
      glUniformMatrix4fv(uModel,1,GL_FALSE,glm::value_ptr(M));
      glUniformMatrix3fv(uNrm, 1,GL_FALSE,glm::value_ptr(N));
      glUniform1f(uSh, shininess[i]);              // ONLY per-cube difference
      glDrawArrays(GL_TRIANGLES,0,36);
    }
    if(haveVAO) glBindVertexArray(0);

    // labels
    glDisable(GL_DEPTH_TEST);
    glUseProgram(prog2D);
    glUniformMatrix4fv(uOrtho,1,GL_FALSE,glm::value_ptr(ortho2d));
    glUniform3f(uColor,0.93f,0.93f,0.93f);
    if(haveVAO) glBindVertexArray(vao2); else glBindBuffer(GL_ARRAY_BUFFER,vbo2);
    for(int i=0;i<8;++i){
      int r=(i<4)?0:1, c=(i&3);
      float size=(r==0)?sizeTop:sizeBot;
      float width=size*(std::string(labels[i]).size())*1.12f;
      float x=cx[c]-width*0.5f;
      float y=(r==0? yTop : yBot);
      std::vector<float> V; V.reserve(6*6*8);
      addNumber(V,x,y,size,labels[i]);
      glBufferData(GL_ARRAY_BUFFER,V.size()*sizeof(float),V.data(),GL_DYNAMIC_DRAW);
      glDrawArrays(GL_TRIANGLES,0,(GLsizei)(V.size()/2));
    }
    if(haveVAO) glBindVertexArray(0); glEnable(GL_DEPTH_TEST);

    glfwSwapBuffers(win);
  }

  glDeleteBuffers(1,&vbo); glDeleteBuffers(1,&vbo2);
  if(haveVAO){ glDeleteVertexArrays(1,&vao); glDeleteVertexArrays(1,&vao2); }
  glDeleteProgram(prog3D); glDeleteProgram(prog2D);
  glfwDestroyWindow(win); glfwTerminate();
  return 0;
}