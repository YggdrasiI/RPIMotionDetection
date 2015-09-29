#include <cwchar>
#include <string.h>
#include <vector>

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

struct shader_t {
    GfxShader vertex_shader;
    GfxShader fragment_shader;
    GfxProgram program;
};

class FontManager{
  protected:
      //store 9 floats ( xyz, st and rgba) for each vertex.
    vector_t *verticesData = vector_new(sizeof(GLfloat));
    std::vector<texture_font_t*> fonts;
    texture_atlas_t *atlas;
    shader_t shader;

    public:
    FontManager();
    ~FontManager();

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
    void add_text( texture_font_t *font, wchar_t *text, vec4 *color, vec2 *pos );

    /* OpenGL Drawing on framebuffer or render_target. */
    void Render(float x0, float y0, float x1, float y1, GfxTexture* render_target);
};

