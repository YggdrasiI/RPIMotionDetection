
#include "Graphics.h"
#include "DrawingFunctions.h"

// Defined in Graphics.cpp
extern uint32_t GScreenWidth;
extern uint32_t GScreenHeight;
extern EGLDisplay GDisplay;
extern EGLSurface GSurface;
extern EGLContext GContext;

/* List of all shader programs which
 * will be used in the drawing functions
 * of this file.
 * The shaders are NOT loaded automaticaly.
 * Initialise in your local Graphics.cpp all GfxPrograms 
 * which will be used in your application.
 * ( Search for InitShaders() functions. )
 * */
extern GfxProgram GSimpleProg;
extern GfxProgram GBlobProg;
extern GfxProgram GPongProg;
extern GfxProgram GBlobsProg;
extern GfxProgram GColouredLinesProg;
//extern GfxProgram GFontProg;

extern GLuint GQuadVertexBuffer;




void DrawTextureRect(GfxTexture* texture, float alpha, float x0, float y0, float x1, float y1, GfxTexture* render_target)
{
	if(render_target)
	{
		glBindFramebuffer(GL_FRAMEBUFFER,render_target->getFramebufferId());
		glViewport ( 0, 0, render_target->getWidth(), render_target->getHeight() );
		check();
	}

	glUseProgram(GSimpleProg.getId());	check();

	glUniform2f(GSimpleProg.getUniformLocation("offset"),x0,y0);
	glUniform2f(GSimpleProg.getUniformLocation("scale"),x1-x0,y1-y0);
	glUniform1i(GSimpleProg.getUniformLocation("tex"), 0);
	glUniform1f(GSimpleProg.getUniformLocation("alpha"),alpha);
	check();

	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBuffer);	check();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,texture->getId());	check();

	GLuint loc = glGetAttribLocation(GSimpleProg.getId(),"vertex");
	glVertexAttribPointer(loc, 4, GL_FLOAT, 0, 16, 0);	check();
	glEnableVertexAttribArray(loc);	check();
	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 ); check();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	if(render_target)
	{
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glViewport ( 0, 0, GScreenWidth, GScreenHeight );
	}
}

void DrawBlobRect(float r, float g, float b, float x0, float y0, float x1, float y1, GfxTexture* render_target)
{
	if(render_target)
	{
		glBindFramebuffer(GL_FRAMEBUFFER,render_target->getFramebufferId());
		glViewport ( 0, 0, render_target->getWidth(), render_target->getHeight() );
		check();
	}

	glUseProgram(GBlobProg.getId());	check();

	glUniform2f(GBlobProg.getUniformLocation("offset"),x0,y0);
	glUniform2f(GBlobProg.getUniformLocation("scale"),x1-x0,y1-y0);
	glUniform3f(GBlobProg.getUniformLocation("blobcol"),r,g,b);
	check();

	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBuffer);	check();

	GLuint loc = glGetAttribLocation(GBlobProg.getId(),"vertex");
	glVertexAttribPointer(loc, 4, GL_FLOAT, 0, 16, 0);	check();
	glEnableVertexAttribArray(loc);	check();
	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 ); check();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	if(render_target)
	{
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glViewport ( 0, 0, GScreenWidth, GScreenHeight );
	}
}

void DrawBlobRects(GLfloat *vertices, GLfloat *colors, GLfloat numRects, GfxTexture* render_target)
{
	if(render_target )
	{
		glBindFramebuffer(GL_FRAMEBUFFER,render_target->getFramebufferId());
		glViewport ( 0, 0, render_target->getWidth(), render_target->getHeight() );

		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glClear(GL_DEPTH_BUFFER_BIT);
		//glClear(GL_COLOR_BUFFER_BIT);

		check();
	}

	if( numRects > 0 ){
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glUseProgram(GBlobsProg.getId());	check();

		GLuint vloc = glGetAttribLocation(GBlobsProg.getId(),"vertex");
		GLuint cloc = glGetAttribLocation(GBlobsProg.getId(),"vertexColor");

		glEnableVertexAttribArray(vloc);	
		glEnableVertexAttribArray(cloc);	check();

		glVertexAttribPointer(vloc, 2, GL_FLOAT, GL_FALSE, 0, vertices);
		glVertexAttribPointer(cloc, 4, GL_FLOAT, GL_FALSE, 0, colors);

		glDrawArrays ( GL_TRIANGLES, 0, numRects*6 ); check();

		glDisableVertexAttribArray(vloc);	
		glDisableVertexAttribArray(cloc);	check();

		glDisable(GL_BLEND);
	}

	if(render_target )
	{
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glViewport ( 0, 0, GScreenWidth, GScreenHeight );
	}
}

void DrawColouredLines(GLfloat *vertices, GLfloat *colors, GLfloat numPoints, GfxTexture* render_target)
{
	if(render_target )
	{
		glBindFramebuffer(GL_FRAMEBUFFER,render_target->getFramebufferId());
		glViewport ( 0, 0, render_target->getWidth(), render_target->getHeight() );
	}

	if( numPoints > 0 ){
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glUseProgram(GColouredLinesProg.getId());	check();

		GLuint vloc = glGetAttribLocation(GColouredLinesProg.getId(),"vertex");
		GLuint cloc = glGetAttribLocation(GColouredLinesProg.getId(),"vertexColor");

		glEnableVertexAttribArray(vloc);	
		glEnableVertexAttribArray(cloc);	check();

		glVertexAttribPointer(vloc, 2, GL_FLOAT, GL_FALSE, 0, vertices);
		glVertexAttribPointer(cloc, 4, GL_FLOAT, GL_FALSE, 0, colors);

		glLineWidth(3.0);
		glDrawArrays ( GL_LINE_STRIP, 0, numPoints ); check();
		glLineWidth(1.0);

		glDisableVertexAttribArray(vloc);	
		glDisableVertexAttribArray(cloc);	check();

		//glDisable(GL_BLEND);
	}

	if(render_target )
	{
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glViewport ( 0, 0, GScreenWidth, GScreenHeight );
	}
}


