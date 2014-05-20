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

#define new_shader 1
#if new_shader > 0
/**
 * Scoremapping:
 * •numerals texture contains 0,1,…,9
 * •scorePosLeft contains rect(x0,y0,x1,y1) and will be mapped on [0,1]² by transformation T1
 * •[0,1] will be mapped on part of numerals texture by transformation  T2
 * • use T2*T1 to map number onto desired position.
 * • use scorePosLeft in the fragment shader to cut of values outside of the desired window.
 */
static RASPITEXUTIL_SHADER_PROGRAM_T pong_shader = {
    .vertex_source =
    "attribute vec2 vertex;\n"
    "varying vec2 texcoord;\n"
    "varying vec2 scorecoordLeft;\n"
    "varying vec2 scorecoordRight;\n"
    "uniform vec2 score;\n"
    "uniform vec4 scorePosLeft;\n"
    "uniform vec4 scorePosRight;\n"
    "void main(void) {\n"
    "   texcoord = 0.5 * (1.0 - vertex );\n"
		"   vec2 leftWH = vec2( scorePosLeft.z-scorePosLeft.x, scorePosLeft.a - scorePosLeft.y );\n"
		"   vec2 leftPos = vec2( (texcoord.x-scorePosLeft.x)/(leftWH.x), (texcoord.y-scorePosLeft.y)/(leftWH.y) );\n"
    "   scorecoordLeft.x = score.x*0.1+leftPos.x*0.1;\n"
		"   scorecoordLeft.y = leftPos.y;\n"
		"   vec2 rightWH = vec2( scorePosRight.z-scorePosRight.x, scorePosRight.a - scorePosRight.y );\n"
		"   vec2 rightPos = vec2( (texcoord.x-scorePosRight.x)/(rightWH.x), (texcoord.y-scorePosRight.y)/(rightWH.y) );\n"
    "   scorecoordRight.x = score.y*0.1+rightPos.x*0.1;\n"
		"   scorecoordRight.y = rightPos.y;\n"
    "   gl_Position = vec4(vertex, 0.0, 1.0);\n"
    "}\n",

    .fragment_source =
    "#extension GL_OES_EGL_image_external : require\n"
    "uniform samplerExternalOES tex;\n"
//		"uniform sampler2D numerals;\n"
    "uniform float offset;\n"
    "uniform vec2 border;\n"
    "uniform vec4 scorePosLeft;\n"
    "uniform vec4 scorePosRight;\n"
    "const float waves = 2.0;\n"
    "varying vec2 texcoord;\n"
    "varying vec2 scorecoordLeft;\n"
    "varying vec2 scorecoordRight;\n"
    "void main(void) {\n"
    "    float x = texcoord.x + 0.00 * sin(offset + (texcoord.y * waves * 2.0 * 3.141592));\n"
    "    float y = texcoord.y + 0.00 * sin(offset + (texcoord.x * waves * 2.0 * 3.141592));\n"
    "    if (y < 1.0 && y > 0.0 && x < 1.0 && x > 0.0) {\n"
		"     vec4 ret = texture2D(tex, texcoord);\n"
		"     if( x < border.x || x > border.y ){\n"
		"      ret.r += 0.2;\n"
		"     }\n"
		"     if( x < scorePosLeft.z && x > scorePosLeft.x && y < scorePosLeft.w && y > scorePosLeft.y ){\n"
//		"       ret = ret + 0.3*texture2D(numerals, scorecoordLeft);\n"
		"       ret = ret + 0.3*texture2D(tex, scorecoordLeft);\n"
    "     }\n"
		"     if( x < scorePosRight.z && x > scorePosRight.x && y < scorePosRight.w && y > scorePosRight.y ){\n"
//		"       ret = ret + 0.3*texture2D(numerals, scorecoordRight);\n"
		"       ret = ret + 0.3*texture2D(tex, scorecoordLeft);\n"
    "     }\n"
		"     gl_FragColor = ret;\n"
    "    }\n"
    "    else {\n"
    "       gl_FragColor = vec4(0.0, 0.0, 0.5, 1.0);\n"
    "    }\n"
		"     gl_FragColor = 1.0-gl_FragColor;\n"
		"     gl_FragColor.a = 1.0;\n"
    "}\n",
    .uniform_names = {"tex", "offset", /*"numerals",*/ "border", "score", "scorePosLeft", "scorePosRight"},
    .attribute_names = {"vertex"},
};
#else
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
		"	      gl_FragColor.a = 1.0;\n"
    "    }\n"
    "    else {\n"
    "       gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "    }\n"
    "}\n",
    .uniform_names = {"tex", "offset"},
    .attribute_names = {"vertex"},
};
#endif

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

		//Call initialisation functions in c++ part (Graphics.cpp)
		InitTextures();
		InitShaders();

    rc = raspitexutil_build_shader_program(&pong_shader);

		printf("Loading numerals\n");
		texScore = raspitexutil_load_texture("../../images/numerals.png");
		//texScore = raspitexutil_load_texture("../../images/test.png");
		printf("Id: %i, W: %i, H:%i\n", texScore.id, texScore.width, texScore.height);
		//raspitexutil_create_framebuffer(&texScore);
		//printf("Framebuffer created.\n");
		//raspitexutil_save_texture("/dev/shm/texture.png", &texScore);
		//printf("Saved Texture\n");

