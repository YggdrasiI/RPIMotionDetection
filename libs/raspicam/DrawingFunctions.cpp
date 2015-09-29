
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
		glBindFramebuffer(GL_FRAMEBUFFER,render_target->GetFramebufferId());
		glViewport ( 0, 0, render_target->GetWidth(), render_target->GetHeight() );
		check();
	}

	glUseProgram(GSimpleProg.GetId());	check();

	glUniform2f(GSimpleProg.GetHandle("offset"),x0,y0);
	glUniform2f(GSimpleProg.GetHandle("scale"),x1-x0,y1-y0);
	glUniform1i(GSimpleProg.GetHandle("tex"), 0);
	glUniform1f(GSimpleProg.GetHandle("alpha"),alpha);
	check();

	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBuffer);	check();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,texture->GetId());	check();

	GLuint loc = glGetAttribLocation(GSimpleProg.GetId(),"vertex");
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
		glBindFramebuffer(GL_FRAMEBUFFER,render_target->GetFramebufferId());
		glViewport ( 0, 0, render_target->GetWidth(), render_target->GetHeight() );
		check();
	}

	glUseProgram(GBlobProg.GetId());	check();

	glUniform2f(GBlobProg.GetHandle("offset"),x0,y0);
	glUniform2f(GBlobProg.GetHandle("scale"),x1-x0,y1-y0);
	glUniform3f(GBlobProg.GetHandle("blobcol"),r,g,b);
	check();

	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBuffer);	check();

	GLuint loc = glGetAttribLocation(GBlobProg.GetId(),"vertex");
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
		glBindFramebuffer(GL_FRAMEBUFFER,render_target->GetFramebufferId());
		glViewport ( 0, 0, render_target->GetWidth(), render_target->GetHeight() );

		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glClear(GL_DEPTH_BUFFER_BIT);
		//glClear(GL_COLOR_BUFFER_BIT);

		check();
	}

	if( numRects > 0 ){
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glUseProgram(GBlobsProg.GetId());	check();

		GLuint vloc = glGetAttribLocation(GBlobsProg.GetId(),"vertex");
		GLuint cloc = glGetAttribLocation(GBlobsProg.GetId(),"vertexColor");

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
		glBindFramebuffer(GL_FRAMEBUFFER,render_target->GetFramebufferId());
		glViewport ( 0, 0, render_target->GetWidth(), render_target->GetHeight() );
	}

	if( numPoints > 0 ){
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glUseProgram(GColouredLinesProg.GetId());	check();

		GLuint vloc = glGetAttribLocation(GColouredLinesProg.GetId(),"vertex");
		GLuint cloc = glGetAttribLocation(GColouredLinesProg.GetId(),"vertexColor");

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


