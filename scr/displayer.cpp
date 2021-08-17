#include <GL/glew.h>
#include "displayer.h"
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <time.h>
#include <sys/timeb.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifndef M_PI
#define M_PI (acos(-1.))
#endif

#ifndef GLUT_WHEEL_UP
#define GLUT_WHEEL_UP 0x0003
#endif

#ifndef GLUT_WHEEL_DOWN
#define GLUT_WHEEL_DOWN 0x0004
#endif

constexpr char* YAML_CONFIG = R"(data.yaml)";

float  tex_width = 256, tex_height = 256;
Displayer Displayer::instance_;
int Displayer::width_ = 860, Displayer::height_ = 640;

std::map<unsigned char, void(*)(int, int)> Displayer::keyboard_functions_;
int Displayer::mouse_down_button_ = 0, Displayer::mouse_down_x_ = 0,
Displayer::mouse_down_y_ = 0;
double Displayer::mouse_down_a_ = 0, Displayer::mouse_down_b_ = 0;
double Displayer::rotate_a_ = M_PI / 4, Displayer::rotate_b_ = M_PI / 4;
double Displayer::translate_a_ = 0, Displayer::translate_b_ = 0;
double Displayer::modelSize = 1;

GLdouble rotate_mat[16];
GLfloat light_x_ = 0.f, light_y_ = 0.f, light_z_ = 0.9f;

int Displayer::positionShader;
int Displayer::skyboxShader;
int Displayer::modelShader;
int Displayer::oceanTexShader;
int Displayer::oceanShader;

Camera Displayer::camera(glm::vec3(-18.32, 5.42, 11.35), glm::vec3(0.0f, 1.0f, 0.0f),-50,-20.4);
unsigned Displayer::skyboxVAO=0;
unsigned Displayer::skyboxTex=0;

Model Displayer::boat;
Model Displayer::car;
Model Displayer::jy01;
Model Displayer::jq01;
Model Displayer::jd01;
Model Displayer::plb01;

const float far_plane = 50.f;
const float near_plane = 1.f;
const float board_height = -1.055f;
const float board_height_eps = 0.05f;
YAML::Node Displayer::conf;
bool flag_selected = false;
glm::fvec4 selected(-0.8, board_height, 0,1);
unsigned int fbo;

vector<float> buff_data;

std::vector<Shader> Displayer::shaders;
std::vector<GLuint> Displayer::texs;
GLuint Displayer::vao;

Fps fps;
float time_now;

void GLAPIENTRY
MessageCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{
    if (type != 0x8251)
    {
        fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
    }
}

void Displayer::init(int& argc, char** argv, const char* title, int width,
    int height) {
    width_ = width;
    height_ = height;
    for (int i = 0; i < argc; i++) {
        std::cerr << argv[i] << std::endl;
    }
    argc--;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(width_, height_);
    glutCreateWindow(title);

    glewInit();
    conf = YAML::LoadFile(YAML_CONFIG);

    initCallBackFunctions();

    initModels();
    initSkybox();
    initShader();
    initParameters();
}

void Displayer::runLoop() {
    glutMainLoop();
}

void Displayer::bindKey(unsigned char key, void function(int, int)) {
    keyboard_functions_.insert_or_assign(key, function);
}


void Displayer::initCallBackFunctions() {
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    glutReshapeFunc(CallBackReshape);
    glutMouseFunc(CallBackMouse);
    glutMotionFunc(CallBackMotion);
    glutKeyboardFunc(CallBackKeyboard);

    glutDisplayFunc(CallBackDisplay);

}


