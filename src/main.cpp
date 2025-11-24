#include "Common.h"

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

using namespace glm;
using namespace std;

mat4 view;
mat4 projection;
mat4 model;

GLint um4p;
GLint um4mv;
GLint ubflag;
bool flag = false;

GLuint program;

// framebuffer post processing
bool usePostProcessing = false;
// FBO parameter
GLuint FBO;
GLuint depthRBO;
GLuint FBODataTexture;
GLuint program2;
GLuint window_vao;
GLuint window_buffer;
GLuint texLoc;
GLuint fboTexLoc;

static const GLfloat window_positions[] =
{
	1.0f,-1.0f,1.0f,0.0f,
	-1.0f,-1.0f,0.0f,0.0f,
	-1.0f,1.0f,0.0f,1.0f,
	1.0f,1.0f,1.0f,1.0f
};

GLuint TexLoc;

typedef struct
{
	GLuint vao;			// vertex array object
	GLuint vbo;			// vertex buffer object

	int materialId;
	int vertexCount;
	GLuint m_texture;
} Shape;

Shape m_shape;

char** loadShaderSource(const char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* src = new char[sz + 1];
	fread(src, sizeof(char), sz, fp);
	src[sz] = '\0';
	char** srcp = new char* [1];
	srcp[0] = src;
	return srcp;
}

void freeShaderSource(char** srcp)
{
	delete srcp[0];
	delete srcp;
}

void My_LoadModels()
{
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string warn;
	string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "assets/ladybug.obj");
	if (!warn.empty()) {
		cout << warn << endl;
	}
	if (!err.empty()) {
		cout << err << endl;
	}
	if (!ret) {
		exit(1);
	}

	vector<float> vertices, texcoords, normals;  // if OBJ preserves vertex order, you can use element array buffer for memory efficiency
	for (int s = 0; s < shapes.size(); ++s) {  // for 'ladybug.obj', there is only one object
		int index_offset = 0;
		for (int f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			for (int v = 0; v < fv; ++v) {
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
				vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
				texcoords.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
				texcoords.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib.normals[3 * idx.normal_index + 2]);
			}
			index_offset += fv;
			m_shape.vertexCount += fv;
		}
	}

	glGenVertexArrays(1, &m_shape.vao);
	glBindVertexArray(m_shape.vao);

	glGenBuffers(1, &m_shape.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_shape.vbo);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + texcoords.size() * sizeof(float) + normals.size() * sizeof(float), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), texcoords.size() * sizeof(float), texcoords.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + texcoords.size() * sizeof(float), normals.size() * sizeof(float), normals.data());

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float) + texcoords.size() * sizeof(float)));
	glEnableVertexAttribArray(2);

	shapes.clear();
	shapes.shrink_to_fit();
	materials.clear();
	materials.shrink_to_fit();
	vertices.clear();
	vertices.shrink_to_fit();
	texcoords.clear();
	texcoords.shrink_to_fit();
	normals.clear();
	normals.shrink_to_fit();

	cout << "Load " << m_shape.vertexCount << " vertices" << endl;

	TextureData tdata = loadImg("assets/ladybug_diff.png");

	glGenTextures(1, &m_shape.m_texture);
	glBindTexture(GL_TEXTURE_2D, m_shape.m_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	delete[] tdata.data;
}

void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// draw model processing program
	program = glCreateProgram();
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** vertexShaderSource = loadShaderSource("shaders/vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("shaders/fragment.fs.glsl");
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);
	printGLShaderLog(vertexShader);
	printGLShaderLog(fragmentShader);
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	um4p = glGetUniformLocation(program, "um4p");
	um4mv = glGetUniformLocation(program, "um4mv");
	ubflag = glGetUniformLocation(program, "ubflag");
	TexLoc = glGetUniformLocation(program, "tex");

	glUseProgram(program);
	// Link texture
	glUniform1i(TexLoc, 0);

	// Load model
	My_LoadModels();

	// post processing program
	program2 = glCreateProgram();
	GLuint vs2 = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs2 = glCreateShader(GL_FRAGMENT_SHADER);
	char** FB_vertexShaderSource = loadShaderSource("shaders/FB_vertex.vs.glsl");
	char** FB_fragmentShaderSource = loadShaderSource("shaders/FB_fragment.fs.glsl");

	glShaderSource(vs2, 1, FB_vertexShaderSource, NULL);
	glShaderSource(fs2, 1, FB_fragmentShaderSource, NULL);
	freeShaderSource(FB_vertexShaderSource);
	freeShaderSource(FB_fragmentShaderSource);
	glCompileShader(fs2);
	glCompileShader(vs2);
	printGLShaderLog(vs2);
	printGLShaderLog(fs2);

	glAttachShader(program2, vs2);
	glAttachShader(program2, fs2);
	glLinkProgram(program2);

	glGenVertexArrays(1, &window_vao);
	glBindVertexArray(window_vao);

	glGenBuffers(1, &window_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, window_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(window_positions), window_positions, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, (const GLvoid*)(sizeof(GL_FLOAT) * 2));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	texLoc = glGetUniformLocation(program2, "tex");

	glGenFramebuffers(1, &FBO);
}

