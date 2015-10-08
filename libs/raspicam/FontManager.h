#include <cwchar>
#include <string.h>
#include <vector>
#include <mutex>

extern "C" {
#include <texture-atlas.h>
#include <texture-font.h>
}

#include "GfxProgram.h"

// Defined in Graphics.cpp
extern uint32_t GScreenWidth;
extern uint32_t GScreenHeight;
extern EGLDisplay GDisplay;
extern EGLSurface GSurface;
extern EGLContext GContext;

/* Some initialisation steps of the
 * font manger could only be done
 * if the 3D pipeline is already running.
 * Thus, you can not setup all settings in
 * the main function.
 * Use a handler of this type to provide a
 * function which load the used fonts, etc.
 * It will be called at the end of initShaders()
 */
class FontManager;
typedef void (*initFontHandler)(FontManager *fontManager);

struct shader_t {
	GfxShader vertex_shader;
	GfxShader fragment_shader;
	GfxProgram program;
};

class FontManager{
	protected:
		//store 9 floats ( xyz, st and rgba) for each vertex.
		vector_t *verticesData;
		std::vector<texture_font_t*> fonts;
		texture_atlas_t *atlas;
		shader_t shader;
		// Should be called after OpenGL initialisation to load shaders, etc.
		initFontHandler m_initHandle;
		/* Block access during operations (add_text, clear_text, render)
		 * on verticesData to made class threadsave.
		 */
		std::mutex m_verticesMutex;
		bool m_text_changed;

	public:
		FontManager();
		~FontManager();

		/* OpenGLES shader initialisation. Should be
		 * done AFTER creation of OpenGL context.
		 */
		void initShaders();

		void setInitFunc( initFontHandler handle); 

		/* Adding font file to internal atlas.
			 Example add_font( "./fonts/custom.ttf", 50 );
			 */
		void add_font(std::string font_file, const float font_size);

		std::vector<texture_font_t*> *getFonts();

		/* Get vertices data (coordinates, textur coords and colour) for all characters. */
		//vector_t *getVerticesData();

		/* Extend vertices data by new characters
		 *
		 * Use getFonts()[id] to map on interal font (see add_font()) or map 
		 * to externally defined texture_font_t.
		 * */
		void add_text( texture_font_t *font, const char *text, vec4 *color, vec2 *pos );
		void add_text( texture_font_t *font, const wchar_t *text, vec4 *color, vec2 *pos );

		// Remove all textes from rendering.
		void clear_text();

		// True if text changed after last render() call. 
		bool render_required();

		/* OpenGL Drawing on framebuffer or render_target. */
		void render(float x0, float y0, float x1, float y1, GfxTexture* render_target);

		void saveFontAtlas(int font_index, const char* fname);
};

