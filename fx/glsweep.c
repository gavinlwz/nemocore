#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <glsweep.h>
#include <glshader.h>
#include <fbohelper.h>
#include <oshelper.h>
#include <nemomisc.h>

static const char GLSWEEP_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLSWEEP_HORIZONTAL_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D texture;\n"
"uniform sampler2D snapshot;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float t;\n"
"void main()\n"
"{\n"
"  float x = vtexcoord.x;\n"
"  float y = vtexcoord.y;\n"
"  if (t < x)\n"
"    gl_FragColor = texture2D(snapshot, vtexcoord);\n"
"  else\n"
"    gl_FragColor = texture2D(texture, vtexcoord);\n"
"}\n";

static const char GLSWEEP_VERTICAL_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D texture;\n"
"uniform sampler2D snapshot;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float t;\n"
"void main()\n"
"{\n"
"  float x = vtexcoord.x;\n"
"  float y = vtexcoord.y;\n"
"  if (t < y)\n"
"    gl_FragColor = texture2D(snapshot, vtexcoord);\n"
"  else\n"
"    gl_FragColor = texture2D(texture, vtexcoord);\n"
"}\n";

static GLuint nemofx_glsweep_create_program(const char *shader)
{
	const char *vertexshader = GLSWEEP_VERTEX_SHADER;
	GLuint frag, vert;
	GLuint program;
	GLint status;

	frag = glshader_compile(GL_FRAGMENT_SHADER, 1, &shader);
	vert = glshader_compile(GL_VERTEX_SHADER, 1, &vertexshader);

	program = glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		GLsizei len;
		char log[1000];

		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%*s\n", len, log);

		return 0;
	}

	glUseProgram(program);
	glBindAttribLocation(program, 0, "position");
	glBindAttribLocation(program, 1, "texcoord");

	return program;
}

struct glsweep *nemofx_glsweep_create(int32_t width, int32_t height)
{
	struct glsweep *sweep;
	int size;

	sweep = (struct glsweep *)malloc(sizeof(struct glsweep));
	if (sweep == NULL)
		return NULL;
	memset(sweep, 0, sizeof(struct glsweep));

	glGenTextures(1, &sweep->texture);

	glBindTexture(GL_TEXTURE_2D, sweep->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	fbo_prepare_context(sweep->texture, width, height, &sweep->fbo, &sweep->dbo);

	sweep->width = width;
	sweep->height = height;

	return sweep;

err1:
	glDeleteTextures(1, &sweep->texture);

	free(sweep);

	return NULL;
}

void nemofx_glsweep_destroy(struct glsweep *sweep)
{
	glDeleteTextures(1, &sweep->texture);

	if (sweep->is_reference == 0 && sweep->snapshot > 0)
		glDeleteTextures(1, &sweep->snapshot);

	glDeleteFramebuffers(1, &sweep->fbo);
	glDeleteRenderbuffers(1, &sweep->dbo);

	if (sweep->program > 0)
		glDeleteProgram(sweep->program);

	free(sweep);
}

void nemofx_glsweep_ref_snapshot(struct glsweep *sweep, GLuint texture, int32_t width, int32_t height)
{
	sweep->snapshot = texture;
	sweep->is_reference = 1;
}

void nemofx_glsweep_set_snapshot(struct glsweep *sweep, GLuint texture, int32_t width, int32_t height)
{
	GLuint fbo, dbo;

	if (sweep->is_reference == 0 && sweep->snapshot > 0)
		glDeleteTextures(1, &sweep->snapshot);

	sweep->is_reference = 0;

	glGenTextures(1, &sweep->snapshot);

	glBindTexture(GL_TEXTURE_2D, sweep->snapshot);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, sweep->width, sweep->height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	fbo_prepare_context(texture, width, height, &fbo, &dbo);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glBindTexture(GL_TEXTURE_2D, sweep->snapshot);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, 0, 0, width, height, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &dbo);
}

void nemofx_glsweep_put_snapshot(struct glsweep *sweep)
{
	if (sweep->is_reference == 0 && sweep->snapshot > 0)
		glDeleteTextures(1, &sweep->snapshot);

	sweep->snapshot = 0;
	sweep->is_reference = 0;
}

void nemofx_glsweep_set_timing(struct glsweep *sweep, float t)
{
	sweep->t = t;
}

void nemofx_glsweep_set_type(struct glsweep *sweep, int type)
{
	const char *programs[] = {
		GLSWEEP_HORIZONTAL_FRAGMENT_SHADER,
		GLSWEEP_VERTICAL_FRAGMENT_SHADER
	};

	if (sweep->program > 0)
		glDeleteProgram(sweep->program);

	sweep->program = nemofx_glsweep_create_program(programs[type]);
	if (sweep->program == 0)
		return;

	sweep->utexture = glGetUniformLocation(sweep->program, "texture");
	sweep->uwidth = glGetUniformLocation(sweep->program, "width");
	sweep->uheight = glGetUniformLocation(sweep->program, "height");
	sweep->usnapshot = glGetUniformLocation(sweep->program, "snapshot");
	sweep->utiming = glGetUniformLocation(sweep->program, "t");
}

void nemofx_glsweep_resize(struct glsweep *sweep, int32_t width, int32_t height)
{
	if (sweep->width != width || sweep->height != height) {
		glBindTexture(GL_TEXTURE_2D, sweep->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteFramebuffers(1, &sweep->fbo);
		glDeleteRenderbuffers(1, &sweep->dbo);

		fbo_prepare_context(sweep->texture, width, height, &sweep->fbo, &sweep->dbo);

		sweep->width = width;
		sweep->height = height;
	}
}

void nemofx_glsweep_dispatch(struct glsweep *sweep, GLuint texture)
{
	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	if (sweep->snapshot == 0 || sweep->program == 0)
		return;

	glBindFramebuffer(GL_FRAMEBUFFER, sweep->fbo);

	glViewport(0, 0, sweep->width, sweep->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(sweep->program);
	glUniform1i(sweep->utexture, 0);
	glUniform1i(sweep->usnapshot, 1);
	glUniform1f(sweep->uwidth, sweep->width);
	glUniform1f(sweep->uheight, sweep->height);
	glUniform1f(sweep->utiming, sweep->t);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, sweep->snapshot);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
