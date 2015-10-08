#include <stdio.h>
#include <string.h>
#include <cstring>
#include <assert.h>

#include "lodepng.h"

#include "FontManager.h"

FontManager::FontManager():
	m_initHandle(NULL),
	m_verticesMutex(),
	m_text_changed(false)
{
	// Init font texture plate
	this->verticesData = vector_new(sizeof(GLfloat));
	this->atlas = texture_atlas_new( 1024, 1024, 1 );
	this->fonts = std::vector<texture_font_t*>();
}

FontManager::~FontManager(){
	for (std::vector<texture_font_t*>::iterator it = this->fonts.begin();
			it != this->fonts.end(); ++it){
		if( *it != NULL){
			texture_font_delete(*it);
		}
	}
	texture_atlas_delete(this->atlas);
}

void FontManager::initShaders(){
	// Init shader program
	this->shader.vertex_shader.loadVertexShader("shader/fontrendering/font1.vert.glsl");
	this->shader.fragment_shader.loadFragmentShader("shader/fontrendering/font1.frag.glsl");
	this->shader.program.create(&this->shader.vertex_shader, &this->shader.fragment_shader);
	check();

	if( m_initHandle != NULL){
		(*m_initHandle)(this);
	}
}

void FontManager::setInitFunc( initFontHandler handle){ 
	this->m_initHandle = handle;
}

/* Example fonts in shader/fontrendering/fonts/ */
void FontManager::add_font(std::string font_file, const float font_size){
	texture_font_t *font = texture_font_new( this->atlas, font_file.c_str(), font_size);
	assert( font != NULL);

	/* Cache some glyphs to speed things up */
	texture_font_load_glyphs( font, L" !\"#$%&'()*+,-./0123456789:;<=>?"
			L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
			L"`abcdefghijklmnopqrstuvwxyz{|}~");
	this->fonts.push_back(font);
}

std::vector<texture_font_t*> *FontManager::getFonts(){
	return &(this->fonts);
}

/*vector_t *FontManager::getVerticesData(){
	return this->verticesData;
	}*/

/*texture_font_t *FontManager::getFont(size_t index){
	try {
	return this->fonts.at(index);
	}
	catch (const std::out_of_range& oor) {
	std::cerr << "Out of range error: " << oor.what() << '\n';
	return NULL;
	}
	}*/

void FontManager::add_text( texture_font_t *font, const char *text, vec4 *color, vec2 *pos ){
	size_t l = strlen(text)+1;
	wchar_t  wtext[l];
	swprintf(wtext, l, L"%hs", text);
	add_text( font, wtext, color, pos);
}

void FontManager::add_text( texture_font_t *font, const wchar_t *text, vec4 *color, vec2 *pos ){

	size_t i;
	GLfloat r = color->red, g = color->green, b = color->blue, a = color->alpha;
	for( i=0; i<wcslen(text); ++i )
	{
		texture_glyph_t *glyph = texture_font_get_glyph( font, text[i] );
		if( glyph != NULL )
		{
			int kerning = 0;
			if( i > 0)
			{
				kerning = texture_glyph_get_kerning( glyph, text[i-1] );
			}
			pos->x += kerning;
			//const GLint x0  = (int)( pos->x + glyph->offset_x );
			//const GLint y0  = (int)( pos->y + glyph->offset_y );
			//const GLint x1  = (int)( x0 + glyph->width );
			//const GLint y1  = (int)( y0 - glyph->height );
			const GLfloat x0  = (GLfloat)( pos->x + glyph->offset_x );
			const GLfloat y0  = (GLfloat)( pos->y + glyph->offset_y );
			const GLfloat x1  = (GLfloat)( x0 + glyph->width );
			const GLfloat y1  = (GLfloat)( y0 - glyph->height );
			const GLfloat s0 = glyph->s0;
			const GLfloat t0 = glyph->t0;
			const GLfloat s1 = glyph->s1;
			const GLfloat t1 = glyph->t1;

			// data is x,y,z,s,t,r,g,b,a
			const GLfloat vertices[] = {
				x0,y0,0,
				s0,t0,
				r, g, b, a,
				x0,y1,0,
				s0,t1,
				r, g, b, a,
				x1,y1,0,
				s1,t1,
				r, g, b, a,
				x0,y0,0,
				s0,t0,
				r, g, b, a,
				x1,y1,0,
				s1,t1,
				r, g, b, a,
				x1,y0,0,
				s1,t0,
				r, g, b, a
			};

			m_verticesMutex.lock();
			vector_push_back_data( this->verticesData, vertices, 9*6);
			m_verticesMutex.unlock();
			m_text_changed = true;

			pos->x += glyph->advance_x;
		}
	}
}

