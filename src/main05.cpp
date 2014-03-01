#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

#include "GL/glew.h"

#include <math.h>
#define _USE_MATH_DEFINES

#ifdef __linux__
#include <GL/glut.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <glut.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include "GL/glut.h"
#endif

#include "inc/objLoader.h"

const char *PROGRAM_NAME = "main05";

///////////////////////////////////////////////////////////////////////
//Error checking functions
void CheckOpenGLError(const char* stmt, const char* fname, int line)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, stmt);
		abort();
	}
}

#define _DEBUG 1
#if _DEBUG
#define GL_CHECK(stmt) do { \
	stmt; \
	CheckOpenGLError(#stmt, __FILE__, __LINE__); \
} while (0)
#else
#define GL_CHECK(stmt) stmt
#endif

#if _DEBUG
#define GET_GLERROR(ret)                                          \
{                                                                 \
	GLenum err = glGetError();                                    \
	if (err != GL_NO_ERROR) {                                     \
	fprintf(stderr, "[%s line %d] GL Error: %s\n",                \
	__FILE__, __LINE__, gluErrorString(err));                     \
	fflush(stderr);                                               \
	}                                                             \
}
#else
#define GET_GLERROR(ret) ;
#endif

GLuint rt_shader;
GLuint texture_shader;

GLfloat model_view_matrix[16] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
};

GLfloat projection_matrix[16] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
};

void load_uniform_mat4(GLfloat matrix[16])
{
	matrix[0] = 1;
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;
	matrix[4] = 0;
	matrix[5] = 1;
	matrix[6] = 0;
	matrix[7] = 0;
	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = 1;
	matrix[11] = 0;
	matrix[12] = 0;
	matrix[13] = 0;
	matrix[14] = 0;
	matrix[15] = 1;
}

/* A simple function that prints a message, the error code returned by SDL,
 * and quits the application */
void sdldie(const char *msg)
{
	printf("%s: %s\n", msg, SDL_GetError());
	SDL_Quit();
	exit(1);
}

void CheckFramebufferStatus()
{
	GLenum status;
	status = (GLenum) glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
	switch(status) {
	case GL_FRAMEBUFFER_COMPLETE:
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED:
		printf("Unsupported framebuffer format\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		printf("Framebuffer incomplete, missing attachment\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
		printf("Framebuffer incomplete, attached images must have same dimensions\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
		printf("Framebuffer incomplete, attached images must have same format\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		printf("Framebuffer incomplete, missing draw buffer\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		printf("Framebuffer incomplete, missing read buffer\n");
		break;
	default:
		break;
	}
}

///////////////////////////////////////////////////////////////////////
//Text file IO for shader files

char *textFileRead(char *fn)
{
	FILE *fp;
	char *content = NULL;

	int count=0;

	if (fn != NULL) {
		fp = fopen(fn,"rt");

		if (fp != NULL) {

			fseek(fp, 0, SEEK_END);
			count = ftell(fp);
			rewind(fp);

			if (count > 0) {
				content = (char *)malloc(sizeof(char) * (count+1));
				count = fread(content,sizeof(char),count,fp);
				content[count] = '\0';
			}
			fclose(fp);
		}
	}

	if (content == NULL)
	{
		fprintf(stderr, "ERROR: could not load in file %s\n", fn);
		exit(1);
	}
	return content;
}

int textFileWrite(char *fn, char *s)
{
	FILE *fp;
	int status = 0;

	if (fn != NULL) {
		fp = fopen(fn,"w");

		if (fp != NULL) {

			if (fwrite(s,sizeof(char),strlen(s),fp) == strlen(s))
				status = 1;
			fclose(fp);
		}
	}
	return(status);
}

///////////////////////////////////////////////////////////////////////


GLuint depth_rb, fb;
float lpos[4] = {5.0, 0.5, 5.0, 0.0};

GLuint tex; // texture

int texWidth;
int texHeight;

// mouse controls
int mouse_old_x, mouse_old_y;
int mouse_buttons = 0;
float rotate_x = 0.0, rotate_y = 0.0;
float move_x = 0.0, move_y = 0.0;
float translate_z = -1.0;

float win_width = 512.0, win_height = 512.0;

void init_framebuffer(int w, int h)
{
	texWidth = w;
	texHeight = h;

	GET_GLERROR(NULL);

	////////framebuffer init
	if(glIsTexture(tex))
	{
		glDeleteRenderbuffers(1, &depth_rb);
		glDeleteTextures(1,&tex);
		glDeleteFramebuffers(1, &fb);
	}

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0, 0.0, 0.0, 1.0);

	glGenFramebuffers(1, &fb);
	glGenTextures(1, &tex);
	glGenRenderbuffers(1, &depth_rb);

	glBindFramebuffer(GL_FRAMEBUFFER, fb);

	// init texture
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texWidth, texHeight, 0,
		GL_RGBA, GL_FLOAT, NULL);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, tex, 0);

	// initialize depth renderbuffer
	glBindRenderbufferEXT(GL_RENDERBUFFER, depth_rb);
	glRenderbufferStorageEXT(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, texWidth, texHeight);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER, depth_rb);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);

	GLenum drawbuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawbuffers); // "1" is the size of DrawBuffers

	CheckFramebufferStatus();

	glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
}