void Displayer::initParameters() {
    GLfloat white_light[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat lmodel_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
    glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lmodel_ambient);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);

    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glPolygonOffset(1.0, 2.0);
    glDepthFunc(GL_LESS);
    glShadeModel(GL_SMOOTH);
    glPointSize(5);
    glEnable(GL_MULTISAMPLE);
    /*
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    */
    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f, 0.0f,  1.0f,  0.0f, 1.0f, //left_up
        -1.0f, 0.0f, -1.0f,  0.0f, 0.0f, //left_bottom
         1.0f, 0.0f, -1.0f,  1.0f, 0.0f, //right_bottom
              
        -1.0f, 0.0f,  1.0f,  0.0f, 1.0f, //left_up
         1.0f, 0.0f, -1.0f,  1.0f, 0.0f, //right_bottom
         1.0f, 0.0f,  1.0f,  1.0f, 1.0f, //right_up
    };
    /*glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/
    unsigned int VBO;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &VBO);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    unsigned int rbo, rbo_d;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, width_, height_);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);

    glGenRenderbuffers(1, &rbo_d);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_d);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_STENCIL, width_, height_);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_d);

    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    buff_data = vector<float>(width_ * height_ * 4);
}


void Displayer::CallBackReshape(int width, int height) {
    width_ = width;
    height_ = height;
    glViewport(0, 0, width_, height_);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    unsigned int rbo, rbo_d;
    glDeleteRenderbuffers(1, &rbo);
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, width_, height_);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);

    glDeleteRenderbuffers(1, &rbo_d);
    glGenRenderbuffers(1, &rbo_d);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_d);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_STENCIL, width_, height_);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_d);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    buff_data = vector<float>(width_ * height_ * 4);
    glutPostRedisplay();
}

void Displayer::CallBackMouse(int button, int status, int x, int y) {
    if (status == GLUT_UP) {
        if (button == GLUT_WHEEL_UP) {
            camera.ProcessMouseScroll(1);
        }
        else if (button == GLUT_WHEEL_DOWN) {
            camera.ProcessMouseScroll(-1);
        }
    }
    else {
        if (button == GLUT_LEFT_BUTTON) {
            mouse_down_button_ = GLUT_LEFT_BUTTON;
            mouse_down_x_ = x;
            mouse_down_y_ = y;

        }
        else if (button == GLUT_RIGHT_BUTTON) {
            mouse_down_button_ = GLUT_RIGHT_BUTTON;
            mouse_down_x_ = x;
            mouse_down_y_ = y;
            flag_selected = true;
        }
    }
    glutPostRedisplay();
}

void Displayer::CallBackKeyboard(unsigned char key, int x, int y) {
    auto function = keyboard_functions_.find(key);
    if (function != keyboard_functions_.end()) {
        function->second(x, y);
        glutPostRedisplay();
    }
    else {
        std::cerr << "Key \'" << key << "\'" << " is not bound." << std::endl;
    }
}

void Displayer::drawModels(Shader &shader, bool draw_position )
{
    if (!draw_position)
        shader.use();
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)width_ / (float)height_, near_plane, far_plane);

    model = glm::translate(model, glm::fvec3(0, 0, 0));
    model = glm::rotate(model, float(M_PI * 0.5), glm::fvec3(-1, 0, 0));
    model = glm::rotate(model, float(M_PI * 0.5), glm::fvec3(0, 0, 1));
    model = glm::scale(model, glm::fvec3(20, 20, 20));
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    boat.Draw(shader);

    if (!draw_position)
    {
        car.processMove(selected);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::fvec3(0, 0.45, 0));
        model = model * car.getTransformMat();
        model = glm::scale(model, glm::fvec3(2, 2, 2));
        model = model * car.getRotateMat();
        //model = glm::rotate(model, float(M_PI * 0.5), glm::fvec3(-1, 0, 0));
       // model = glm::rotate(model, float(M_PI * 0.5), glm::fvec3(0, 0, 1));
        //model = glm::scale(model, glm::fvec3(0.01, 0.01, 0.01));
        shader.setMat4("model", model);
        car.Draw(shader);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::fvec3(5.363, board_height+ board_height_eps, -1.7));
        model = glm::scale(model, glm::fvec3(1, 1, 1));
        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        jy01.Draw(shader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::fvec3(1, board_height + board_height_eps, -1.7));
        model = glm::scale(model, glm::fvec3(1, 1, 1));
        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        jd01.Draw(shader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::fvec3(7.36304, board_height + board_height_eps, 3.36758));
        model = glm::scale(model, glm::fvec3(1, 1, 1));
        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        jq01.Draw(shader);


        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::fvec3(-4.92105, board_height + board_height_eps, 0.638822));
        model = glm::scale(model, glm::fvec3(1, 1, 1));
        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        plb01.Draw(shader);


        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::fvec3(-4.92105, board_height + board_height_eps, 0.638822));
        model = glm::scale(model, glm::fvec3(1, 1, 1));
        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        plb01.Draw(shader);

        glBlendFunc(GL_ONE, GL_ZERO);
        shader.unuse();
    }
}

void Displayer::drawSkybox()
{
    shaders[skyboxShader].use();
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)width_ / (float)height_, near_plane, far_plane);

    shaders[skyboxShader].setMat4("model", model);
    shaders[skyboxShader].setMat4("view", view);
    shaders[skyboxShader].setMat4("projection", projection);
    //rotate(time, vec3(0., 1, 0))* rotate(-0.1, vec3(1., 0, 0))

    shaders[skyboxShader].setVec3("camera_pos", camera.Position);
    // skybox cube
    glDepthFunc(GL_LEQUAL);

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);

    // set depth function back to default
    glDepthFunc(GL_LESS);
    shaders[skyboxShader].unuse();
}


void Displayer::CallBackDisplay() {

    time_now = clock() * 1.0 / CLOCKS_PER_SEC;
    fps.add(time_now);
    cout << "Fps: " << fps.get() << "\r";
    cout.flush();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)width_ / (float)height_, near_plane, far_plane);

    float htan = tan(0.5 * glm::radians(camera.Zoom));

    // draw scene as normal
    if (flag_selected) 
    {
        //glm::vec2 posc(mouse_down_x_, mouse_down_y_);
        //selected.y = board_height;
        //for (int i = 0; i < 4; i++)
        //{
        //    for (int j = 0; j < 4; j++)
        //    {
        //        cout << projection[i][j] << ",  ";
        //    }
        //    cout << endl;
        //}
        //selected.z = -selected.y / (posc.y * htan);
        //selected.x = ((float)width_ / (float)height_) * (selected.y / posc.y) * posc.x;
        //selected.w = 1;
        //selected = glm::inverse(view) * selected;
        //selected = selected / selected.w;
        //flag_selected = false;
        //std::cout << mouse_down_x_ << "," << mouse_down_y_ << std::endl;
        //std::cout << selected.x << "," << selected.y << "," << selected.z << std::endl;
        

        shaders[positionShader].use();
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(0, 0, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawModels(shaders[positionShader], true);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        /*glReadPixels(
            0, 0,
            width_, height_,
            GL_RGBA, GL_FLOAT,
            (void*)buff_data.data());
        vector<unsigned char> buf_img(width_ * height_ * 4);
        for (int i = 0; i < width_*height_*4; i++)
        {
            buf_img[i] = buff_data[i] * 255;
        }
        stbi_flip_vertically_on_write(true);
        stbi_write_png("D:/a.png", width_, height_, 4, buf_img.data(),0);
        
        int pos = (mouse_down_x_ + (height_ - 1 - mouse_down_y_) * width_) * 4;
        selected.x = buff_data[pos + 0];
        selected.y = buff_data[pos + 1];
        selected.z = buff_data[pos + 2];
        selected.w = buff_data[pos + 3];*/
        glReadPixels(
            mouse_down_x_, height_ - 1 - mouse_down_y_,
            1, 1,
            GL_RGBA, GL_FLOAT,
            (void*)buff_data.data());
        selected.x = buff_data[0];
        selected.y = buff_data[1];
        selected.z = buff_data[2];
        selected.w = buff_data[3];

        selected = selected * 2.f - glm::fvec4(1, 1, 1, 1);
        selected = glm::inverse(view) * glm::inverse(projection) * selected;
        //selected = selected / selected.w;
        selected = selected / selected.y * (board_height);
        selected.w = 1;

        shaders[positionShader].unuse();
        flag_selected = false;
        std::cout << mouse_down_x_ << "," << mouse_down_y_ << std::endl;
        std::cout << selected.x << "," << selected.y << "," << selected.z << std::endl;

    }

    drawModels(shaders[modelShader]);

    shaders[oceanTexShader].use();
    glDisable(GL_DEPTH_TEST);
    shaders[oceanTexShader].setFloat("near", near_plane);
    shaders[oceanTexShader].setVec3("camera_pos", camera.Position);
    shaders[oceanTexShader].setVec3("camera_x", camera.Right);
    shaders[oceanTexShader].setVec3("camera_y", camera.Up);
    shaders[oceanTexShader].setVec3("camera_z", -camera.Front);
    shaders[oceanTexShader].setVec2("hsize", glm::vec2(htan, htan));

    shaders[oceanTexShader].setVec3("iResolution", glm::vec3(tex_width, tex_height, 1));
    shaders[oceanTexShader].setFloat("iTime", time_now);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    shaders[oceanTexShader].unuse();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    shaders[oceanShader].use();

    shaders[oceanShader].setMat4("model", model);
    shaders[oceanShader].setMat4("view", view);
    shaders[oceanShader].setMat4("projection", projection);
    shaders[oceanShader].setTex("screenTexture", shaders[oceanTexShader].TBO,0);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    shaders[oceanShader].unuse();



    //drawSkybox();
    
    glutSwapBuffers();
    glutPostRedisplay();
}