end:
    return rc;
}

#define BORDER 0.2
int score[2] = {0,0};

static int pong_redraw(RASPITEX_STATE *raspitex_state) {
    static float offset = 0.0;

    // Start with a clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		GLCHK(glDisable(GL_BLEND));
#if new_shader > 0
		/* ATTENTION, bind EXTERNAL_OES images AFTER other textures!! */
//		GLCHK(glUniform1i(pong_shader.uniform_locations[2], 0));//numerals

		/* ATTENTION. Dont set location for EXTERNAL_OES texture. The next(!) called shader will fail.*/
		//GLCHK(glUniform1i(pong_shader.uniform_locations[0], 1));//tex

		// Bind Score texture
		glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texScore.id);

    // Bind the OES texture which is used to render the camera preview
//		glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_EXTERNAL_OES, raspitex_state->texture);
		glActiveTexture(GL_TEXTURE0);
#else
		//GLCHK(glUniform1i(pong_shader.uniform_locations[0], 0));//tex
		glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, raspitex_state->texture);
#endif

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
#if new_shader > 0
    GLCHK(glUniform1f(pong_shader.uniform_locations[1], offset));//offset
    GLCHK(glUniform2f(pong_shader.uniform_locations[3-1], BORDER, 1.0-BORDER));//border
    GLCHK(glUniform2f(pong_shader.uniform_locations[4-1], (float) score[0], (float) score[1]));//score
    GLCHK(glUniform4f(pong_shader.uniform_locations[5-1], 0.0, 0.25, 0.5, 0.75)); //scorePosLeft
    GLCHK(glUniform4f(pong_shader.uniform_locations[6-1], 0.5, 0.25, 1.0, 0.75)); //scorePosRight
#else
    GLCHK(glUniform1f(pong_shader.uniform_locations[1], offset));
#endif


    GLCHK(glDrawArrays(GL_TRIANGLES, 0, 6));

    GLCHK(glDisableVertexAttribArray(pong_shader.attribute_locations[0]));

		/*glActiveTexture(GL_TEXTURE1);
		GLCHK(glBindTexture(GL_TEXTURE_2D, 0));
		glActiveTexture(GL_TEXTURE0);
		*/
		GLCHK(glBindTexture(GL_TEXTURE_2D, 0));

    GLCHK(glUseProgram(0));

#if 1
		GLCHK(glEnable(GL_BLEND));
		GLCHK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		GLCHK(glFlush());
		RedrawTextures();
#endif

		static int counter_fb = 0;
		if( ++counter_fb == 200 ){
			raspitexutil_save_framebuffer("/dev/shm/fb.png");
			printf("Saved framebuffer in frame %i.\n", counter_fb-1);
		}

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