///////////////////////////////////////////////////////////////////////
//adapt viewport when window size changes
void changeSize(SDL_Window *main_window)
{
	// Prevent a divide by zero, when window is too small
	if(win_height == 0)
		win_height = 1;
	if(win_width == 0)
		win_width = 1;

	float aspect = 1.0 * win_width / win_height;

	// Set the viewport to be the entire window
	glViewport(0, 0, win_width, win_height);

	float near = 0.1;
	float far = 1000;

	double yScale = 1.0 / tan((M_PI / 180.0) * 45 / 2);
	double xScale = yScale / aspect;
	double nearmfar = near - far;

	/*projection_matrix[0] = xScale;
	projection_matrix[5] = yScale;
	projection_matrix[10] = (far + near) / nearmfar;
	projection_matrix[11] = -1;
	projection_matrix[14] = 2 * far * near / nearmfar;
	projection_matrix[14] = 2 * far * near / nearmfar;*/

	// projection_matrix = {
	//     xScale, 0, 0, 0,
	//     0, yScale, 0, 0,
	//     0, 0, (far + near) / nearmfar, 2 * far * near / nearmfar,
	//     0, 0, -1, 2 * far * near / nearmfar
	// };

	texWidth = win_width;
	texHeight = win_height;
	init_framebuffer(texWidth, texHeight);

	SDL_SetWindowSize(main_window, win_width, win_height);
}

