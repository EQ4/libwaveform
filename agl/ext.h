/**
 * Utility for getting OpenGL extension function pointers
 */

void agl_get_extensions();

#ifndef APIENTRYP
	#define APIENTRYP APIENTRY *
#endif

/* OpenGL 2.0 */
PFNGLATTACHSHADERPROC glAttachShader;
#if 0
static PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = NULL;
#endif
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLDELETESHADERPROC glDeleteShader;
#if 0
PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib_func;
PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform_func;
PFNGLGETATTACHEDSHADERSPROC glGetAttachedShaders_func;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation_func;
#endif
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
#if 0
PFNGLGETSHADERSOURCEPROC glGetShaderSource;
PFNGLGETUNIFORMFVPROC glGetUniformfv;
#endif
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLISPROGRAMPROC glIsProgram;
PFNGLISSHADERPROC glIsShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM2IPROC glUniform2i_func;
PFNGLUNIFORM3IPROC glUniform3i_func;
PFNGLUNIFORM4IPROC glUniform4i_func;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM2FPROC glUniform2f_func;
PFNGLUNIFORM3FPROC glUniform3f_func;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORM1FVPROC glUniform1fv;
PFNGLUNIFORM2FVPROC glUniform2fv;
PFNGLUNIFORM3FVPROC glUniform3fv;
PFNGLUNIFORM4FVPROC glUniform4fv;
PFNGLUNIFORM1IVPROC glUniform1iv;
#if 0
static PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv_func = NULL;
static PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv_func = NULL;
#endif
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;

typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
PFNGLUSEPROGRAMPROC glUseProgram;
#if 0
static PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f_func = NULL;
static PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f_func = NULL;
static PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f_func = NULL;
static PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f_func = NULL;
static PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv_func = NULL;
static PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv_func = NULL;
static PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv_func = NULL;
static PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv_func = NULL;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer_func = NULL;
#endif
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
#if 0
static PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray_func = NULL;

/* OpenGL 2.1 */
static PFNGLUNIFORMMATRIX2X3FVPROC glUniformMatrix2x3fv_func = NULL;
static PFNGLUNIFORMMATRIX3X2FVPROC glUniformMatrix3x2fv_func = NULL;
static PFNGLUNIFORMMATRIX2X4FVPROC glUniformMatrix2x4fv_func = NULL;
static PFNGLUNIFORMMATRIX4X2FVPROC glUniformMatrix4x2fv_func = NULL;
static PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv_func = NULL;
static PFNGLUNIFORMMATRIX4X3FVPROC glUniformMatrix4x3fv_func = NULL;

/* OpenGL 1.4 */
static PFNGLPOINTPARAMETERFVPROC glPointParameterfv_func = NULL;
static PFNGLSECONDARYCOLOR3FVPROC glSecondaryColor3fv_func = NULL;

/* GL_ARB_vertex/fragment_program */
static PFNGLBINDPROGRAMARBPROC glBindProgramARB_func = NULL;
static PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB_func = NULL;
static PFNGLGENPROGRAMSARBPROC glGenProgramsARB_func = NULL;
static PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glGetProgramLocalParameterdvARB_func = NULL;
static PFNGLISPROGRAMARBPROC glIsProgramARB_func = NULL;
static PFNGLPROGRAMLOCALPARAMETER4DARBPROC glProgramLocalParameter4dARB_func = NULL;
static PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glProgramLocalParameter4fvARB_func = NULL;
static PFNGLPROGRAMSTRINGARBPROC glProgramStringARB_func = NULL;
static PFNGLVERTEXATTRIB1FARBPROC glVertexAttrib1fARB_func = NULL;

/* GL_APPLE_vertex_array_object */
#endif
PFNGLBINDVERTEXARRAYAPPLEPROC glBindVertexArrayAPPLE;
#if 0
static PFNGLDELETEVERTEXARRAYSAPPLEPROC glDeleteVertexArraysAPPLE_func = NULL;
static PFNGLGENVERTEXARRAYSAPPLEPROC glGenVertexArraysAPPLE_func = NULL;
static PFNGLISVERTEXARRAYAPPLEPROC glIsVertexArrayAPPLE_func = NULL;

/* GL_EXT_stencil_two_side */
static PFNGLACTIVESTENCILFACEEXTPROC glActiveStencilFaceEXT_func = NULL;

/* GL_ARB_buffer_object */
static PFNGLGENBUFFERSARBPROC glGenBuffersARB_func = NULL;
static PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB_func = NULL;
static PFNGLBINDBUFFERARBPROC glBindBufferARB_func = NULL;
static PFNGLBUFFERDATAARBPROC glBufferDataARB_func = NULL;
static PFNGLBUFFERSUBDATAARBPROC glBufferSubDataARB_func = NULL;
static PFNGLMAPBUFFERARBPROC glMapBufferARB_func = NULL;
static PFNGLUNMAPBUFFERARBPROC glUnmapBufferARB_func = NULL;

/* GL_EXT_framebuffer_object */
static PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT_func = NULL;
static PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT_func = NULL;
static PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT_func = NULL;
static PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT_func = NULL;
static PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT_func = NULL;
static PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT_func = NULL;
static PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT_func = NULL;
static PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT_func = NULL;
static PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT = NULL;
static PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = NULL;
static PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT_func = NULL;
static PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT_func = NULL;
static PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT_func = NULL;
static PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT_func = NULL;
static PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT_func;
static PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT_func;

/* GL_ARB_framebuffer_object */
static PFNGLISRENDERBUFFERPROC glIsRenderbuffer_func;
#endif
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv;
#if 0
static PFNGLISFRAMEBUFFERPROC glIsFramebuffer_func;
#endif
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
PFNGLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1D;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
#if 0
static PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D_func;
#endif
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv;
#if 0
static PFNGLGENERATEMIPMAPPROC glGenerateMipmap_func;
static PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer_func;
static PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample_func;
static PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer_func;
#endif
PFNGLSTRINGMARKERGREMEDYPROC glStringMarkerGREMEDY;

