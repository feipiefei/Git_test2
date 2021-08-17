#pragma once
#include <map>
#include <vector>
#include <string>
#include <yaml-cpp/yaml.h>
#include "camera.h"
#include "shader.h"
#include "model.h"

class Displayer {
public:
	~Displayer() = default;

	static Displayer& getInstance() { return instance_; }

	void init(int& argc, char** argv, const char* title, int width, int height);

	void runLoop();

	static void bindKey(unsigned char key, void function(int, int));

	static Camera camera;

private:
	Displayer() = default;

	static void initCallBackFunctions();

	static void initParameters();

	static void CallBackReshape(int width, int height);

	static void CallBackMouse(int status, int button, int x, int y);

	static void CallBackKeyboard(unsigned char key, int x, int y);

	static void CallBackDisplay();

	static void CallBackMotion(int x, int y);

	static void initSkybox();

	static void initModels();

	static void initShader();

	static int addShader(std::string v_shader, std::string f_shader, bool create_fbo, int width = 0, int height = 0);

	static unsigned int loadTexture3D(std::string path);

	static unsigned int loadTexture2D(std::string path);

	static unsigned int loadCubemap(std::vector<std::string> faces);
	static void drawModels(Shader& shader,bool draw_position=false);
	static void drawSkybox();

	static Displayer instance_;
	static int width_, height_;
	static double scale_times;
	static std::map<unsigned char, void (*)(int, int)> keyboard_functions_;
	static int mouse_down_button_, mouse_down_x_, mouse_down_y_;
	static double mouse_down_a_, mouse_down_b_;
	static double rotate_a_, rotate_b_;
	static double translate_a_, translate_b_;
	static double modelSize;

	static unsigned skyboxVAO;
	static unsigned skyboxTex;

	static int positionShader;
	static int skyboxShader;
	static int modelShader;
	static int oceanShader;
	static int oceanTexShader;

	static Model boat;
	static Model car;

	static std::vector<Shader> shaders;
	static std::vector<GLuint> texs;

	static Model jy01;
	static Model jq01;
	static Model jd01;
	static Model plb01;
	static YAML::Node conf;
	static GLuint vao;
};

class Fps
{
	std::vector<float> times;
	int pos;
	int span;
public:
	Fps(int span = 5) :span(span), pos(0) {
		times.reserve(span);
	}
	void add(float time_in_second)
	{
		if (times.size() < span)
			times.push_back(time_in_second);
		else
		{
			times[pos] = time_in_second;
			pos++;
			if (pos >= span)
				pos = 0;
		}
	}
	float get()
	{
		float sum = 0;
		if (times.size() < span)
			return 0;
		if (pos == 0)
			sum = times[span - 1] - times[0];
		else
			sum = times[pos - 1] - times[pos];
		return span / sum;
	}
};