// GLUT callback. Called to draw the scene.
void My_Display(GLFWwindow* window)
{
	// #TODO 5:
	if (usePostProcessing)
	{
		// (1) Bind the framebuffer object correctly
		// BEGIN ANSWER

		// END ANSWER

		// (2) Set draw buffer
		// BEGIN ANSWER

		// END ANSWER
	}
	else
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

	glUseProgram(program);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Clear color & depth buffer before we draw
	static const GLfloat clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glClearBufferfv(GL_COLOR, 0, clearColor);
	glClear(GL_DEPTH_BUFFER_BIT);

	// Draw the ladybug
	glBindVertexArray(m_shape.vao);
	GLfloat move = glfwGetTime() * 20.0f;
	model = rotate(mat4(1.0f), radians(move), vec3(0.0, 1.0, 0.0));

	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	// #TODO 1: Pass flag to shader (global variable: ubflag) via uniform
	// BEGIN ANSWER

	// END ANSWER

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_shape.m_texture);

	glDrawArrays(GL_TRIANGLES, 0, m_shape.vertexCount);

	if (usePostProcessing)
	{
		// Re-bind the framebuffer and clear it 
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		// #TODO 6:
		// (1) Clear depth buffer
		// (2) Bind the post processing shader program
		// (3) Bind the correct vao for post processing
		// (4) Bind input texture for post processing shader program

		// BEGIN ANSWER

		// END ANSWER

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
	glfwSwapBuffers(window);
}

void My_Reshape(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);

	float viewportAspect = (float)width / (float)height;

	projection = perspective(radians(60.0f), viewportAspect, 0.1f, 1000.0f);
	view = lookAt(vec3(-10.0f, 5.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

	// If the windows is reshaped, we need to reset some settings of framebuffer
	glDeleteRenderbuffers(1, &depthRBO);
	glDeleteTextures(1, &FBODataTexture);
	glGenRenderbuffers(1, &depthRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);


	// #TODO 7:
	// (1) Generate a color texture for FBO
	// (2) Allocate a storage for the texture object via glTexImage2D.
	// (2) Set filtering parameters for the color texture.
	// BEGIN ANSWER

	// END ANSWER

	// #TODO 8:
	// (1) Bind the framebuffer object to GL_DRAW_FRAMEBUFFER
	// (2) Attach depth renderbuffer object to the bound framebuffer object
	// (3) Attach the color texture to the bound framebuffer object
	// BEGIN ANSWER

	// END ANSWER

}

void My_Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	double x, y;
    glfwGetCursorPos(window, &x, &y);
	printf("Key %c is pressed at (%d, %d)\n", key, x, y);
    if (action == GLFW_PRESS) {
		switch (key)
		{
		case GLFW_KEY_A:
			// #TODO 2: Change flag for color (global variable: flag)
			// BEGIN ANSWER
			flag = !flag;
			// END ANSWER
			break;
		case GLFW_KEY_Z:
			usePostProcessing = !usePostProcessing;
		}
	}
}

void My_MouseButton(GLFWwindow* window, int button, int action, int mods)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (action == GLFW_PRESS) {
        switch (button)
        {
        case GLFW_MOUSE_BUTTON_LEFT:
            printf("Left mouse button pressed at (%.0f, %.0f)\n", x, y);
            break;
        }
    }
    else if (action == GLFW_RELEASE) {
        switch (button)
        {
        case GLFW_MOUSE_BUTTON_LEFT:
            printf("Left mouse button released at (%.0f, %.0f)\n", x, y);
            break;
        }
    }
}


int main(int argc, char *argv[])
{
    // Initialize GLFW
	if (!glfwInit()) {
		return -1;
	}
            
	// Set OpenGL version and core profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// Create window
	GLFWwindow* window = glfwCreateWindow(600, 600, "Quiz_04", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}
            
	// Position window
	glfwSetWindowPos(window, 100, 100);
            
	// Make context current
	glfwMakeContextCurrent(window);
            
	// Load OpenGL functions
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		return -1;
	}
        
    printGLContextInfo();
    My_Init();
        
    // Set callbacks
	glfwSetFramebufferSizeCallback(window, My_Reshape);
	glfwSetKeyCallback(window, My_Keyboard);
	glfwSetMouseButtonCallback(window, My_MouseButton);
         
	// Set initial viewport
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
    
	My_Reshape(window, width, height);
         
	// Main loop
	while (!glfwWindowShouldClose(window)) {
		My_Display(window);
		glfwPollEvents();
	}
         
	// Cleanup
	glfwTerminate();
	return 0;

}