void FontManager::clear_text(){
			m_verticesMutex.lock();
			vector_clear( this->verticesData );
			m_verticesMutex.unlock();
			m_text_changed = true;
}

bool FontManager::render_required(){
	if( m_text_changed ){
		if( m_verticesMutex.try_lock() ){
			m_verticesMutex.unlock();
			return true;
		}
		// vertices vector locked
		return false;
	}
	// no changes
	return false;
}

void FontManager::render(float x0, float y0, float x1, float y1, GfxTexture* render_target)
{
	if(render_target)
	{
		glBindFramebuffer(GL_FRAMEBUFFER,render_target->getFramebufferId());
		glViewport ( 0, 0, render_target->getWidth(), render_target->getHeight() );
		check();
	}

	printf("Fontmanager.render()\n");
	glUseProgram(this->shader.program.getId());	check();

	const GLint vertexHandle = this->shader.program.getAttribLocation("a_position");
	const GLint texCoordHandle = this->shader.program.getAttribLocation("a_st");
	const GLint colorHandle = this->shader.program.getAttribLocation("a_color");
	const GLint samplerHandle = this->shader.program.getUniformLocation("texure_uniform");
	const GLint mvpHandle = this->shader.program.getUniformLocation( "u_mvp");
	const vector_t * const vVector = this->verticesData;

	/*
		 a 0 0 X    c -s 0 0   ac -as 0 X    cp 0 -sp 0    a*cy*cp -a*sp  -sp*a*cy X
		 0 b 0 Y *  s  c 0 0 = bs  bc 0 Y ,*  0 1   0 0 =  b*sy*cp  b*cy  -sp*b*cy Y
		 0 0 1 0    0  0 1 0   0    0 1 0    sp 0  cp 0    sp       0      cp      0
		 0 0 0 1    0  0 0 1   0    0 0 1     0 0   0 1    0        0      0       1
		 */
	// Pitch, roll and yaw
	float yaw = M_PI;//0.0; //Rotating by 180° required (unknown reason).
	float pitch = 0.0;//0.0;
	float sy = sin(yaw);
	float cy = cos(yaw);
	float sp = sin(pitch);
	float cp = cos(pitch);
	float a = 1.0f/(render_target?render_target->getWidth():GScreenWidth); //1.0=>x_i diff
	float b = 1.0f/(render_target?render_target->getHeight():GScreenHeight);
	// Set scaling so model coords are screen coords
	GLfloat mvp[] = {
		a*cy*cp, -a*sy, -sp*a*cy, 0/*-x0*/,
		b*sy*cp, b*cy, -sp*b*cy, 0/*-y0*/,
		sp, 0, cp, 0,
		0, 0, 0, 1.0
	};

	glUniformMatrix4fv(mvpHandle, 1, GL_FALSE, (GLfloat *) mvp);

	m_verticesMutex.lock();
	// Load the vertex data
	glVertexAttribPointer ( vertexHandle, 3, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), vVector->items );
	glEnableVertexAttribArray ( vertexHandle );
	glVertexAttribPointer ( texCoordHandle, 2, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), (GLfloat*)vVector->items+3 );
	glEnableVertexAttribArray ( texCoordHandle );
	glVertexAttribPointer ( colorHandle, 4, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), (GLfloat*)vVector->items+5 );
	glEnableVertexAttribArray ( colorHandle );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, atlas->id );
	
	glUniform1i(samplerHandle, 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);

	glDrawArrays ( GL_TRIANGLES, 0, vVector->size/9 );

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisableVertexAttribArray( vertexHandle );
	glDisableVertexAttribArray ( texCoordHandle );
	glDisableVertexAttribArray ( colorHandle );

	m_verticesMutex.unlock();
	m_text_changed = false;

	if(render_target)
	{
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glViewport ( 0, 0, GScreenWidth, GScreenHeight );
	}
}

void FontManager::saveFontAtlas(int font_index, const char* fname)
{
	texture_atlas_t *tex = this->fonts[font_index]->atlas;

	unsigned error = lodepng::encode(fname, (const unsigned char*)tex->data, tex->width, tex->height, tex->depth>1 ? LCT_RGBA : LCT_GREY);
	if(error) 
		printf("error: %d\n",error);
}
