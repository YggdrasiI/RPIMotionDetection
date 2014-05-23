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

#define WITH_TWO_TEXTURES 1

#if 1
static RASPITEXUTIL_SHADER_PROGRAM_T pong_shader = {
    .vertex_source =
    "attribute vec2 vertex;\n"
    "varying vec2 texcoord;\n"
    "void main(void) {\n"
    "   texcoord = 0.5 * (1.0 - vertex );\n"
    "   gl_Position = vec4(vertex, 0.0, 1.0);\n"
    "}\n",

    .fragment_source =
    "#extension GL_OES_EGL_image_external : require\n"
    "uniform samplerExternalOES aaatex;\n"
		"uniform sampler2D guitex;\n"
    "varying vec2 texcoord;\n"
    "const float waves = 2.0;\n"
    "void main(void) {\n"
    "    float x = texcoord.x ;\n"
    "    float y = texcoord.y ;\n"
    "    if (y < 1.0 && y > 0.0 && x < 1.0 && x > 0.0) {\n"
		"     vec4 gui = texture2D(guitex, texcoord);\n"
		"     gl_FragColor.rgb = mix(texture2D(aaatex, texcoord).rgb, gui.rgb, gui.a );\n"
    "    }\n"
    "    else {\n"
    "       gl_FragColor = vec4(0.0, 0.0, 0.5, 1.0);\n"
    "    }\n"
		"    gl_FragColor.a = 1.0;\n"
    "}\n",
    .uniform_names = {"aaatex", "guitex"},
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
		"uniform sampler2D texGui;\n"
    "const float waves = 2.0;\n"
    "varying vec2 texcoord;\n"
    "void main(void) {\n"
    "    float x = texcoord.x;\n"
    "    float y = texcoord.y;\n"
    "    if (y < 1.0 && y > 0.0 && x < 1.0 && x > 0.0) {\n"
#if WITH_TWO_TEXTURES > 0		
		"	      gl_FragColor = mix(texture2D(tex, texcoord),texture2D(texGui, texcoord),0.5);\n"
#else
		"	      gl_FragColor = texture2D(tex, texcoord);\n"
#endif
		"	      gl_FragColor.a = 1.0;\n"
    "    }\n"
    "    else {\n"
    "       gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "    }\n"
    "}\n",
#if WITH_TWO_TEXTURES > 0		
    .uniform_names = {"tex", "texGui"},
#else
    .uniform_names = {"tex"},
#endif
    .attribute_names = {"vertex"},
};
#endif

static RASPITEXUTIL_TEXTURE_T cameraBuffer; //for fany output effects
RASPITEXUTIL_TEXTURE_T guiBuffer; //connected with GfxTexture in C++ class
static unsigned char render_into_framebuffer = 0;
static uint32_t GScreenWidth;
static uint32_t GScreenHeight;
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

		//printf("Loading numerals\n");
		texScore = raspitexutil_load_texture("../../images/numerals.png");
		//texScore = raspitexutil_load_texture("../../images/test.png");
		//printf("Id: %i, W: %i, H:%i\n", texScore.id, texScore.width, texScore.height);
		//raspitexutil_create_framebuffer(&texScore);
		//printf("Framebuffer created.\n");
		//raspitexutil_save_texture("/dev/shm/texture.png", &texScore);
		//printf("Saved Texture\n");
		
		graphics_get_display_size(0 /* LCD */, &GScreenWidth, &GScreenHeight);
		cameraBuffer = raspitexutil_create_texture_rgba(GScreenWidth, GScreenHeight, 0, NULL);
		raspitexutil_create_framebuffer(&cameraBuffer);

end:
    return rc;
}

#if 1
static int pong_redraw(RASPITEX_STATE *raspitex_state) {
    static float offset = 0.0;

    // Start with a clear screen
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GLCHK(glDisable(GL_BLEND));
		RedrawGui();

		//GLCHK(glDisable(GL_BLEND));
		GLCHK(glUseProgram(pong_shader.program));

		GLCHK(glUniform1i(pong_shader.uniform_locations[0], 0));
		GLCHK(glUniform1i(pong_shader.uniform_locations[1], 1));

		// Bind Score texture
		glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, raspitex_state->texture);

		glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, guiBuffer.id);

    offset += 0.05;
    //GLCHK(glUseProgram(pong_shader.program));
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
#define BORDER 0.15
    //GLCHK(glUniform1f(pong_shader.uniform_locations[2], offset));//offset

    GLCHK(glDrawArrays(GL_TRIANGLES, 0, 6));

    GLCHK(glDisableVertexAttribArray(pong_shader.attribute_locations[0]));

		GLCHK(glBindTexture(GL_TEXTURE_2D, 0));
		glActiveTexture(GL_TEXTURE0);

#if 1
		RedrawTextures();
#endif

    return 0;
}
#else
static int pong_redraw(RASPITEX_STATE *raspitex_state) {

    // Start with a clear screen
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		GLCHK(glDisable(GL_BLEND));
		RedrawGui();

		if( render_into_framebuffer ){
			/* Render OES image into framebuffer to store it for some more complex 
			 * shaders */

		}else{

			GLCHK(glEnable(GL_BLEND));
			GLCHK(glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA));

			GLCHK(glUseProgram(pong_shader.program));

#if WITH_TWO_TEXTURES > 0
			GLCHK(glUniform1f(pong_shader.uniform_locations[0], 0));
			GLCHK(glUniform1f(pong_shader.uniform_locations[1], 1));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, raspitex_state->texture);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, guiBuffer.id);
#else
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, raspitex_state->texture);
#endif

			GLCHK(glEnableVertexAttribArray(pong_shader.attribute_locations[0]));

			static const GLfloat varray[] = {
				-1.0f, -1.0f,
				1.0f,  1.0f,
				1.0f, -1.0f,

				-1.0f,  1.0f,
				1.0f,  1.0f,
				-1.0f, -1.0f,
			};
			GLCHK(glVertexAttribPointer(pong_shader.attribute_locations[0], 2, GL_FLOAT, GL_FALSE, 0, varray));

			GLCHK(glDrawArrays(GL_TRIANGLES, 0, 6));

			GLCHK(glDisableVertexAttribArray(pong_shader.attribute_locations[0]));
		}

    //GLCHK(glUseProgram(0));

		GLCHK(glBindTexture(GL_TEXTURE_2D, 0));
		glActiveTexture(GL_TEXTURE0);


#if 1
//		GLCHK(glEnable(GL_BLEND));
		GLCHK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		//RedrawTextures();
		GLCHK(glDisable(GL_BLEND));
#endif

#if 0
		static int counter_fb = 0;
		if( ++counter_fb == 1000 ){
			printf("Saving…  ");
			raspitexutil_save_framebuffer("/dev/shm/fb.png");
			printf("Saved framebuffer in frame %i.\n", counter_fb-1);
		}
#endif

    return 0;
}
#endif

int pong_open(RASPITEX_STATE *state)
{
	//use own attribs list
	//state->egl_config_attribs = attribute_list ;

	state->ops.gl_init = pong_init;
	state->ops.redraw = pong_redraw;
	state->ops.update_texture = raspitexutil_update_texture;
	return 0;
}
