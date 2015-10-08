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

#include "motion.h"
#include "RaspiTex.h"
#include "RaspiTexUtil.h"
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "RaspiImv.h"
#include "GraphicsStub.h"

/**
 * Draws an external EGL image and applies a sine wave distortion to create
 * a hall of motions effect.
 */
static RASPITEXUTIL_SHADER_PROGRAM_T motion_shader = {
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
		"uniform sampler2D guitex;\n"
    "varying vec2 texcoord;\n"
    "void main(void) {\n"
		"		    vec4 gui = texture2D(guitex, texcoord);\n"
    //"       gl_FragColor = texture2D(tex, texcoord);\n"
		"		    gl_FragColor.rgb = mix(texture2D(tex, texcoord).rgb, gui.rgb, gui.a );\n"
    "       gl_FragColor.a = 1.0;\n"
		//"				gl_FragColor.rgb = vec3(1.0,0.5,0.5);\n" 
    "}\n",
    .uniform_names = {"tex", "guitex"},
    .attribute_names = {"vertex"},
};
extern RASPITEXUTIL_TEXTURE_T guiBuffer; //connected with GfxTexture in C++ class

static const EGLint attribute_list[] =
{
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_NONE
};

static GLfloat varray[] = {
	-1.0f, -1.0f,
	 1.0f, -1.0f,
	 1.0f,  1.0f,

	-1.0f,  1.0f,
	-1.0f, -1.0f,
	 1.0f,  1.0f,
};


/**
 * Creates the OpenGL ES 2.X context and builds the shaders.
 * @param raspitex_state A pointer to the GL preview state.
 * @return Zero if successful.
 */
static int motion_init(RASPITEX_STATE *state)
{
    int rc = raspitexutil_gl_init_2_0(state);
    if (rc != 0)
       goto end;

    rc = raspitexutil_build_shader_program(&motion_shader);

		//Call initialisation functions in c++ part (Graphics.cpp)
		InitTextures(state->width, state->height);
		InitShaders();
end:
    return rc;
}


static void motion_video1(RASPITEX_STATE *raspitex_state) {

	GLCHK(glUseProgram(motion_shader.program));

	GLCHK(glUniform1i(motion_shader.uniform_locations[0], 0));
	GLCHK(glUniform1i(motion_shader.uniform_locations[1], 1));

	// Bind video texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, raspitex_state->texture);

	// Bind score texture
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, guiBuffer.id);

	GLCHK(glEnableVertexAttribArray(motion_shader.attribute_locations[0]));

	GLCHK(glVertexAttribPointer(motion_shader.attribute_locations[0], 2, GL_FLOAT, GL_FALSE, 0, varray));
	GLCHK(glDrawArrays(GL_TRIANGLES, 0, 6));

	GLCHK(glDisableVertexAttribArray(motion_shader.attribute_locations[0]));
	GLCHK(glBindTexture(GL_TEXTURE_2D, 0));
	glActiveTexture(GL_TEXTURE0);

}

static int motion_redraw(RASPITEX_STATE *raspitex_state) {

    // Start with a clear screen
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		RedrawGui();

		/*
    GLCHK(glUseProgram(motion_shader.program));
		GLCHK(glUniform1i(motion_shader.uniform_locations[0], 0));

    // Bind the OES texture which is used to render the camera preview
		glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, raspitex_state->texture);

		// Bind score texture
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, guiBuffer.id);

    GLCHK(glEnableVertexAttribArray(motion_shader.attribute_locations[0]));

    GLCHK(glVertexAttribPointer(motion_shader.attribute_locations[0], 2, GL_FLOAT, GL_FALSE, 0, varray));
    GLCHK(glDrawArrays(GL_TRIANGLES, 0, 6));

    GLCHK(glDisableVertexAttribArray(motion_shader.attribute_locations[0]));
		GLCHK(glBindTexture(GL_TEXTURE_2D, 0));
		glActiveTexture(GL_TEXTURE0);
    GLCHK(glUseProgram(0));
		*/
		motion_video1(raspitex_state);

		GLCHK(glEnable(GL_BLEND));
		GLCHK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		RedrawTextures();
		GLCHK(glDisable(GL_BLEND));

		update_fps();

    return 0;
}

int motion_open(RASPITEX_STATE *state)
{
	//use own attribs list
	//state->egl_config_attribs = attribute_list ;

	state->ops.gl_init = motion_init;
	state->ops.redraw = motion_redraw;
	state->ops.update_texture = raspitexutil_update_texture;
	return 0;
}