void Displayer::CallBackMotion(int xpos, int ypos) {

    if (mouse_down_button_ == GLUT_LEFT_BUTTON)
    {
        int dx =  xpos - mouse_down_x_;
        int dy =  ypos - mouse_down_y_;
        mouse_down_x_ = xpos;
        mouse_down_y_ = ypos;

        camera.ProcessMouseMovement(-dx, dy);
    }
    glutPostRedisplay();
}

void Displayer::initShader()
{
    string shader_dir = conf["shader_dir"].as<std::string>();

    string position_v = shader_dir + conf["shader"]["position_v"].as<std::string>();
    string position_f = shader_dir + conf["shader"]["position_f"].as<std::string>();
    positionShader=addShader(position_v, position_f, false);

    string model_v = shader_dir + conf["shader"]["model_v"].as<std::string>();
    string model_f = shader_dir + conf["shader"]["model_f"].as<std::string>();
    modelShader=addShader(model_v, model_f, false);

    string skybox_v = shader_dir + conf["shader"]["skybox_v"].as<std::string>();
    string skybox_f = shader_dir + conf["shader"]["skybox_f"].as<std::string>();
    skyboxShader=addShader(skybox_v, skybox_f, false);


    string oceanTexShader_v = shader_dir + "w_v.glsl";
    string oceanTexShader_f = shader_dir + "w_f.glsl";
    oceanTexShader = addShader(oceanTexShader_v, oceanTexShader_f,  true, tex_width, tex_height);



    string oceanShader_v = shader_dir + "oceanAndCloud_v.glsl";
    string oceanShader_f = shader_dir + "oceanAndCloud_f.glsl";
    oceanShader = addShader(oceanShader_v, oceanShader_f, false);
}

