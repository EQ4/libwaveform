#ifndef __agl_typedefs_h__
#define __agl_typedefs_h__

typedef struct _AGlShader         AGlShader;
typedef struct _alphamap_shader   AlphaMapShader;
typedef struct _plain_shader      PlainShader;
typedef struct _texture_unit      AGlTextureUnit; 
typedef struct _AGlFBO            AGlFBO;
typedef struct _AGlActor          AGlActor;
typedef struct _AGlRootActor      AGlRootActor;
typedef struct _AGlRootActor      AGlScene;
typedef struct _AGlTextureActor   AGlTextureActor;

typedef struct {int x, y;}             AGliPt;
typedef struct {float x, y;}           AGlfPt;
typedef struct {int x1, y1, x2, y2;}   AGliRegion;
typedef struct {float x, y, w, h;}     AGlRect;
typedef struct {float x0, y0, x1, y1;} AGlQuad;
typedef struct {float r, g, b;}        AGlColourFloat;


#ifndef bool
#  define bool int
#endif

#define AGL_ENABLE_BLEND        (1<<1)
#define AGL_ENABLE_TEXTURE_2D   (1<<2)
#define AGL_ENABLE_ALPHA_TEST   (1<<3)
#define AGL_ENABLE_TEXTURE_RECT (1<<4)

#endif //__agl_typedefs_h__