///////////////////////////////////////////////////////////////////////
//the actual render function, which is called for each frame
void renderScene(SDL_Window *main_window)
{
	GET_GLERROR(0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, fb);

	glUseProgram(rt_shader);

	GLuint loc = glGetUniformLocation(rt_shader, "modelViewMatrix");
	if(loc != -1)
		glUniformMatrix4fv( loc, 1, GL_FALSE, model_view_matrix );

	loc = glGetUniformLocation(rt_shader, "projectionMatrix");
	if(loc != -1)
		glUniformMatrix4fv( loc, 1, GL_FALSE, projection_matrix );

	GET_GLERROR(0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, texWidth, texHeight);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	GET_GLERROR(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GET_GLERROR(0);

	// now render to the screen using the texture...
	glClearColor(0.0, 0.0, 1.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GET_GLERROR(0);

	// draw textured quad
	glUseProgram(texture_shader);

	GET_GLERROR(0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);

	//glViewport(0,0,win_width,win_height);

	GET_GLERROR(0);

	loc = glGetUniformLocation(texture_shader, "texMap");
	if(loc != -1)
		glUniform1i( loc, 0 );

	loc = glGetUniformLocation(texture_shader, "in_colour");
	if(loc != -1)
		glUniform4f( loc, 0.5f, 0.0f, 0.6f, 1.0f );

	GET_GLERROR(0);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDrawArrays(GL_LINE_LOOP, 0, 6);

	glUseProgram(0);

	GET_GLERROR(0);

	SDL_GL_SwapWindow( main_window );
}

///////////////////////////////////////////////////////////////////////
// mouse interaction functions
void mouseClick(int button, int state, int x, int y)
{
	if (state == 0)
	{
	 	mouse_buttons |= 1 << button;
	}
	else if (state == 1)
	{
	 	mouse_buttons &= !(1 << button);
	}

	mouse_old_x = x;
	mouse_old_y = y;
}

void mouseMotion(int x, int y)
{
	float dx, dy;
	dx = (float)(x - mouse_old_x);
	dy = (float)(y - mouse_old_y);

	if (mouse_buttons & (1 << SDL_BUTTON_LEFT))
	{
		rotate_x += dy * 0.2f;
		rotate_y += dx * 0.2f;
	}
	else if (mouse_buttons & (1 << SDL_BUTTON_MIDDLE))
	{
		move_x += dx * 1.0f/win_width;
		move_y -= dy * 1.0f/win_height;
	}
	else if (mouse_buttons & (1 << SDL_BUTTON_RIGHT))
	{
		translate_z += dy * 0.01f;
	}

	mouse_old_x = x;
	mouse_old_y = y;
}

///////////////////////////////////////////////////////////////////////
//load, compile and set the shaders
GLuint setShaders(char *shader_names[], GLenum shader_types[], uint32_t num_shaders)
{
	GLuint shader_handle;

	char *vs,*fs,*rfs;

	GLuint *shader_idents = (GLuint *)malloc(sizeof(GLuint) * num_shaders);
	char *(*shader_sources) = (char **)malloc(sizeof(const char *) * num_shaders);

	shader_handle = glCreateProgram();

	for(int shader_index = 0; shader_index < num_shaders; ++shader_index)
	{
		GET_GLERROR(0);

		shader_idents[shader_index] = glCreateShader(shader_types[shader_index]);
		shader_sources[shader_index] = textFileRead(shader_names[shader_index]);
		glShaderSource(shader_idents[shader_index], 1, (const char**)(&shader_sources[shader_index]), NULL);
		free(shader_sources[shader_index]);
		glCompileShader(shader_idents[shader_index]);

		GLint blen = 0;
		GLsizei slen = 0;

		glGetShaderiv(shader_idents[shader_index], GL_INFO_LOG_LENGTH , &blen);
		if (blen > 1)
		{
			GLchar* compiler_log = (GLchar*)malloc(blen);
			glGetShaderInfoLog(shader_idents[shader_index], blen, &slen, compiler_log);
			printf("%s compiler log:\n%s\n", shader_names[shader_index], compiler_log);
			free (compiler_log);
		}

		glAttachShader(shader_handle, shader_idents[shader_index]);

		GET_GLERROR(0);
	}

	free(shader_sources);

	glBindAttribLocation(shader_handle, 0, "in_position");
	glBindAttribLocation(shader_handle, 1, "in_texCoord");

	GET_GLERROR(0);

	glLinkProgram(shader_handle);
	glUseProgram(shader_handle);
	//set uniforms

	GET_GLERROR(0);

	glUseProgram(0);

	return shader_handle;
}

SDL_GLContext setup_GL_context(SDL_Window *mainwindow)
{
	SDL_GLContext maincontext;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	/* Turn on double buffering with a 24bit Z buffer. */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	/* Create our opengl context and attach it to our window */
	maincontext = SDL_GL_CreateContext(mainwindow);

	/* This makes our buffer swap syncronized with the monitor's vertical refresh */
	SDL_GL_SetSwapInterval(1);

	return maincontext;
}

PPMImage * textureImage;

void initialize ()
{
	//glViewport(0, 0, win_width, win_height);
	GET_GLERROR(NULL);

	GLfloat aspect = (GLfloat) 568 / 320;

	glClearColor( 0.0f, 0.0f, 0.0f, 0.5f );
	GET_GLERROR(NULL);
	glClearDepth( 1.0f );
	GET_GLERROR(NULL);
	glEnable( GL_DEPTH_TEST );
	GET_GLERROR(NULL);
	glDepthFunc( GL_LEQUAL );
	GET_GLERROR(NULL);

	glDepthFunc( GL_LEQUAL );
	GET_GLERROR(NULL);
	glEnable( GL_DEPTH_TEST );

	// glActiveTexture(GL_TEXTURE0);
	// textureImage = readPPM("pebbles_texture.ppm");
	// //textureImage = readPPM("gradient.ppm");
	// glGenTextures(1, &tex);
	// glBindTexture(GL_TEXTURE_2D, tex);
	// glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// printf("%d %d \n", textureImage->x, textureImage->y);
	// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureImage->x, textureImage->y, 0, GL_RGB, GL_UNSIGNED_BYTE, textureImage->data);

	// glBindTexture(GL_TEXTURE_2D, 0);

	init_framebuffer(win_width, win_height);

	GET_GLERROR(0);

	// The fullscreen quad's FBO
	GLuint quad_VertexArrayID;
	glGenVertexArrays(1, &quad_VertexArrayID);
	glBindVertexArray(quad_VertexArrayID);

	static const GLfloat g_quad_vertex_buffer_data[] = {
		-1.0f, -1.0f, -1.0f,		0.0f, 0.0f,
		1.0f, -1.0f, -1.0f,			1.0f, 0.0f,
		-1.0f,  1.0f, -1.0f,		0.0f, 1.0f,
		-1.0f,  1.0f, -1.0f,		0.0f, 1.0f,
		1.0f, -1.0f, -1.0f,			1.0f, 0.0f,
		1.0f,  1.0f, -1.0f,			1.0f, 1.0f,
	};

	GLuint quad_vertexbuffer;
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (void*)(sizeof(GLfloat) * 3));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

SDL_Window *setup_SDL(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) /* Initialize SDL's Video subsystem */
		sdldie("Unable to initialize SDL"); /* Or die on error */

	SDL_Window *mainwindow = SDL_CreateWindow(PROGRAM_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		win_width, win_height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	if (!mainwindow) /* Die if creation failed */
		sdldie("Unable to create window");

	return mainwindow;
}

///////////////////////////////////////////////////////////////////////
//main, setup and execution of environment
int main(int argc, char **argv) {
	/*glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(512,320);
	glutCreateWindow("Computer Graphics");

	glutDisplayFunc(renderScene);
	glutIdleFunc(renderScene);
	glutReshapeFunc(changeSize);
	glutKeyboardFunc(processNormalKeys);

	glutMouseFunc(mouseClick);
	glutMotionFunc(mouseMotion);*/

	SDL_Window *main_window = setup_SDL();

	Uint32 windowID = SDL_GetWindowID(main_window);

	SDL_GLContext main_context = setup_GL_context(main_window);

	const unsigned char *gl_version = glGetString(GL_VERSION);
	const unsigned char *glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);

	printf("OpenGL version: %s\n", gl_version);
	printf("GLSL version: %s\n", glsl_version);

	glEnable( GL_DEPTH_TEST );
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

	glewExperimental = GL_TRUE;

	glewInit();

	GET_GLERROR(NULL);

	if (GLEW_ARB_vertex_shader && GL_ARB_fragment_shader)
		printf("Ready for GLSL\n");
	else {
		printf("No GLSL support\n");
		exit(1);
	}

	if (glewIsSupported("GL_VERSION_3_3"))
		printf("Ready for OpenGL 3.3\n");
	else {
		printf("OpenGL 3.3 not supported\n");
		exit(1);
	}

	if (GLEW_ARB_vertex_shader && GL_ARB_fragment_shader && GL_EXT_geometry_shader4)
		printf("Ready for GLSL - vertex, fragment, and geometry units\n");
	else {
		printf("Not totally ready :( \n");
		exit(1);
	}

	if (GL_ARB_fragment_program && GL_ARB_vertex_program && GL_EXT_framebuffer_object)
		printf("Ready for FrameBuffer\n");
	else {
		printf("Not totally ready :( \n");
		exit(1);
	}

	GET_GLERROR(NULL);

	initialize();

	GET_GLERROR(NULL);

	const uint32_t num_tex_shaders = 2;
	char *texture_shader_names[num_tex_shaders] = {"./textureshader.vert", "./textureshader.frag"};
	GLenum texture_shader_types[num_tex_shaders] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
	texture_shader = setShaders(texture_shader_names, texture_shader_types, num_tex_shaders);

	const uint32_t num_rt_shaders = 2;
	char *rt_shader_names[num_rt_shaders] = {"./shader05.vert", "./shader05.frag"};
	GLenum rt_shader_types[num_rt_shaders] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
	rt_shader = setShaders(rt_shader_names, rt_shader_types, num_rt_shaders);

	GET_GLERROR(0);

	bool done = false;
	SDL_Event event;

	while(!done)
	{
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_QUIT:
					done = true;
					break;

				case SDL_WINDOWEVENT:
					if (event.window.windowID == windowID)
					{
						switch (event.window.event)
						{
							case SDL_WINDOWEVENT_RESIZED:
								win_width = event.window.data1,
								win_height = event.window.data2;
								changeSize(main_window);
						}
					}
					break;

				case SDL_KEYDOWN:
					if(event.key.keysym.sym == SDLK_ESCAPE)
						done = true;
						break;

				case SDL_MOUSEBUTTONDOWN:
					mouseClick(event.button.button, 0, event.button.x, event.button.y);
					break;

				case SDL_MOUSEBUTTONUP:
					mouseClick(event.button.button, 1, event.button.x, event.button.y);
					break;

				case SDL_MOUSEMOTION:
					mouseMotion(event.motion.x, event.motion.y);
					break;
			}
		}

		renderScene(main_window);
	}

	SDL_GL_DeleteContext(main_context);
	SDL_DestroyWindow(main_window);
	main_window = NULL;
	SDL_Quit();
	return EXIT_SUCCESS;
}