void Displayer::initModels()
{
    string resource_dir = conf["resource_dir"].as<std::string>();
    string boatPath = resource_dir+conf["resource"]["boat"].as<std::string>();
    string carPath = resource_dir+conf["resource"]["car"].as<std::string>();
    string jy01Path = resource_dir+"/jy01/jy01.obj";
    string jq01Path = resource_dir+"/jq01/jq01.obj";
    string jd01Path = resource_dir+"/jd01/jd01.obj";
    string plb01Path = resource_dir+"/plb01/plb01.obj";
    stbi_set_flip_vertically_on_load(true);
    boat.loadModel(boatPath);
    stbi_set_flip_vertically_on_load(false);
    car.loadModel(carPath);
    jy01.loadModel(jy01Path);
    jd01.loadModel(jd01Path);
    jq01.loadModel(jq01Path);
    plb01.loadModel(plb01Path);
    


}


void Displayer::initSkybox()
{
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
    // skybox VAO
    unsigned int skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    YAML::Node conf = YAML::LoadFile(YAML_CONFIG);

    string resource_dir = conf["resource_dir"].as<std::string>();
    std::vector<std::string> faces
    {
        resource_dir + conf["resource"]["skybox"]["right"].as<std::string>(),
        resource_dir + conf["resource"]["skybox"]["left"].as<std::string>(),
        resource_dir + conf["resource"]["skybox"]["top"].as<std::string>(),
        resource_dir + conf["resource"]["skybox"]["bottom"].as<std::string>(),
        resource_dir + conf["resource"]["skybox"]["front"].as<std::string>(),
        resource_dir + conf["resource"]["skybox"]["back"].as<std::string>()
    };
    skyboxTex = loadCubemap(faces);
}

int Displayer::addShader(string v_shader, string f_shader, bool create_fbo, int width, int height)
{
    shaders.push_back(Shader());
    shaders.back().compile(v_shader.c_str(), f_shader.c_str());
    if (create_fbo)
    {
        shaders.back().initFBO(width, height, true);
    }
    return shaders.size() - 1;
}

unsigned int Displayer::loadTexture2D(std::string path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int Displayer::loadTexture3D(std::string path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;

    ifstream bin;
    bin.open(path, std::ifstream::binary | std::ifstream::in);
    bin.seekg(0, bin.end);
    int length = bin.tellg();
    bin.seekg(0, bin.beg);

    unsigned char* data = new unsigned char[length];

    bin.read((char*)data, length);
    bin.close();


    glBindTexture(GL_TEXTURE_3D, textureID);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, 32, 32, 32, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_3D);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    return textureID;
}

unsigned int Displayer::loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}