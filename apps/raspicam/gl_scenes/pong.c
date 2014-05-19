/*
Copyright (c) 2013, Broadcom Europe Ltd
Copyright (c) 2013, Tim Gover
All rights reserved.


Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "pong.h"
#include "RaspiTex.h"
#include "RaspiTexUtil.h"
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "RaspiImv.h"
#include "GraphicsStub.h"


/**
 * Draws an external EGL image and applies a sine wave distortion to create
 * a hall of pongs effect.
 */
static RASPITEXUTIL_SHADER_PROGRAM_T pong_shader_test = {
    .vertex_source =
    "attribute vec2 vertex;\n"
    "varying vec2 texcoord;"
    "void main(void) {\n"
    "   texcoord = 0.5 * (1.0 - vertex );\n"
    "   gl_Position = vec4(vertex, 0.0, 1.0);\n"
    "}\n",

    .fragment_source =
    "#extension GL_OES_EGL_image_external : require\n"
    "uniform samplerExternalOES tex;\n"
    "uniform float offset;\n"
//    "uniform vec2 border;\n"
		"uniform sampler2D foo;\n"
    "const float waves = 2.0;\n"
    "varying vec2 texcoord;\n"
    "void main(void) {\n"
    "    float x = texcoord.x + 0.00 * sin(offset + (texcoord.y * waves * 2.0 * 3.141592));\n"
    "    float y = texcoord.y + 0.00 * sin(offset + (texcoord.x * waves * 2.0 * 3.141592));\n"
    "    if (y < 1.0 && y > 0.0 && x < 1.0 && x > 0.0) {\n"
//		"     if( x < border.x || x > border.y ){\n"
		"     if( x > 1.0 ){\n"
		"       gl_FragColor = 0.7*texture2D(tex, texcoord)+0.3*texture2D(foo, texcoord);\n"
		"     }else{\n"
		"       gl_FragColor = texture2D(tex, texcoord);\n"
    "     }\n"
    "    }\n"
    "    else {\n"
		"       gl_FragColor = texture2D(foo, texcoord);\n"
    "       gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "    }\n"
    "}\n",
    .uniform_names = {"tex", "offset", /*"border",*/ "foo"},
    .attribute_names = {"vertex"},
};

static RASPITEXUTIL_SHADER_PROGRAM_T pong_shader = {
    .vertex_source =
    "attribute vec2 vertex;\n"
    "varying vec2 texcoord;"
    "void main(void) {\n"
    "   texcoord = 0.5 * (1.0 - vertex );\n"
    "   gl_Position = vec4(vertex, 0.0, 1.0);\n"
    "}\n",

    .fragment_source =
    "#extension GL_OES_EGL_image_external : require\n"
    "uniform samplerExternalOES tex;\n"
    "uniform float offset;\n"
    "const float waves = 2.0;\n"
    "varying vec2 texcoord;\n"
    "void main(void) {\n"
    "    float x = texcoord.x + 0.00 * sin(offset + (texcoord.y * waves * 2.0 * 3.141592));\n"
    "    float y = texcoord.y + 0.00 * sin(offset + (texcoord.x * waves * 2.0 * 3.141592));\n"
    "    if (y < 1.0 && y > 0.0 && x < 1.0 && x > 0.0) {\n"
		"	      gl_FragColor = texture2D(tex, texcoord);\n"
    "    }\n"
    "    else {\n"
    "       gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "    }\n"
    "}\n",
    .uniform_names = {"tex", "offset"},
    .attribute_names = {"vertex"},
};

static RASPITEXUTIL_TEXTURE_T texScore;

static const EGLint attribute_list[] =
{
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_NONE
};

/**
 * Creates the OpenGL ES 2.X context and builds the shaders.
 * @param raspitex_state A pointer to the GL preview state.
 * @return Zero if successful.
 */
static int pong_init(RASPITEX_STATE *state)
{
    int rc = raspitexutil_gl_init_2_0(state);
    if (rc != 0)
       goto end;

    rc = raspitexutil_build_shader_program(&pong_shader);

		printf("Loading numerals\n");
		//texScore = raspitexutil_load_texture("shader/ziffern.png");

		InitTextures();
		InitShaders();
end:
    return rc;
}

#define BORDER 0.3
static int score[2] = {0,0};

static int pong_redraw(RASPITEX_STATE *raspitex_state) {
    static float offset = 0.0;

    // Start with a clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind the OES texture which is used to render the camera preview
		glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, raspitex_state->texture);

		// Bind Score texture
		glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texScore.id);
		glActiveTexture(GL_TEXTURE0);

    offset += 0.05;
    GLCHK(glUseProgram(pong_shader.program));
    GLCHK(glEnableVertexAttribArray(pong_shader.attribute_locations[0]));

    GLfloat varray[] = {
        -1.0f, -1.0f,
        1.0f,  1.0f,
        1.0f, -1.0f,

        -1.0f,  1.0f,
        1.0f,  1.0f,
        -1.0f, -1.0f,
    };
    GLCHK(glVertexAttribPointer(pong_shader.attribute_locations[0], 2, GL_FLOAT, GL_FALSE, 0, varray));
    GLCHK(glUniform1f(pong_shader.uniform_locations[1], offset));
    //GLCHK(glUniform2f(pong_shader.uniform_locations[2], BORDER, 1.0-BORDER));
    //GLCHK(glUniform2i(pong_shader.uniform_locations[2], score[0], score[1]));
    GLCHK(glDrawArrays(GL_TRIANGLES, 0, 6));

    GLCHK(glDisableVertexAttribArray(pong_shader.attribute_locations[0]));
    GLCHK(glUseProgram(0));

		GLCHK(glEnable(GL_BLEND));
		GLCHK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		RedrawTextures();
		GLCHK(glDisable(GL_BLEND));

    return 0;
}

int pong_open(RASPITEX_STATE *state)
{
	//use own attribs list
	//state->egl_config_attribs = attribute_list ;

	state->ops.gl_init = pong_init;
	state->ops.redraw = pong_redraw;
	state->ops.update_texture = raspitexutil_update_texture;
	return 0;
}
