/**********************************************************************************************
*
*   rlgl v6.0 - A multi-OpenGL abstraction layer with an immediate-mode style API
*
*   DESCRIPTION:
*       An abstraction layer for multiple OpenGL versions (1.1, 2.1, 3.3 Core, 4.3 Core, ES 2.0, ES 3.0)
*       that provides a pseudo-OpenGL 1.1 immediate-mode style API (rlVertex, rlTranslate, rlRotate...)
*
*   ADDITIONAL NOTES:
*       When choosing an OpenGL backend different than OpenGL 1.1, some internal buffers are
*       initialized on rlglInit() to accumulate vertex data
*
*       When an internal state change is required all the stored vertex data is rendered in a batch,
*       additionally, rlDrawRenderBatchActive() could be called to force flushing of the batch
*
*       Some resources are also loaded for convenience, here the complete list:
*          - Default batch (RLGL.defaultBatch): RenderBatch system to accumulate vertex data
*          - Default texture (RLGL.defaultTextureId): 1x1 white pixel R8G8B8A8
*          - Default shader (RLGL.State.defaultShaderId, RLGL.State.defaultShaderLocs)
*
*       Internal buffer (and resources) must be manually unloaded calling rlglClose()
*
*   CONFIGURATION:
*       #define GRAPHICS_API_OPENGL_SOFTWARE
*       #define GRAPHICS_API_OPENGL_11
*       #define GRAPHICS_API_OPENGL_21
*       #define GRAPHICS_API_OPENGL_33
*       #define GRAPHICS_API_OPENGL_43
*       #define GRAPHICS_API_OPENGL_ES2
*       #define GRAPHICS_API_OPENGL_ES3
*           Use selected OpenGL graphics backend, should be supported by platform
*           Those preprocessor defines are only used on the rlgl module, if OpenGL version is
*           required by any other module, use rlGetVersion() to check it
*
*       #define RLGL_IMPLEMENTATION
*           Generates the implementation of the library into the included file
*           If not defined, the library is in header only mode and can be included in other headers
*           or source files without problems. But only ONE file should hold the implementation
*
*       #if RLGL_SHOW_GL_DETAILS_INFO
*           Show OpenGL extensions and capabilities detailed logs on init
*
*       #if RLGL_ENABLE_OPENGL_DEBUG_CONTEXT
*           Enable debug context (only available on OpenGL 4.3)
*
*       rlgl capabilities could be customized defining some internal
*       values before library inclusion (default values listed):
*
*       #define RL_DEFAULT_BATCH_BUFFER_ELEMENTS   8192    // Default internal render batch elements limits
*       #define RL_DEFAULT_BATCH_BUFFERS              1    // Default number of batch buffers (multi-buffering)
*       #define RL_DEFAULT_BATCH_DRAWCALLS          256    // Default number of batch draw calls (by state changes: mode, texture)
*       #define RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS    4    // Maximum number of texture units that can be activated on batch drawing (SetShaderValueTexture())
*
*       #define RL_MAX_MATRIX_STACK_SIZE             32    // Maximum size of internal Matrix stack
*       #define RL_MAX_SHADER_LOCATIONS              32    // Maximum number of shader locations supported
*       #define RL_CULL_DISTANCE_NEAR              0.05    // Default projection matrix near cull distance
*       #define RL_CULL_DISTANCE_FAR             4000.0    // Default projection matrix far cull distance
*
*       When loading a shader, the following vertex attributes and uniform
*       location names are tried to be set automatically:
*
*       #define RL_DEFAULT_SHADER_ATTRIB_NAME_POSITION     "vertexPosition"    // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION
*       #define RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD     "vertexTexCoord"    // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD
*       #define RL_DEFAULT_SHADER_ATTRIB_NAME_NORMAL       "vertexNormal"      // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL
*       #define RL_DEFAULT_SHADER_ATTRIB_NAME_COLOR        "vertexColor"       // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR
*       #define RL_DEFAULT_SHADER_ATTRIB_NAME_TANGENT      "vertexTangent"     // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT
*       #define RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD2    "vertexTexCoord2"   // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2
*       #define RL_DEFAULT_SHADER_ATTRIB_NAME_BONEINDICES  "vertexBoneIndices" // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEINDICES
*       #define RL_DEFAULT_SHADER_ATTRIB_NAME_BONEWEIGHTS  "vertexBoneWeights" // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEWEIGHTS
*       #define RL_DEFAULT_SHADER_UNIFORM_NAME_MVP         "mvp"               // model-view-projection matrix
*       #define RL_DEFAULT_SHADER_UNIFORM_NAME_VIEW        "matView"           // view matrix
*       #define RL_DEFAULT_SHADER_UNIFORM_NAME_PROJECTION  "matProjection"     // projection matrix
*       #define RL_DEFAULT_SHADER_UNIFORM_NAME_MODEL       "matModel"          // model matrix
*       #define RL_DEFAULT_SHADER_UNIFORM_NAME_NORMAL      "matNormal"         // normal matrix (transpose(inverse(matModelView)))
*       #define RL_DEFAULT_SHADER_UNIFORM_NAME_COLOR       "colDiffuse"        // color diffuse (base tint color, multiplied by texture color)
*       #define RL_DEFAULT_SHADER_UNIFORM_NAME_BONEMATRICES "boneMatrices"     // bone matrices
*       #define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE0  "texture0"          // texture0 (texture slot active 0)
*       #define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE1  "texture1"          // texture1 (texture slot active 1)
*       #define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE2  "texture2"          // texture2 (texture slot active 2)
*
*   DEPENDENCIES:
*      - OpenGL libraries (depending on platform and OpenGL version selected)
*      - GLAD OpenGL extensions loading library (only for OpenGL 3.3 Core, 4.3 Core)
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2014-2026 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#ifndef RLGL_H
#define RLGL_H

#define RLGL_VERSION  "6.0"

// Function specifiers in case library is build/used as a shared library
// NOTE: Microsoft specifiers to tell compiler that symbols are imported/exported from a .dll
// NOTE: visibility(default) attribute makes symbols "visible" when compiled with -fvisibility=hidden
#if defined(_WIN32) && defined(BUILD_LIBTYPE_SHARED)
    #define RLAPI __declspec(dllexport)     // Building the library as a Win32 shared library (.dll)
#elif defined(BUILD_LIBTYPE_SHARED)
    #define RLAPI __attribute__((visibility("default"))) // Building the library as a Unix shared library (.so/.dylib)
#elif defined(_WIN32) && defined(USE_LIBTYPE_SHARED)
    #define RLAPI __declspec(dllimport)     // Using the library as a Win32 shared library (.dll)
#endif

// Function specifiers definition
#ifndef RLAPI
    #define RLAPI       // Functions defined as 'extern' by default (implicit specifiers)
#endif

// Support TRACELOG macros
#ifndef TRACELOG
    #define TRACELOG(level, ...) (void)0
#endif

// Allow custom memory allocators
#ifndef RL_MALLOC
    #define RL_MALLOC(sz)     malloc(sz)
#endif
#ifndef RL_CALLOC
    #define RL_CALLOC(n,sz)   calloc(n,sz)
#endif
#ifndef RL_REALLOC
    #define RL_REALLOC(n,sz)  realloc(n,sz)
#endif
#ifndef RL_FREE
    #define RL_FREE(p)        free(p)
#endif

// Security check in case no GRAPHICS_API_OPENGL_* defined
#if !defined(GRAPHICS_API_OPENGL_SOFTWARE) && \
    !defined(GRAPHICS_API_OPENGL_11) && \
    !defined(GRAPHICS_API_OPENGL_21) && \
    !defined(GRAPHICS_API_OPENGL_33) && \
    !defined(GRAPHICS_API_OPENGL_43) && \
    !defined(GRAPHICS_API_OPENGL_ES2) && \
    !defined(GRAPHICS_API_OPENGL_ES3)
        #define GRAPHICS_API_OPENGL_33
#endif

// Security check in case multiple GRAPHICS_API_OPENGL_* defined
#if defined(GRAPHICS_API_OPENGL_11) || defined(GRAPHICS_API_OPENGL_SOFTWARE)
    #if defined(GRAPHICS_API_OPENGL_21)
        #undef GRAPHICS_API_OPENGL_21
    #endif
    #if defined(GRAPHICS_API_OPENGL_33)
        #undef GRAPHICS_API_OPENGL_33
    #endif
    #if defined(GRAPHICS_API_OPENGL_43)
        #undef GRAPHICS_API_OPENGL_43
    #endif
    #if defined(GRAPHICS_API_OPENGL_ES2)
        #undef GRAPHICS_API_OPENGL_ES2
    #endif
#endif

// Software implementation uses OpenGL 1.1 functionality
#if defined(GRAPHICS_API_OPENGL_SOFTWARE)
    #define GRAPHICS_API_OPENGL_11
#endif

// OpenGL 2.1 uses most of OpenGL 3.3 Core functionality
// WARNING: Specific parts are checked with #if defines
#if defined(GRAPHICS_API_OPENGL_21)
    #define GRAPHICS_API_OPENGL_33
#endif

// OpenGL 4.3 uses OpenGL 3.3 Core functionality
#if defined(GRAPHICS_API_OPENGL_43)
    #define GRAPHICS_API_OPENGL_33
#endif

// OpenGL ES 3.0 uses OpenGL ES 2.0 functionality (and more)
#if defined(GRAPHICS_API_OPENGL_ES3)
    #define GRAPHICS_API_OPENGL_ES2
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------

// Default internal render batch elements limits
#ifndef RL_DEFAULT_BATCH_BUFFER_ELEMENTS
    #if defined(GRAPHICS_API_OPENGL_11) || defined(GRAPHICS_API_OPENGL_33)
        // This is the maximum amount of elements (quads) per batch
        // NOTE: Be careful with text, every letter maps to a quad
        #define RL_DEFAULT_BATCH_BUFFER_ELEMENTS  8192
    #endif
    #if defined(GRAPHICS_API_OPENGL_ES2)
        // Reducing memory sizes for embedded systems (RPI and HTML5)
        // NOTE: On HTML5 (emscripten) this is allocated on heap,
        // by default heap is only 16MB!...just take care...
        #define RL_DEFAULT_BATCH_BUFFER_ELEMENTS  2048
    #endif
#endif
#ifndef RL_DEFAULT_BATCH_BUFFERS
    #define RL_DEFAULT_BATCH_BUFFERS                 1      // Default number of batch buffers (multi-buffering)
#endif
#ifndef RL_DEFAULT_BATCH_DRAWCALLS
    #define RL_DEFAULT_BATCH_DRAWCALLS             256      // Default number of batch draw calls (by state changes: mode, texture)
#endif
#ifndef RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS
    #define RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS       4      // Maximum number of textures units that can be activated on batch drawing (SetShaderValueTexture())
#endif

// Internal Matrix stack
#ifndef RL_MAX_MATRIX_STACK_SIZE
    #define RL_MAX_MATRIX_STACK_SIZE                32      // Maximum size of Matrix stack
#endif

// Shader limits
#ifndef RL_MAX_SHADER_LOCATIONS
    #define RL_MAX_SHADER_LOCATIONS                 32      // Maximum number of shader locations supported
#endif

// Projection matrix culling
#ifndef RL_CULL_DISTANCE_NEAR
    #define RL_CULL_DISTANCE_NEAR                 0.05      // Default near cull distance
#endif
#ifndef RL_CULL_DISTANCE_FAR
    #define RL_CULL_DISTANCE_FAR                4000.0      // Default far cull distance
#endif

// Texture parameters (equivalent to OpenGL defines)
#define RL_TEXTURE_WRAP_S                       0x2802      // GL_TEXTURE_WRAP_S
#define RL_TEXTURE_WRAP_T                       0x2803      // GL_TEXTURE_WRAP_T
#define RL_TEXTURE_MAG_FILTER                   0x2800      // GL_TEXTURE_MAG_FILTER
#define RL_TEXTURE_MIN_FILTER                   0x2801      // GL_TEXTURE_MIN_FILTER

#define RL_TEXTURE_FILTER_NEAREST               0x2600      // GL_NEAREST
#define RL_TEXTURE_FILTER_LINEAR                0x2601      // GL_LINEAR
#define RL_TEXTURE_FILTER_MIP_NEAREST           0x2700      // GL_NEAREST_MIPMAP_NEAREST
#define RL_TEXTURE_FILTER_NEAREST_MIP_LINEAR    0x2702      // GL_NEAREST_MIPMAP_LINEAR
#define RL_TEXTURE_FILTER_LINEAR_MIP_NEAREST    0x2701      // GL_LINEAR_MIPMAP_NEAREST
#define RL_TEXTURE_FILTER_MIP_LINEAR            0x2703      // GL_LINEAR_MIPMAP_LINEAR
#define RL_TEXTURE_FILTER_ANISOTROPIC           0x3000      // Anisotropic filter (custom identifier)
#define RL_TEXTURE_MIPMAP_BIAS_RATIO            0x4000      // Texture mipmap bias, percentage ratio (custom identifier)

#define RL_TEXTURE_WRAP_REPEAT                  0x2901      // GL_REPEAT
#define RL_TEXTURE_WRAP_CLAMP                   0x812F      // GL_CLAMP_TO_EDGE
#define RL_TEXTURE_WRAP_MIRROR_REPEAT           0x8370      // GL_MIRRORED_REPEAT
#define RL_TEXTURE_WRAP_MIRROR_CLAMP            0x8742      // GL_MIRROR_CLAMP_EXT

// Matrix modes (equivalent to OpenGL)
#define RL_MODELVIEW                            0x1700      // GL_MODELVIEW
#define RL_PROJECTION                           0x1701      // GL_PROJECTION
#define RL_TEXTURE                              0x1702      // GL_TEXTURE

// Primitive assembly draw modes
#define RL_LINES                                0x0001      // GL_LINES
#define RL_TRIANGLES                            0x0004      // GL_TRIANGLES
#define RL_QUADS                                0x0007      // GL_QUADS

// GL equivalent data types
#define RL_UNSIGNED_BYTE                        0x1401      // GL_UNSIGNED_BYTE
#define RL_FLOAT                                0x1406      // GL_FLOAT

// GL buffer usage hint
#define RL_STREAM_DRAW                          0x88E0      // GL_STREAM_DRAW
#define RL_STREAM_READ                          0x88E1      // GL_STREAM_READ
#define RL_STREAM_COPY                          0x88E2      // GL_STREAM_COPY
#define RL_STATIC_DRAW                          0x88E4      // GL_STATIC_DRAW
#define RL_STATIC_READ                          0x88E5      // GL_STATIC_READ
#define RL_STATIC_COPY                          0x88E6      // GL_STATIC_COPY
#define RL_DYNAMIC_DRAW                         0x88E8      // GL_DYNAMIC_DRAW
#define RL_DYNAMIC_READ                         0x88E9      // GL_DYNAMIC_READ
#define RL_DYNAMIC_COPY                         0x88EA      // GL_DYNAMIC_COPY

// GL Shader type
#define RL_FRAGMENT_SHADER                      0x8B30      // GL_FRAGMENT_SHADER
#define RL_VERTEX_SHADER                        0x8B31      // GL_VERTEX_SHADER
#define RL_COMPUTE_SHADER                       0x91B9      // GL_COMPUTE_SHADER

// GL blending factors
#define RL_ZERO                                 0           // GL_ZERO
#define RL_ONE                                  1           // GL_ONE
#define RL_SRC_COLOR                            0x0300      // GL_SRC_COLOR
#define RL_ONE_MINUS_SRC_COLOR                  0x0301      // GL_ONE_MINUS_SRC_COLOR
#define RL_SRC_ALPHA                            0x0302      // GL_SRC_ALPHA
#define RL_ONE_MINUS_SRC_ALPHA                  0x0303      // GL_ONE_MINUS_SRC_ALPHA
#define RL_DST_ALPHA                            0x0304      // GL_DST_ALPHA
#define RL_ONE_MINUS_DST_ALPHA                  0x0305      // GL_ONE_MINUS_DST_ALPHA
#define RL_DST_COLOR                            0x0306      // GL_DST_COLOR
#define RL_ONE_MINUS_DST_COLOR                  0x0307      // GL_ONE_MINUS_DST_COLOR
#define RL_SRC_ALPHA_SATURATE                   0x0308      // GL_SRC_ALPHA_SATURATE
#define RL_CONSTANT_COLOR                       0x8001      // GL_CONSTANT_COLOR
#define RL_ONE_MINUS_CONSTANT_COLOR             0x8002      // GL_ONE_MINUS_CONSTANT_COLOR
#define RL_CONSTANT_ALPHA                       0x8003      // GL_CONSTANT_ALPHA
#define RL_ONE_MINUS_CONSTANT_ALPHA             0x8004      // GL_ONE_MINUS_CONSTANT_ALPHA

// GL blending functions/equations
#define RL_FUNC_ADD                             0x8006      // GL_FUNC_ADD
#define RL_MIN                                  0x8007      // GL_MIN
#define RL_MAX                                  0x8008      // GL_MAX
#define RL_FUNC_SUBTRACT                        0x800A      // GL_FUNC_SUBTRACT
#define RL_FUNC_REVERSE_SUBTRACT                0x800B      // GL_FUNC_REVERSE_SUBTRACT
#define RL_BLEND_EQUATION                       0x8009      // GL_BLEND_EQUATION
#define RL_BLEND_EQUATION_RGB                   0x8009      // GL_BLEND_EQUATION_RGB   // (Same as BLEND_EQUATION)
#define RL_BLEND_EQUATION_ALPHA                 0x883D      // GL_BLEND_EQUATION_ALPHA
#define RL_BLEND_DST_RGB                        0x80C8      // GL_BLEND_DST_RGB
#define RL_BLEND_SRC_RGB                        0x80C9      // GL_BLEND_SRC_RGB
#define RL_BLEND_DST_ALPHA                      0x80CA      // GL_BLEND_DST_ALPHA
#define RL_BLEND_SRC_ALPHA                      0x80CB      // GL_BLEND_SRC_ALPHA
#define RL_BLEND_COLOR                          0x8005      // GL_BLEND_COLOR

#define RL_READ_FRAMEBUFFER                     0x8CA8      // GL_READ_FRAMEBUFFER
#define RL_DRAW_FRAMEBUFFER                     0x8CA9      // GL_DRAW_FRAMEBUFFER

// Default shader vertex attribute locations
#ifndef RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION
    #define RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION    0
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD
    #define RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD    1
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL
    #define RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL      2
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR
    #define RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR       3
#endif
    #ifndef RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT
#define RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT         4
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2
    #define RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2   5
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_LOCATION_INDICES
    #define RL_DEFAULT_SHADER_ATTRIB_LOCATION_INDICES     6
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEINDICES
    #define RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEINDICES 7
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEWEIGHTS
    #define RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEWEIGHTS 8
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_LOCATION_INSTANCETRANSFORM
    #define RL_DEFAULT_SHADER_ATTRIB_LOCATION_INSTANCETRANSFORM 9
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
#if (defined(__STDC__) && __STDC_VERSION__ >= 199901L) || (defined(_MSC_VER) && _MSC_VER >= 1800)
    #include <stdbool.h>
#elif !defined(__cplusplus) && !defined(bool) && !defined(RL_BOOL_TYPE)
    // Boolean type
typedef enum bool { false = 0, true = !false } bool;
#endif

#if !defined(RL_MATRIX_TYPE)
// Matrix, 4x4 components, column major, OpenGL style, right handed
typedef struct Matrix {
    float m0, m4, m8, m12;      // Matrix first row (4 components)
    float m1, m5, m9, m13;      // Matrix second row (4 components)
    float m2, m6, m10, m14;     // Matrix third row (4 components)
    float m3, m7, m11, m15;     // Matrix fourth row (4 components)
} Matrix;
#define RL_MATRIX_TYPE
#endif

// Dynamic vertex buffers (position + texcoords + colors + indices arrays)
typedef struct rlVertexBuffer {
    int elementCount;           // Number of elements in the buffer (QUADS)

    float *vertices;            // Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
    float *texcoords;           // Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
    float *normals;             // Vertex normal (XYZ - 3 components per vertex) (shader-location = 2)
    unsigned char *colors;      // Vertex colors (RGBA - 4 components per vertex) (shader-location = 3)
#if defined(GRAPHICS_API_OPENGL_11) || defined(GRAPHICS_API_OPENGL_33)
    unsigned int *indices;      // Vertex indices (in case vertex data comes indexed) (6 indices per quad)
#endif
#if defined(GRAPHICS_API_OPENGL_ES2)
    unsigned short *indices;    // Vertex indices (in case vertex data comes indexed) (6 indices per quad)
#endif
    unsigned int vaoId;         // OpenGL Vertex Array Object id
    unsigned int vboId[5];      // OpenGL Vertex Buffer Objects id (5 types of vertex data)
} rlVertexBuffer;

// Draw call type
// NOTE: Only texture changes register a new draw, other state-change-related elements are not
// used at this moment (vaoId, shaderId, matrices), raylib forces a batch draw call if any
// of those state-change happens (this is done in core module)
typedef struct rlDrawCall {
    int mode;                   // Drawing mode: LINES, TRIANGLES, QUADS
    int vertexCount;            // Number of vertex of the draw
    int vertexAlignment;        // Number of vertex required for index alignment (LINES, TRIANGLES)
    //unsigned int vaoId;       // Vertex array id to be used on the draw -> Using RLGL.currentBatch->vertexBuffer.vaoId
    //unsigned int shaderId;    // Shader id to be used on the draw -> Using RLGL.currentShaderId
    unsigned int textureId;     // Texture id to be used on the draw -> Use to create new draw call if changes

    //Matrix projection;        // Projection matrix for this draw -> Using RLGL.projection by default
    //Matrix modelview;         // Modelview matrix for this draw -> Using RLGL.modelview by default
} rlDrawCall;

// rlRenderBatch type
typedef struct rlRenderBatch {
    int bufferCount;            // Number of vertex buffers (multi-buffering support)
    int currentBuffer;          // Current buffer tracking in case of multi-buffering
    rlVertexBuffer *vertexBuffer; // Dynamic buffer(s) for vertex data

    rlDrawCall *draws;          // Draw calls array, depends on textureId
    int drawCounter;            // Draw calls counter
    float currentDepth;         // Current depth value for next draw
} rlRenderBatch;

// OpenGL version
typedef enum {
    RL_OPENGL_SOFTWARE = 0,  // Software rendering
    RL_OPENGL_11,               // OpenGL 1.1
    RL_OPENGL_21,               // OpenGL 2.1 (GLSL 120)
    RL_OPENGL_33,               // OpenGL 3.3 (GLSL 330)
    RL_OPENGL_43,               // OpenGL 4.3 (using GLSL 330)
    RL_OPENGL_ES_20,            // OpenGL ES 2.0 (GLSL 100)
    RL_OPENGL_ES_30             // OpenGL ES 3.0 (GLSL 300 es)
} rlGlVersion;

// Trace log level
// NOTE: Organized by priority level
typedef enum {
    RL_LOG_ALL = 0,             // Display all logs
    RL_LOG_TRACE,               // Trace logging, intended for internal use only
    RL_LOG_DEBUG,               // Debug logging, used for internal debugging, it should be disabled on release builds
    RL_LOG_INFO,                // Info logging, used for program execution info
    RL_LOG_WARNING,             // Warning logging, used on recoverable failures
    RL_LOG_ERROR,               // Error logging, used on unrecoverable failures
    RL_LOG_FATAL,               // Fatal logging, used to abort program: exit(EXIT_FAILURE)
    RL_LOG_NONE                 // Disable logging
} rlTraceLogLevel;

// Texture pixel formats
// NOTE: Support depends on OpenGL version
typedef enum {
    RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE = 1,     // 8 bit per pixel (no alpha)
    RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA,        // 8*2 bpp (2 channels)
    RL_PIXELFORMAT_UNCOMPRESSED_R5G6B5,            // 16 bpp
    RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8,            // 24 bpp
    RL_PIXELFORMAT_UNCOMPRESSED_R5G5B5A1,          // 16 bpp (1 bit alpha)
    RL_PIXELFORMAT_UNCOMPRESSED_R4G4B4A4,          // 16 bpp (4 bit alpha)
    RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,          // 32 bpp
    RL_PIXELFORMAT_UNCOMPRESSED_R32,               // 32 bpp (1 channel - float)
    RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32,         // 32*3 bpp (3 channels - float)
    RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32,      // 32*4 bpp (4 channels - float)
    RL_PIXELFORMAT_UNCOMPRESSED_R16,               // 16 bpp (1 channel - half float)
    RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16,         // 16*3 bpp (3 channels - half float)
    RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,      // 16*4 bpp (4 channels - half float)
    RL_PIXELFORMAT_COMPRESSED_DXT1_RGB,            // 4 bpp (no alpha)
    RL_PIXELFORMAT_COMPRESSED_DXT1_RGBA,           // 4 bpp (1 bit alpha)
    RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA,           // 8 bpp
    RL_PIXELFORMAT_COMPRESSED_DXT5_RGBA,           // 8 bpp
    RL_PIXELFORMAT_COMPRESSED_ETC1_RGB,            // 4 bpp
    RL_PIXELFORMAT_COMPRESSED_ETC2_RGB,            // 4 bpp
    RL_PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA,       // 8 bpp
    RL_PIXELFORMAT_COMPRESSED_PVRT_RGB,            // 4 bpp
    RL_PIXELFORMAT_COMPRESSED_PVRT_RGBA,           // 4 bpp
    RL_PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA,       // 8 bpp
    RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA        // 2 bpp
} rlPixelFormat;

// Texture parameters: filter mode
// NOTE 1: Filtering considers mipmaps if available in the texture
// NOTE 2: Filter is accordingly set for minification and magnification
typedef enum {
    RL_TEXTURE_FILTER_POINT = 0,        // No filter, pixel approximation
    RL_TEXTURE_FILTER_BILINEAR,         // Linear filtering
    RL_TEXTURE_FILTER_TRILINEAR,        // Trilinear filtering (linear with mipmaps)
    RL_TEXTURE_FILTER_ANISOTROPIC_4X,   // Anisotropic filtering 4x
    RL_TEXTURE_FILTER_ANISOTROPIC_8X,   // Anisotropic filtering 8x
    RL_TEXTURE_FILTER_ANISOTROPIC_16X,  // Anisotropic filtering 16x
} rlTextureFilter;

// Color blending modes (pre-defined)
typedef enum {
    RL_BLEND_ALPHA = 0,                 // Blend textures considering alpha (default)
    RL_BLEND_ADDITIVE,                  // Blend textures adding colors
    RL_BLEND_MULTIPLIED,                // Blend textures multiplying colors
    RL_BLEND_ADD_COLORS,                // Blend textures adding colors (alternative)
    RL_BLEND_SUBTRACT_COLORS,           // Blend textures subtracting colors (alternative)
    RL_BLEND_ALPHA_PREMULTIPLY,         // Blend premultiplied textures considering alpha
    RL_BLEND_CUSTOM,                    // Blend textures using custom src/dst factors (use rlSetBlendFactors())
    RL_BLEND_CUSTOM_SEPARATE            // Blend textures using custom src/dst factors (use rlSetBlendFactorsSeparate())
} rlBlendMode;

// Shader location point type
typedef enum {
    RL_SHADER_LOC_VERTEX_POSITION = 0,  // Shader location: vertex attribute: position
    RL_SHADER_LOC_VERTEX_TEXCOORD01,    // Shader location: vertex attribute: texcoord01
    RL_SHADER_LOC_VERTEX_TEXCOORD02,    // Shader location: vertex attribute: texcoord02
    RL_SHADER_LOC_VERTEX_NORMAL,        // Shader location: vertex attribute: normal
    RL_SHADER_LOC_VERTEX_TANGENT,       // Shader location: vertex attribute: tangent
    RL_SHADER_LOC_VERTEX_COLOR,         // Shader location: vertex attribute: color
    RL_SHADER_LOC_MATRIX_MVP,           // Shader location: matrix uniform: model-view-projection
    RL_SHADER_LOC_MATRIX_VIEW,          // Shader location: matrix uniform: view (camera transform)
    RL_SHADER_LOC_MATRIX_PROJECTION,    // Shader location: matrix uniform: projection
    RL_SHADER_LOC_MATRIX_MODEL,         // Shader location: matrix uniform: model (transform)
    RL_SHADER_LOC_MATRIX_NORMAL,        // Shader location: matrix uniform: normal
    RL_SHADER_LOC_VECTOR_VIEW,          // Shader location: vector uniform: view
    RL_SHADER_LOC_COLOR_DIFFUSE,        // Shader location: vector uniform: diffuse color
    RL_SHADER_LOC_COLOR_SPECULAR,       // Shader location: vector uniform: specular color
    RL_SHADER_LOC_COLOR_AMBIENT,        // Shader location: vector uniform: ambient color
    RL_SHADER_LOC_MAP_ALBEDO,           // Shader location: sampler2d texture: albedo (same as: RL_SHADER_LOC_MAP_DIFFUSE)
    RL_SHADER_LOC_MAP_METALNESS,        // Shader location: sampler2d texture: metalness (same as: RL_SHADER_LOC_MAP_SPECULAR)
    RL_SHADER_LOC_MAP_NORMAL,           // Shader location: sampler2d texture: normal
    RL_SHADER_LOC_MAP_ROUGHNESS,        // Shader location: sampler2d texture: roughness
    RL_SHADER_LOC_MAP_OCCLUSION,        // Shader location: sampler2d texture: occlusion
    RL_SHADER_LOC_MAP_EMISSION,         // Shader location: sampler2d texture: emission
    RL_SHADER_LOC_MAP_HEIGHT,           // Shader location: sampler2d texture: height
    RL_SHADER_LOC_MAP_CUBEMAP,          // Shader location: samplerCube texture: cubemap
    RL_SHADER_LOC_MAP_IRRADIANCE,       // Shader location: samplerCube texture: irradiance
    RL_SHADER_LOC_MAP_PREFILTER,        // Shader location: samplerCube texture: prefilter
    RL_SHADER_LOC_MAP_BRDF              // Shader location: sampler2d texture: brdf
} rlShaderLocationIndex;

#define RL_SHADER_LOC_MAP_DIFFUSE       RL_SHADER_LOC_MAP_ALBEDO
#define RL_SHADER_LOC_MAP_SPECULAR      RL_SHADER_LOC_MAP_METALNESS

// Shader uniform data type
typedef enum {
    RL_SHADER_UNIFORM_FLOAT = 0,        // Shader uniform type: float
    RL_SHADER_UNIFORM_VEC2,             // Shader uniform type: vec2 (2 float)
    RL_SHADER_UNIFORM_VEC3,             // Shader uniform type: vec3 (3 float)
    RL_SHADER_UNIFORM_VEC4,             // Shader uniform type: vec4 (4 float)
    RL_SHADER_UNIFORM_INT,              // Shader uniform type: int
    RL_SHADER_UNIFORM_IVEC2,            // Shader uniform type: ivec2 (2 int)
    RL_SHADER_UNIFORM_IVEC3,            // Shader uniform type: ivec3 (3 int)
    RL_SHADER_UNIFORM_IVEC4,            // Shader uniform type: ivec4 (4 int)
    RL_SHADER_UNIFORM_UINT,             // Shader uniform type: unsigned int
    RL_SHADER_UNIFORM_UIVEC2,           // Shader uniform type: uivec2 (2 unsigned int)
    RL_SHADER_UNIFORM_UIVEC3,           // Shader uniform type: uivec3 (3 unsigned int)
    RL_SHADER_UNIFORM_UIVEC4,           // Shader uniform type: uivec4 (4 unsigned int)
    RL_SHADER_UNIFORM_SAMPLER2D         // Shader uniform type: sampler2d
} rlShaderUniformDataType;

// Shader attribute data types
typedef enum {
    RL_SHADER_ATTRIB_FLOAT = 0,         // Shader attribute type: float
    RL_SHADER_ATTRIB_VEC2,              // Shader attribute type: vec2 (2 float)
    RL_SHADER_ATTRIB_VEC3,              // Shader attribute type: vec3 (3 float)
    RL_SHADER_ATTRIB_VEC4               // Shader attribute type: vec4 (4 float)
} rlShaderAttributeDataType;

// Framebuffer attachment type
// NOTE: By default up to 8 color channels defined, but it can be more
typedef enum {
    RL_ATTACHMENT_COLOR_CHANNEL0 = 0,       // Framebuffer attachment type: color 0
    RL_ATTACHMENT_COLOR_CHANNEL1 = 1,       // Framebuffer attachment type: color 1
    RL_ATTACHMENT_COLOR_CHANNEL2 = 2,       // Framebuffer attachment type: color 2
    RL_ATTACHMENT_COLOR_CHANNEL3 = 3,       // Framebuffer attachment type: color 3
    RL_ATTACHMENT_COLOR_CHANNEL4 = 4,       // Framebuffer attachment type: color 4
    RL_ATTACHMENT_COLOR_CHANNEL5 = 5,       // Framebuffer attachment type: color 5
    RL_ATTACHMENT_COLOR_CHANNEL6 = 6,       // Framebuffer attachment type: color 6
    RL_ATTACHMENT_COLOR_CHANNEL7 = 7,       // Framebuffer attachment type: color 7
    RL_ATTACHMENT_DEPTH = 100,              // Framebuffer attachment type: depth
    RL_ATTACHMENT_STENCIL = 200,            // Framebuffer attachment type: stencil
} rlFramebufferAttachType;

// Framebuffer texture attachment type
typedef enum {
    RL_ATTACHMENT_CUBEMAP_POSITIVE_X = 0,   // Framebuffer texture attachment type: cubemap, +X side
    RL_ATTACHMENT_CUBEMAP_NEGATIVE_X = 1,   // Framebuffer texture attachment type: cubemap, -X side
    RL_ATTACHMENT_CUBEMAP_POSITIVE_Y = 2,   // Framebuffer texture attachment type: cubemap, +Y side
    RL_ATTACHMENT_CUBEMAP_NEGATIVE_Y = 3,   // Framebuffer texture attachment type: cubemap, -Y side
    RL_ATTACHMENT_CUBEMAP_POSITIVE_Z = 4,   // Framebuffer texture attachment type: cubemap, +Z side
    RL_ATTACHMENT_CUBEMAP_NEGATIVE_Z = 5,   // Framebuffer texture attachment type: cubemap, -Z side
    RL_ATTACHMENT_TEXTURE2D = 100,          // Framebuffer texture attachment type: texture2d
    RL_ATTACHMENT_RENDERBUFFER = 200,       // Framebuffer texture attachment type: renderbuffer
} rlFramebufferAttachTextureType;

// Face culling mode
typedef enum {
    RL_CULL_FACE_FRONT = 0,
    RL_CULL_FACE_BACK
} rlCullMode;

//------------------------------------------------------------------------------------
// Functions Declaration - Matrix operations
//------------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C" {            // Prevents name mangling of functions
#endif

RLAPI void rlMatrixMode(int mode);                      // Choose the current matrix to be transformed
RLAPI void rlPushMatrix(void);                          // Push the current matrix to stack
RLAPI void rlPopMatrix(void);                           // Pop latest inserted matrix from stack
RLAPI void rlLoadIdentity(void);                        // Reset current matrix to identity matrix
RLAPI void rlTranslatef(float x, float y, float z);     // Multiply the current matrix by a translation matrix
RLAPI void rlRotatef(float angle, float x, float y, float z); // Multiply the current matrix by a rotation matrix
RLAPI void rlScalef(float x, float y, float z);         // Multiply the current matrix by a scaling matrix
RLAPI void rlMultMatrixf(const float *matf);            // Multiply the current matrix by another matrix
RLAPI void rlFrustum(double left, double right, double bottom, double top, double znear, double zfar);
RLAPI void rlOrtho(double left, double right, double bottom, double top, double znear, double zfar);
RLAPI void rlViewport(int x, int y, int width, int height); // Set the viewport area
RLAPI void rlSetClipPlanes(double nearPlane, double farPlane);    // Set clip planes distances
RLAPI double rlGetCullDistanceNear(void);               // Get cull plane distance near
RLAPI double rlGetCullDistanceFar(void);                // Get cull plane distance far

//------------------------------------------------------------------------------------
// Functions Declaration - Vertex level operations
//------------------------------------------------------------------------------------
RLAPI void rlBegin(int mode);                           // Initialize drawing mode (how to organize vertex)
RLAPI void rlEnd(void);                                 // Finish vertex providing
RLAPI void rlVertex2i(int x, int y);                    // Define one vertex (position) - 2 int
RLAPI void rlVertex2f(float x, float y);                // Define one vertex (position) - 2 float
RLAPI void rlVertex3f(float x, float y, float z);       // Define one vertex (position) - 3 float
RLAPI void rlTexCoord2f(float x, float y);              // Define one vertex (texture coordinate) - 2 float
RLAPI void rlNormal3f(float x, float y, float z);       // Define one vertex (normal) - 3 float
RLAPI void rlColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a); // Define one vertex (color) - 4 byte
RLAPI void rlColor3f(float x, float y, float z);        // Define one vertex (color) - 3 float
RLAPI void rlColor4f(float x, float y, float z, float w); // Define one vertex (color) - 4 float

//------------------------------------------------------------------------------------
// Functions Declaration - OpenGL style functions (common to 1.1, 3.3+, ES2)
// NOTE: This functions are used to completely abstract raylib code from OpenGL layer,
// some of them are direct wrappers over OpenGL calls, some others are custom
//------------------------------------------------------------------------------------

// Vertex buffers state
RLAPI bool rlEnableVertexArray(unsigned int vaoId);     // Enable vertex array (VAO, if supported)
RLAPI void rlDisableVertexArray(void);                  // Disable vertex array (VAO, if supported)
RLAPI void rlEnableVertexBuffer(unsigned int id);       // Enable vertex buffer (VBO)
RLAPI void rlDisableVertexBuffer(void);                 // Disable vertex buffer (VBO)
RLAPI void rlEnableVertexBufferElement(unsigned int id); // Enable vertex buffer element (VBO element)
RLAPI void rlDisableVertexBufferElement(void);          // Disable vertex buffer element (VBO element)
RLAPI void rlEnableVertexAttribute(unsigned int index); // Enable vertex attribute index
RLAPI void rlDisableVertexAttribute(unsigned int index); // Disable vertex attribute index
RLAPI void rlEnableStatePointer(int vertexAttribType, void *buffer); // Enable attribute state pointer
RLAPI void rlDisableStatePointer(int vertexAttribType); // Disable attribute state pointer

// Textures state
RLAPI void rlActiveTextureSlot(int slot);               // Select and active a texture slot
RLAPI void rlEnableTexture(unsigned int id);            // Enable texture
RLAPI void rlDisableTexture(void);                      // Disable texture
RLAPI void rlEnableTextureCubemap(unsigned int id);     // Enable texture cubemap
RLAPI void rlDisableTextureCubemap(void);               // Disable texture cubemap
RLAPI void rlTextureParameters(unsigned int id, int param, int value); // Set texture parameters (filter, wrap)
RLAPI void rlCubemapParameters(unsigned int id, int param, int value); // Set cubemap parameters (filter, wrap)

// Shader state
RLAPI void rlEnableShader(unsigned int id);             // Enable shader program
RLAPI void rlDisableShader(void);                       // Disable shader program

// Framebuffer state
RLAPI void rlEnableFramebuffer(unsigned int id);        // Enable render texture (fbo)
RLAPI void rlDisableFramebuffer(void);                  // Disable render texture (fbo), return to default framebuffer
RLAPI unsigned int rlGetActiveFramebuffer(void);        // Get the currently active render texture (fbo), 0 for default framebuffer
RLAPI void rlActiveDrawBuffers(int count);              // Activate multiple draw color buffers
RLAPI void rlBlitFramebuffer(int srcX, int srcY, int srcWidth, int srcHeight, int dstX, int dstY, int dstWidth, int dstHeight, int bufferMask); // Blit active framebuffer to main framebuffer
RLAPI void rlBindFramebuffer(unsigned int target, unsigned int framebuffer); // Bind framebuffer (FBO)

// General render state
RLAPI void rlEnableColorBlend(void);                    // Enable color blending
RLAPI void rlDisableColorBlend(void);                   // Disable color blending
RLAPI void rlEnableDepthTest(void);                     // Enable depth test
RLAPI void rlDisableDepthTest(void);                    // Disable depth test
RLAPI void rlEnableDepthMask(void);                     // Enable depth write
RLAPI void rlDisableDepthMask(void);                    // Disable depth write
RLAPI void rlEnableBackfaceCulling(void);               // Enable backface culling
RLAPI void rlDisableBackfaceCulling(void);              // Disable backface culling
RLAPI void rlColorMask(bool r, bool g, bool b, bool a); // Color mask control
RLAPI void rlSetCullFace(int mode);                     // Set face culling mode
RLAPI void rlEnableScissorTest(void);                   // Enable scissor test
RLAPI void rlDisableScissorTest(void);                  // Disable scissor test
RLAPI void rlScissor(int x, int y, int width, int height); // Scissor test
RLAPI void rlEnablePointMode(void);                     // Enable point mode
RLAPI void rlDisablePointMode(void);                    // Disable point mode
RLAPI void rlSetPointSize(float size);                  // Set the point drawing size
RLAPI float rlGetPointSize(void);                       // Get the point drawing size
RLAPI void rlEnableWireMode(void);                      // Enable wire mode
RLAPI void rlDisableWireMode(void);                     // Disable wire mode
RLAPI void rlSetLineWidth(float width);                 // Set the line drawing width
RLAPI float rlGetLineWidth(void);                       // Get the line drawing width
RLAPI void rlEnableSmoothLines(void);                   // Enable line aliasing
RLAPI void rlDisableSmoothLines(void);                  // Disable line aliasing
RLAPI void rlEnableStereoRender(void);                  // Enable stereo rendering
RLAPI void rlDisableStereoRender(void);                 // Disable stereo rendering
RLAPI bool rlIsStereoRenderEnabled(void);               // Check if stereo render is enabled

RLAPI void rlClearColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a); // Clear color buffer with color
RLAPI void rlClearScreenBuffers(void);                  // Clear used screen buffers (color and depth)
RLAPI void rlCheckErrors(void);                         // Check and log OpenGL error codes
RLAPI void rlSetBlendMode(int mode);                    // Set blending mode
RLAPI void rlSetBlendFactors(int glSrcFactor, int glDstFactor, int glEquation); // Set blending mode factor and equation (using OpenGL factors)
RLAPI void rlSetBlendFactorsSeparate(int glSrcRGB, int glDstRGB, int glSrcAlpha, int glDstAlpha, int glEqRGB, int glEqAlpha); // Set blending mode factors and equations separately (using OpenGL factors)

//------------------------------------------------------------------------------------
// Functions Declaration - rlgl functionality
//------------------------------------------------------------------------------------
// rlgl initialization functions
RLAPI void rlglInit(int width, int height);             // Initialize rlgl (buffers, shaders, textures, states)
RLAPI void rlglClose(void);                             // De-initialize rlgl (buffers, shaders, textures)
RLAPI void rlLoadExtensions(void *loader);              // Load OpenGL extensions (loader function required)
RLAPI void *rlGetProcAddress(const char *procName);     // Get OpenGL procedure address
RLAPI int rlGetVersion(void);                           // Get current OpenGL version
RLAPI void rlSetFramebufferWidth(int width);            // Set current framebuffer width
RLAPI int rlGetFramebufferWidth(void);                  // Get default framebuffer width
RLAPI void rlSetFramebufferHeight(int height);          // Set current framebuffer height
RLAPI int rlGetFramebufferHeight(void);                 // Get default framebuffer height

RLAPI unsigned int rlGetTextureIdDefault(void);         // Get default texture id
RLAPI unsigned int rlGetShaderIdDefault(void);          // Get default shader id
RLAPI int *rlGetShaderLocsDefault(void);                // Get default shader locations

// Render batch management
// NOTE: rlgl provides a default render batch to behave like OpenGL 1.1 immediate mode
// but this render batch API is exposed in case of custom batches are required
RLAPI rlRenderBatch rlLoadRenderBatch(int numBuffers, int bufferElements); // Load a render batch system
RLAPI void rlUnloadRenderBatch(rlRenderBatch batch);    // Unload render batch system
RLAPI void rlDrawRenderBatch(rlRenderBatch *batch);     // Draw render batch data (Update->Draw->Reset)
RLAPI void rlSetRenderBatchActive(rlRenderBatch *batch); // Set the active render batch for rlgl (NULL for default internal)
RLAPI void rlDrawRenderBatchActive(void);               // Update and draw internal render batch
RLAPI bool rlCheckRenderBatchLimit(int vCount);         // Check internal buffer overflow for a given number of vertex

RLAPI void rlSetTexture(unsigned int id);               // Set current texture for render batch and check buffers limits

//------------------------------------------------------------------------------------------------------------------------

// Vertex buffers management
RLAPI unsigned int rlLoadVertexArray(void);             // Load vertex array (vao) if supported
RLAPI unsigned int rlLoadVertexBuffer(const void *buffer, int size, bool dynamic); // Load a vertex buffer object
RLAPI unsigned int rlLoadVertexBufferElement(const void *buffer, int size, bool dynamic); // Load vertex buffer elements object
RLAPI void rlUpdateVertexBuffer(unsigned int bufferId, const void *data, int dataSize, int offset); // Update vertex buffer object data on GPU buffer
RLAPI void rlUpdateVertexBufferElements(unsigned int id, const void *data, int dataSize, int offset); // Update vertex buffer elements data on GPU buffer
RLAPI void rlUnloadVertexArray(unsigned int vaoId);     // Unload vertex array (vao)
RLAPI void rlUnloadVertexBuffer(unsigned int vboId);    // Unload vertex buffer object
RLAPI void rlSetVertexAttribute(unsigned int index, int compSize, int type, bool normalized, int stride, int offset); // Set vertex attribute data configuration
RLAPI void rlSetVertexAttributeDivisor(unsigned int index, int divisor); // Set vertex attribute data divisor
RLAPI void rlSetVertexAttributeDefault(int locIndex, const void *value, int attribType, int count); // Set vertex attribute default value, when attribute to provided
RLAPI void rlDrawVertexArray(int offset, int count);    // Draw vertex array (currently active vao)
RLAPI void rlDrawVertexArrayElements(int offset, int count, const void *buffer); // Draw vertex array elements
RLAPI void rlDrawVertexArrayInstanced(int offset, int count, int instances); // Draw vertex array (currently active vao) with instancing
RLAPI void rlDrawVertexArrayElementsInstanced(int offset, int count, const void *buffer, int instances); // Draw vertex array elements with instancing

// Textures management
RLAPI unsigned int rlLoadTexture(const void *data, int width, int height, int format, int mipmapCount); // Load texture data
RLAPI unsigned int rlLoadTextureDepth(int width, int height, bool useRenderBuffer); // Load depth texture/renderbuffer (to be attached to fbo)
RLAPI unsigned int rlLoadTextureCubemap(const void *data, int size, int format, int mipmapCount); // Load texture cubemap data
RLAPI void rlUpdateTexture(unsigned int id, int offsetX, int offsetY, int width, int height, int format, const void *data); // Update texture with new data on GPU
RLAPI void rlGetGlTextureFormats(int format, unsigned int *glInternalFormat, unsigned int *glFormat, unsigned int *glType); // Get OpenGL internal formats
RLAPI const char *rlGetPixelFormatName(unsigned int format);              // Get name string for pixel format
RLAPI void rlUnloadTexture(unsigned int id);                              // Unload texture from GPU memory
RLAPI void rlGenTextureMipmaps(unsigned int id, int width, int height, int format, int *mipmaps); // Generate mipmap data for selected texture
RLAPI void *rlReadTexturePixels(unsigned int id, int width, int height, int format); // Read texture pixel data
RLAPI unsigned char *rlReadScreenPixels(int width, int height);           // Read screen pixel data (color buffer)

// Framebuffer management (fbo)
RLAPI unsigned int rlLoadFramebuffer(void);                               // Load an empty framebuffer
RLAPI void rlFramebufferAttach(unsigned int id, unsigned int texId, int attachType, int texType, int mipLevel); // Attach texture/renderbuffer to a framebuffer
RLAPI bool rlFramebufferComplete(unsigned int id);                        // Verify framebuffer is complete
RLAPI void rlUnloadFramebuffer(unsigned int id);                          // Delete framebuffer from GPU
// WARNING: Copy and resize framebuffer functionality only defined for software backend
RLAPI void rlCopyFramebuffer(int x, int y, int width, int height, int format, void *pixels); // Copy framebuffer pixel data to internal buffer
RLAPI void rlResizeFramebuffer(int width, int height);                    // Resize internal framebuffer

// Shaders management
RLAPI unsigned int rlLoadShader(const char *code, int type);                    // Load (compile) shader and return shader id (type: RL_VERTEX_SHADER, RL_FRAGMENT_SHADER, RL_COMPUTE_SHADER)
RLAPI unsigned int rlLoadShaderProgram(const char *vsCode, const char *fsCode); // Load shader from code strings
RLAPI unsigned int rlLoadShaderProgramEx(unsigned int vsId, unsigned int fsId); // Load shader program, using already loaded shader ids
RLAPI unsigned int rlLoadShaderProgramCompute(unsigned int csId);               // Load compute shader program
RLAPI void rlUnloadShader(unsigned int id);                                     // Unload shader, loaded with rlLoadShader()
RLAPI void rlUnloadShaderProgram(unsigned int id);                              // Unload shader program
RLAPI int rlGetLocationUniform(unsigned int id, const char *uniformName);       // Get shader location uniform, requires shader program id
RLAPI int rlGetLocationAttrib(unsigned int id, const char *attribName);         // Get shader location attribute, requires shader program id
RLAPI void rlSetUniform(int locIndex, const void *value, int uniformType, int count); // Set shader value uniform
RLAPI void rlSetUniformMatrix(int locIndex, Matrix mat);                        // Set shader value matrix
RLAPI void rlSetUniformMatrices(int locIndex, const Matrix *mat, int count);    // Set shader value matrices
RLAPI void rlSetUniformSampler(int locIndex, unsigned int textureId);           // Set shader value sampler
RLAPI void rlSetShader(unsigned int id, int *locs);                             // Set shader currently active (id and locations)

// Compute shader management
RLAPI void rlComputeShaderDispatch(unsigned int groupX, unsigned int groupY, unsigned int groupZ); // Dispatch compute shader (equivalent to *draw* for graphics pipeline)

// Shader buffer storage object management (ssbo)
RLAPI unsigned int rlLoadShaderBuffer(unsigned int size, const void *data, int usageHint); // Load shader storage buffer object (SSBO)
RLAPI void rlUnloadShaderBuffer(unsigned int ssboId);                           // Unload shader storage buffer object (SSBO)
RLAPI void rlUpdateShaderBuffer(unsigned int id, const void *data, unsigned int dataSize, unsigned int offset); // Update SSBO buffer data
RLAPI void rlBindShaderBuffer(unsigned int id, unsigned int index);             // Bind SSBO buffer
RLAPI void rlReadShaderBuffer(unsigned int id, void *dest, unsigned int count, unsigned int offset); // Read SSBO buffer data (GPU->CPU)
RLAPI void rlCopyShaderBuffer(unsigned int destId, unsigned int srcId, unsigned int destOffset, unsigned int srcOffset, unsigned int count); // Copy SSBO data between buffers
RLAPI unsigned int rlGetShaderBufferSize(unsigned int id);                      // Get SSBO buffer size

// Buffer management
RLAPI void rlBindImageTexture(unsigned int id, unsigned int index, int format, bool readonly);  // Bind image texture

// Matrix state management
RLAPI Matrix rlGetMatrixModelview(void);                                  // Get internal modelview matrix
RLAPI Matrix rlGetMatrixProjection(void);                                 // Get internal projection matrix
RLAPI Matrix rlGetMatrixTransform(void);                                  // Get internal accumulated transform matrix
RLAPI Matrix rlGetMatrixProjectionStereo(int eye);                        // Get internal projection matrix for stereo render (selected eye)
RLAPI Matrix rlGetMatrixViewOffsetStereo(int eye);                        // Get internal view offset matrix for stereo render (selected eye)
RLAPI void rlSetMatrixProjection(Matrix proj);                            // Set a custom projection matrix (replaces internal projection matrix)
RLAPI void rlSetMatrixModelview(Matrix view);                             // Set a custom modelview matrix (replaces internal modelview matrix)
RLAPI void rlSetMatrixProjectionStereo(Matrix right, Matrix left);        // Set eyes projection matrices for stereo rendering
RLAPI void rlSetMatrixViewOffsetStereo(Matrix right, Matrix left);        // Set eyes view offsets matrices for stereo rendering

// Quick and dirty cube/quad buffers load->draw->unload
RLAPI void rlLoadDrawCube(void);     // Load and draw a cube
RLAPI void rlLoadDrawQuad(void);     // Load and draw a quad

#if defined(__cplusplus)
}
#endif

#endif // RLGL_H

/***********************************************************************************
*
*   RLGL IMPLEMENTATION
*
************************************************************************************/

#if defined(RLGL_IMPLEMENTATION)

// Expose OpenGL functions from glad in raylib
#if defined(BUILD_LIBTYPE_SHARED)
    #define GLAD_API_CALL_EXPORT
    #define GLAD_API_CALL_EXPORT_BUILD
#endif

#if defined(GRAPHICS_API_OPENGL_11)
    #if defined(GRAPHICS_API_OPENGL_SOFTWARE)
        #define RLSW_IMPLEMENTATION
        #define SW_MALLOC(sz) RL_MALLOC(sz)
        #define SW_CALLOC(n,sz) RL_CALLOC(n, sz)
        #define SW_REALLOC(ptr, newSz) RL_REALLOC(ptr, newSz)
        #define SW_FREE(ptr) RL_FREE(ptr)
        #include "external/rlsw.h"          // OpenGL 1.1 software implementation
    #else
        #if defined(__APPLE__)
            #include <OpenGL/gl.h>          // OpenGL 1.1 library for OSX
            #include <OpenGL/glext.h>       // OpenGL extensions library
        #else
            // APIENTRY for OpenGL function pointer declarations is required
            #if !defined(APIENTRY)
                #if defined(_WIN32)
                    #define APIENTRY __stdcall
                #else
                    #define APIENTRY
                #endif
            #endif
            // WINGDIAPI definition. Some Windows OpenGL headers need it
            #if !defined(WINGDIAPI) && defined(_WIN32)
                #define WINGDIAPI __declspec(dllimport)
            #endif

            #include <GL/gl.h>              // OpenGL 1.1 library
        #endif
    #endif
#endif

#if defined(GRAPHICS_API_OPENGL_33)
    #define GLAD_MALLOC RL_MALLOC
    #define GLAD_FREE RL_FREE

    #define GLAD_GL_IMPLEMENTATION
    #include "external/glad.h"          // GLAD extensions loading library, includes OpenGL headers
#endif

#if defined(GRAPHICS_API_OPENGL_ES3)
    #include <GLES3/gl3.h>              // OpenGL ES 3.0 library
    #define GL_GLEXT_PROTOTYPES
    #include <GLES2/gl2ext.h>           // OpenGL ES 2.0 extensions library
#elif defined(GRAPHICS_API_OPENGL_ES2)
    // NOTE: OpenGL ES 2.0 can be enabled on Desktop platforms,
    // in that case, functions are loaded from a custom glad for OpenGL ES 2.0
    // TODO: OpenGL ES 2.0 support shouldn't be platform-dependent, neither require GLAD
    #if defined(PLATFORM_DESKTOP_GLFW) || defined(PLATFORM_DESKTOP_SDL)
        #define GLAD_GLES2_IMPLEMENTATION
        #include "external/glad_gles2.h"
    #else
        #define GL_GLEXT_PROTOTYPES
        //#include <EGL/egl.h>          // EGL library -> not required, platform layer
        #include <GLES2/gl2.h>          // OpenGL ES 2.0 library
        #include <GLES2/gl2ext.h>       // OpenGL ES 2.0 extensions library
    #endif

    // It seems OpenGL ES 2.0 instancing entry points are not defined on Raspberry Pi
    // provided headers (despite being defined in official Khronos GLES2 headers)
    // TODO: Avoid raylib platform-dependent code on rlgl, it should be a completely portable library
    #if defined(PLATFORM_DRM)
    typedef void (GL_APIENTRYP PFNGLDRAWARRAYSINSTANCEDEXTPROC) (GLenum mode, GLint start, GLsizei count, GLsizei primcount);
    typedef void (GL_APIENTRYP PFNGLDRAWELEMENTSINSTANCEDEXTPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
    typedef void (GL_APIENTRYP PFNGLVERTEXATTRIBDIVISOREXTPROC) (GLuint index, GLuint divisor);
    #endif
#endif

#include <stdlib.h>                     // Required for: calloc(), free()
#include <string.h>                     // Required for: strcmp(), strlen() [Used in rlglInit(), on extensions loading]
#include <math.h>                       // Required for: sqrtf(), sinf(), cosf(), floor(), log()

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#ifndef PI
    #define PI 3.14159265358979323846f
#endif
#ifndef DEG2RAD
    #define DEG2RAD (PI/180.0f)
#endif
#ifndef RAD2DEG
    #define RAD2DEG (180.0f/PI)
#endif

#ifndef GL_SHADING_LANGUAGE_VERSION
    #define GL_SHADING_LANGUAGE_VERSION         0x8B8C
#endif

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
    #define GL_COMPRESSED_RGB_S3TC_DXT1_EXT     0x83F0
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT    0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT    0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT    0x83F3
#endif
#ifndef GL_ETC1_RGB8_OES
    #define GL_ETC1_RGB8_OES                    0x8D64
#endif
#ifndef GL_COMPRESSED_RGB8_ETC2
    #define GL_COMPRESSED_RGB8_ETC2             0x9274
#endif
#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
    #define GL_COMPRESSED_RGBA8_ETC2_EAC        0x9278
#endif
#ifndef GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG
    #define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG  0x8C00
#endif
#ifndef GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
    #define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#endif
#ifndef GL_COMPRESSED_RGBA_ASTC_4x4_KHR
    #define GL_COMPRESSED_RGBA_ASTC_4x4_KHR     0x93b0
#endif
#ifndef GL_COMPRESSED_RGBA_ASTC_8x8_KHR
    #define GL_COMPRESSED_RGBA_ASTC_8x8_KHR     0x93b7
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
    #define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT   0x84FF
#endif
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
    #define GL_TEXTURE_MAX_ANISOTROPY_EXT       0x84FE
#endif

#ifndef GL_PROGRAM_POINT_SIZE
    #define GL_PROGRAM_POINT_SIZE               0x8642
#endif

#ifndef GL_LINE_WIDTH
    #define GL_LINE_WIDTH                       0x0B21
#endif

#if defined(GRAPHICS_API_OPENGL_11)
    #define GL_UNSIGNED_SHORT_5_6_5             0x8363
    #define GL_UNSIGNED_SHORT_5_5_5_1           0x8034
    #define GL_UNSIGNED_SHORT_4_4_4_4           0x8033
#endif

#if defined(GRAPHICS_API_OPENGL_21)
    #define GL_LUMINANCE                        0x1909
    #define GL_LUMINANCE_ALPHA                  0x190A
#endif

#if defined(GRAPHICS_API_OPENGL_ES2)
    #define glClearDepth                 glClearDepthf
    #if !defined(GRAPHICS_API_OPENGL_ES3)
        #define GL_READ_FRAMEBUFFER         GL_FRAMEBUFFER
        #define GL_DRAW_FRAMEBUFFER         GL_FRAMEBUFFER
    #endif
#endif

// Default shader vertex attribute names to set location points
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_POSITION
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_POSITION     "vertexPosition"    // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD     "vertexTexCoord"    // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_NORMAL
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_NORMAL       "vertexNormal"      // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_COLOR
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_COLOR        "vertexColor"       // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_COLOR
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_TANGENT
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_TANGENT      "vertexTangent"     // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD2
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD2    "vertexTexCoord2"   // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_BONEINDICES
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_BONEINDICES  "vertexBoneIndices" // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEINDICES
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_BONEWEIGHTS
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_BONEWEIGHTS  "vertexBoneWeights" // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_BONEWEIGHTS
#endif

#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_INSTANCETRANSFORM
    #define RL_DEFAULT_SHADER_ATTRIB_NAME_INSTANCETRANSFORM "instanceTransform" // Bound by default to shader location: RL_DEFAULT_SHADER_ATTRIB_LOCATION_INSTANCETRANSFORM
#endif

#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_MVP
    #define RL_DEFAULT_SHADER_UNIFORM_NAME_MVP         "mvp"               // model-view-projection matrix
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_VIEW
    #define RL_DEFAULT_SHADER_UNIFORM_NAME_VIEW        "matView"           // view matrix
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_PROJECTION
    #define RL_DEFAULT_SHADER_UNIFORM_NAME_PROJECTION  "matProjection"     // projection matrix
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_MODEL
    #define RL_DEFAULT_SHADER_UNIFORM_NAME_MODEL       "matModel"          // model matrix
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_NORMAL
    #define RL_DEFAULT_SHADER_UNIFORM_NAME_NORMAL      "matNormal"         // normal matrix (transpose(inverse(matModelView))
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_COLOR
    #define RL_DEFAULT_SHADER_UNIFORM_NAME_COLOR       "colDiffuse"        // color diffuse (base tint color, multiplied by texture color)
#endif
#ifndef RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE0
    #define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE0  "texture0"          // texture0 (texture slot active 0)
#endif
#ifndef RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE1
    #define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE1  "texture1"          // texture1 (texture slot active 1)
#endif
#ifndef RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE2
    #define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE2  "texture2"          // texture2 (texture slot active 2)
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_BONEMATRICES
    #define RL_DEFAULT_SHADER_UNIFORM_NAME_BONEMATRICES "boneMatrices"     // bone matrices (required for GPU skinning)
#endif

//----------------------------------------------------------------------------------
// Module Types and Structures Definition
//----------------------------------------------------------------------------------
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)

typedef void *(*rlglLoadProc)(const char *name);   // OpenGL extension functions loader signature (same as GLADloadproc)

typedef struct rlglData {
    rlRenderBatch *currentBatch;            // Current render batch
    rlRenderBatch defaultBatch;             // Default internal render batch

    rlglLoadProc loader;                    // OpenGL function loader

    struct {
        int vertexCounter;                  // Current active render batch vertex counter (generic, used for all batches)
        float texcoordx, texcoordy;         // Current active texture coordinate (added on glVertex*())
        float normalx, normaly, normalz;    // Current active normal (added on glVertex*())
        unsigned char colorr, colorg, colorb, colora;   // Current active color (added on glVertex*())

        int currentMatrixMode;              // Current matrix mode
        Matrix *currentMatrix;              // Current matrix pointer
        Matrix modelview;                   // Default modelview matrix
        Matrix projection;                  // Default projection matrix
        Matrix transform;                   // Transform matrix to be used with rlTranslate, rlRotate, rlScale
        bool transformRequired;             // Require transform matrix application to current draw-call vertex (if required)
        Matrix stack[RL_MAX_MATRIX_STACK_SIZE];// Matrix stack for push/pop
        int stackCounter;                   // Matrix stack counter

        unsigned int currentTextureId;      // Current texture id to be used on glBegin
        unsigned int defaultTextureId;      // Default texture used on shapes/poly drawing (required by shader)
        unsigned int activeTextureId[RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS];    // Active texture ids to be enabled on batch drawing (0 active by default)
        unsigned int defaultVShaderId;      // Default vertex shader id (used by default shader program)
        unsigned int defaultFShaderId;      // Default fragment shader id (used by default shader program)
        unsigned int defaultShaderId;       // Default shader program id, supports vertex color and diffuse texture
        int *defaultShaderLocs;             // Default shader locations pointer to be used on rendering
        unsigned int currentShaderId;       // Current shader id to be used on rendering (by default, defaultShaderId)
        int *currentShaderLocs;             // Current shader locations pointer to be used on rendering (by default, defaultShaderLocs)

        bool stereoRender;                  // Stereo rendering flag
        Matrix projectionStereo[2];         // VR stereo rendering eyes projection matrices
        Matrix viewOffsetStereo[2];         // VR stereo rendering eyes view offset matrices

        // Blending variables
        int currentBlendMode;               // Blending mode active
        int glBlendSrcFactor;               // Blending source factor
        int glBlendDstFactor;               // Blending destination factor
        int glBlendEquation;                // Blending equation
        int glBlendSrcFactorRGB;            // Blending source RGB factor
        int glBlendDestFactorRGB;           // Blending destination RGB factor
        int glBlendSrcFactorAlpha;          // Blending source alpha factor
        int glBlendDestFactorAlpha;         // Blending destination alpha factor
        int glBlendEquationRGB;             // Blending equation for RGB
        int glBlendEquationAlpha;           // Blending equation for alpha
        bool glCustomBlendModeModified;     // Custom blending factor and equation modification status

        int framebufferWidth;               // Current framebuffer width
        int framebufferHeight;              // Current framebuffer height

    } State;            // Renderer state
    struct {
        bool vao;                           // VAO support (OpenGL ES2 could not support VAO extension) (GL_ARB_vertex_array_object)
        bool instancing;                    // Instancing supported (GL_ANGLE_instanced_arrays, GL_EXT_draw_instanced + GL_EXT_instanced_arrays)
        bool texNPOT;                       // NPOT textures full support (GL_ARB_texture_non_power_of_two, GL_OES_texture_npot)
        bool texDepth;                      // Depth textures supported (GL_ARB_depth_texture, GL_OES_depth_texture)
        bool texDepthWebGL;                 // Depth textures supported WebGL specific (GL_WEBGL_depth_texture)
        bool texFloat32;                    // float textures support (32 bit per channel) (GL_OES_texture_float)
        bool texFloat16;                    // half float textures support (16 bit per channel) (GL_OES_texture_half_float)
        bool texCompDXT;                    // DDS texture compression support (GL_EXT_texture_compression_s3tc, GL_WEBGL_compressed_texture_s3tc, GL_WEBKIT_WEBGL_compressed_texture_s3tc)
        bool texCompETC1;                   // ETC1 texture compression support (GL_OES_compressed_ETC1_RGB8_texture, GL_WEBGL_compressed_texture_etc1)
        bool texCompETC2;                   // ETC2/EAC texture compression support (GL_ARB_ES3_compatibility)
        bool texCompPVRT;                   // PVR texture compression support (GL_IMG_texture_compression_pvrtc)
        bool texCompASTC;                   // ASTC texture compression support (GL_KHR_texture_compression_astc_hdr, GL_KHR_texture_compression_astc_ldr)
        bool texMirrorClamp;                // Clamp mirror wrap mode supported (GL_EXT_texture_mirror_clamp)
        bool texAnisoFilter;                // Anisotropic texture filtering support (GL_EXT_texture_filter_anisotropic)
        bool computeShader;                 // Compute shaders support (GL_ARB_compute_shader)
        bool ssbo;                          // Shader storage buffer object support (GL_ARB_shader_storage_buffer_object)

        float maxAnisotropyLevel;           // Maximum anisotropy level supported (minimum is 2.0f)
        int maxDepthBits;                   // Maximum bits for depth component

    } ExtSupported;     // Extensions supported flags
} rlglData;

#endif // GRAPHICS_API_OPENGL_33 || GRAPHICS_API_OPENGL_ES2

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static bool isGpuReady = false;
static double rlCullDistanceNear = RL_CULL_DISTANCE_NEAR;
static double rlCullDistanceFar = RL_CULL_DISTANCE_FAR;

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
static rlglData RLGL = { 0 };
#endif

#if defined(GRAPHICS_API_OPENGL_ES2) && !defined(GRAPHICS_API_OPENGL_ES3)
// VAO functions entry points
// NOTE: VAO functionality is exposed through extensions (OES)
static PFNGLGENVERTEXARRAYSOESPROC glGenVertexArrays = NULL;
static PFNGLBINDVERTEXARRAYOESPROC glBindVertexArray = NULL;
static PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArrays = NULL;

// Instancing functionality entry points
// NOTE: Instancing functionality could be available through extensions
static PFNGLDRAWARRAYSINSTANCEDEXTPROC glDrawArraysInstanced = NULL;
static PFNGLDRAWELEMENTSINSTANCEDEXTPROC glDrawElementsInstanced = NULL;
static PFNGLVERTEXATTRIBDIVISOREXTPROC glVertexAttribDivisor = NULL;
#endif

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
static void rlLoadShaderDefault(void);      // Load default shader
static void rlUnloadShaderDefault(void);    // Unload default shader
#if RLGL_SHOW_GL_DETAILS_INFO
static const char *rlGetCompressedFormatName(int format); // Get compressed format official GL identifier name
#endif
#endif

static int rlGetPixelDataSize(int width, int height, int format);   // Get pixel data size in bytes (image or texture)

static Matrix rlMatrixIdentity(void);                       // Get identity matrix
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
// Auxiliar matrix math functions
typedef struct rl_float16 { float v[16]; } rl_float16;
static rl_float16 rlMatrixToFloatV(Matrix mat);             // Get float array of matrix data
#define rlMatrixToFloat(mat) (rlMatrixToFloatV(mat).v)      // Get float vector for Matrix
static Matrix rlMatrixMultiply(Matrix left, Matrix right);  // Multiply two matrices
static Matrix rlMatrixTranspose(Matrix mat);                // Transposes provided matrix
static Matrix rlMatrixInvert(Matrix mat);                   // Invert provided matrix
#endif

//----------------------------------------------------------------------------------
// Module Functions Definition - Matrix operations
//----------------------------------------------------------------------------------

#if defined(GRAPHICS_API_OPENGL_11)
// Fallback to OpenGL 1.1 function calls
//---------------------------------------
void rlMatrixMode(int mode)
{
    switch (mode)
    {
        case RL_PROJECTION: glMatrixMode(GL_PROJECTION); break;
        case RL_MODELVIEW: glMatrixMode(GL_MODELVIEW); break;
        case RL_TEXTURE: glMatrixMode(GL_TEXTURE); break;
        default: break;
    }
}

void rlFrustum(double left, double right, double bottom, double top, double znear, double zfar)
{
    glFrustum(left, right, bottom, top, znear, zfar);
}

void rlOrtho(double left, double right, double bottom, double top, double znear, double zfar)
{
    glOrtho(left, right, bottom, top, znear, zfar);
}

void rlPushMatrix(void) { glPushMatrix(); }
void rlPopMatrix(void) { glPopMatrix(); }
void rlLoadIdentity(void) { glLoadIdentity(); }
void rlTranslatef(float x, float y, float z) { glTranslatef(x, y, z); }
void rlRotatef(float angle, float x, float y, float z) { glRotatef(angle, x, y, z); }
void rlScalef(float x, float y, float z) { glScalef(x, y, z); }
void rlMultMatrixf(const float *matf) { glMultMatrixf(matf); }
#endif
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
// Choose the current matrix to be transformed
void rlMatrixMode(int mode)
{
    if (mode == RL_PROJECTION) RLGL.State.currentMatrix = &RLGL.State.projection;
    else if (mode == RL_MODELVIEW) RLGL.State.currentMatrix = &RLGL.State.modelview;
    //else if (mode == RL_TEXTURE) // Not supported

    RLGL.State.currentMatrixMode = mode;
}

// Push the current matrix into RLGL.State.stack
void rlPushMatrix(void)
{
    if (RLGL.State.stackCounter >= RL_MAX_MATRIX_STACK_SIZE) TRACELOG(RL_LOG_ERROR, "RLGL: Matrix stack overflow (RL_MAX_MATRIX_STACK_SIZE)");

    if (RLGL.State.currentMatrixMode == RL_MODELVIEW)
    {
        RLGL.State.transformRequired = true;
        RLGL.State.currentMatrix = &RLGL.State.transform;
    }

    RLGL.State.stack[RLGL.State.stackCounter] = *RLGL.State.currentMatrix;
    RLGL.State.stackCounter++;
}

// Pop latest inserted matrix from RLGL.State.stack
void rlPopMatrix(void)
{
    if (RLGL.State.stackCounter > 0)
    {
        Matrix mat = RLGL.State.stack[RLGL.State.stackCounter - 1];
        *RLGL.State.currentMatrix = mat;
        RLGL.State.stackCounter--;
    }

    if ((RLGL.State.stackCounter == 0) && (RLGL.State.currentMatrixMode == RL_MODELVIEW))
    {
        RLGL.State.currentMatrix = &RLGL.State.modelview;
        RLGL.State.transformRequired = false;
    }
}

// Reset current matrix to identity matrix
void rlLoadIdentity(void)
{
    *RLGL.State.currentMatrix = rlMatrixIdentity();
}

// Multiply the current matrix by a translation matrix
void rlTranslatef(float x, float y, float z)
{
    Matrix matTranslation = rlMatrixIdentity();

    // Set translation component of matrix
    matTranslation.m12 = x;
    matTranslation.m13 = y;
    matTranslation.m14 = z;

    // NOTE: Transposing matrix by multiplication order
    *RLGL.State.currentMatrix = rlMatrixMultiply(matTranslation, *RLGL.State.currentMatrix);
}

// Multiply the current matrix by a rotation matrix
// NOTE: The provided angle must be in degrees
void rlRotatef(float angle, float x, float y, float z)
{
    Matrix matRotation = rlMatrixIdentity();

    // Axis vector (x, y, z) normalization
    float lengthSquared = x*x + y*y + z*z;
    if ((lengthSquared != 1.0f) && (lengthSquared != 0.0f))
    {
        float inverseLength = 1.0f/sqrtf(lengthSquared);
        x *= inverseLength;
        y *= inverseLength;
        z *= inverseLength;
    }

    // Rotation matrix generation
    float sinres = sinf(DEG2RAD*angle);
    float cosres = cosf(DEG2RAD*angle);
    float t = 1.0f - cosres;

    matRotation.m0 = x*x*t + cosres;
    matRotation.m1 = y*x*t + z*sinres;
    matRotation.m2 = z*x*t - y*sinres;
    matRotation.m3 = 0.0f;

    matRotation.m4 = x*y*t - z*sinres;
    matRotation.m5 = y*y*t + cosres;
    matRotation.m6 = z*y*t + x*sinres;
    matRotation.m7 = 0.0f;

    matRotation.m8 = x*z*t + y*sinres;
    matRotation.m9 = y*z*t - x*sinres;
    matRotation.m10 = z*z*t + cosres;
    matRotation.m11 = 0.0f;

    matRotation.m12 = 0.0f;
    matRotation.m13 = 0.0f;
    matRotation.m14 = 0.0f;
    matRotation.m15 = 1.0f;

    // NOTE: Transposing matrix by multiplication order
    *RLGL.State.currentMatrix = rlMatrixMultiply(matRotation, *RLGL.State.currentMatrix);
}

// Multiply the current matrix by a scaling matrix
void rlScalef(float x, float y, float z)
{
    Matrix matScale = rlMatrixIdentity();

    // Set scale component of matrix
    matScale.m0 = x;
    matScale.m5 = y;
    matScale.m10 = z;

    // NOTE: Transposing matrix by multiplication order
    *RLGL.State.currentMatrix = rlMatrixMultiply(matScale, *RLGL.State.currentMatrix);
}

// Multiply the current matrix by another matrix
void rlMultMatrixf(const float *matf)
{
    // Matrix creation from array
    // Conversion from column-major to row-major memory order
    Matrix mat = { matf[0], matf[4], matf[8], matf[12],
                   matf[1], matf[5], matf[9], matf[13],
                   matf[2], matf[6], matf[10], matf[14],
                   matf[3], matf[7], matf[11], matf[15] };

    *RLGL.State.currentMatrix = rlMatrixMultiply(mat, *RLGL.State.currentMatrix);
}

// Multiply the current matrix by a perspective matrix generated by parameters
void rlFrustum(double left, double right, double bottom, double top, double znear, double zfar)
{
    Matrix matFrustum = { 0 };

    float rl = (float)(right - left);
    float tb = (float)(top - bottom);
    float fn = (float)(zfar - znear);

    matFrustum.m0 = ((float) znear*2.0f)/rl;
    matFrustum.m1 = 0.0f;
    matFrustum.m2 = 0.0f;
    matFrustum.m3 = 0.0f;

    matFrustum.m4 = 0.0f;
    matFrustum.m5 = ((float) znear*2.0f)/tb;
    matFrustum.m6 = 0.0f;
    matFrustum.m7 = 0.0f;

    matFrustum.m8 = ((float)right + (float)left)/rl;
    matFrustum.m9 = ((float)top + (float)bottom)/tb;
    matFrustum.m10 = -((float)zfar + (float)znear)/fn;
    matFrustum.m11 = -1.0f;

    matFrustum.m12 = 0.0f;
    matFrustum.m13 = 0.0f;
    matFrustum.m14 = -((float)zfar*(float)znear*2.0f)/fn;
    matFrustum.m15 = 0.0f;

    *RLGL.State.currentMatrix = rlMatrixMultiply(*RLGL.State.currentMatrix, matFrustum);
}

// Multiply the current matrix by an orthographic matrix generated by parameters
void rlOrtho(double left, double right, double bottom, double top, double znear, double zfar)
{
    // NOTE: If left-right and top-botton values are equal it could create a division by zero,
    // response to it is platform/compiler dependent
    Matrix matOrtho = { 0 };

    float rl = (float)(right - left);
    float tb = (float)(top - bottom);
    float fn = (float)(zfar - znear);

    matOrtho.m0 = 2.0f/rl;
    matOrtho.m1 = 0.0f;
    matOrtho.m2 = 0.0f;
    matOrtho.m3 = 0.0f;
    matOrtho.m4 = 0.0f;
    matOrtho.m5 = 2.0f/tb;
    matOrtho.m6 = 0.0f;
    matOrtho.m7 = 0.0f;
    matOrtho.m8 = 0.0f;
    matOrtho.m9 = 0.0f;
    matOrtho.m10 = -2.0f/fn;
    matOrtho.m11 = 0.0f;
    matOrtho.m12 = -((float)left + (float)right)/rl;
    matOrtho.m13 = -((float)top + (float)bottom)/tb;
    matOrtho.m14 = -((float)zfar + (float)znear)/fn;
    matOrtho.m15 = 1.0f;

    *RLGL.State.currentMatrix = rlMatrixMultiply(*RLGL.State.currentMatrix, matOrtho);
}
#endif

// Set the viewport area (transformation from normalized device coordinates to window coordinates)
void rlViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

// Set clip planes distances
void rlSetClipPlanes(double nearPlane, double farPlane)
{
    rlCullDistanceNear = nearPlane;
    rlCullDistanceFar = farPlane;
}

// Get cull plane distance near
double rlGetCullDistanceNear(void)
{
    return rlCullDistanceNear;
}

// Get cull plane distance far
double rlGetCullDistanceFar(void)
{
    return rlCullDistanceFar;
}

//----------------------------------------------------------------------------------
// Module Functions Definition - Vertex level operations
//----------------------------------------------------------------------------------
#if defined(GRAPHICS_API_OPENGL_11)
// Fallback to OpenGL 1.1 function calls
//---------------------------------------
void rlBegin(int mode)
{
    switch (mode)
    {
        case RL_LINES: glBegin(GL_LINES); break;
        case RL_TRIANGLES: glBegin(GL_TRIANGLES); break;
        case RL_QUADS: glBegin(GL_QUADS); break;
        default: break;
    }
}

void rlEnd(void) { glEnd(); }
void rlVertex2i(int x, int y) { glVertex2i(x, y); }
void rlVertex2f(float x, float y) { glVertex2f(x, y); }
void rlVertex3f(float x, float y, float z) { glVertex3f(x, y, z); }
void rlTexCoord2f(float x, float y) { glTexCoord2f(x, y); }
void rlNormal3f(float x, float y, float z) { glNormal3f(x, y, z); }
void rlColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a) { glColor4ub(r, g, b, a); }
void rlColor3f(float x, float y, float z) { glColor3f(x, y, z); }
void rlColor4f(float x, float y, float z, float w) { glColor4f(x, y, z, w); }
#endif
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
// Initialize drawing mode (how to organize vertex)
void rlBegin(int mode)
{
    // Draw mode can be RL_LINES, RL_TRIANGLES and RL_QUADS
    // NOTE: In all three cases, vertex are accumulated over default internal vertex buffer
    if (RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].mode != mode)
    {
        if (RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount > 0)
        {
            // Make sure current RLGL.currentBatch->draws[i].vertexCount is aligned a multiple of 4,
            // that way, following QUADS drawing will keep aligned with index processing
            // It implies adding some extra alignment vertex at the end of the draw,
            // those vertex are not processed but they are considered as an additional offset
            // for the next set of vertex to be drawn
            if (RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].mode == RL_LINES) RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexAlignment = ((RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount < 4)? RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount : RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount%4);
            else if (RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].mode == RL_TRIANGLES) RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexAlignment = ((RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount < 4)? 1 : (4 - (RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount%4)));
            else RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexAlignment = 0;

            if (!rlCheckRenderBatchLimit(RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexAlignment))
            {
                RLGL.State.vertexCounter += RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexAlignment;
                RLGL.currentBatch->drawCounter++;
            }
        }

        if (RLGL.currentBatch->drawCounter >= RL_DEFAULT_BATCH_DRAWCALLS) rlDrawRenderBatch(RLGL.currentBatch);

        RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].mode = mode;
        RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].textureId = RLGL.State.currentTextureId;
        RLGL.State.currentTextureId = RLGL.State.defaultTextureId;
    }
}

// Finish vertex providing
void rlEnd(void)
{
    // NOTE: Depth increment is dependent on rlOrtho(): z-near and z-far values,
    // as well as depth buffer bit-depth (16bit or 24bit or 32bit)
    // Correct increment formula would be: depthInc = (zfar - znear)/pow(2, bits)
    RLGL.currentBatch->currentDepth += (1.0f/20000.0f);
}

// Define one vertex (position)
// NOTE: Vertex position data is the basic information required for drawing
void rlVertex3f(float x, float y, float z)
{
    float tx = x;
    float ty = y;
    float tz = z;

    // Transform provided vector if required
    if (RLGL.State.transformRequired)
    {
        tx = RLGL.State.transform.m0*x + RLGL.State.transform.m4*y + RLGL.State.transform.m8*z + RLGL.State.transform.m12;
        ty = RLGL.State.transform.m1*x + RLGL.State.transform.m5*y + RLGL.State.transform.m9*z + RLGL.State.transform.m13;
        tz = RLGL.State.transform.m2*x + RLGL.State.transform.m6*y + RLGL.State.transform.m10*z + RLGL.State.transform.m14;
    }

    // WARNING: Be careful with primitives breaking when launching a new batch!
    // RL_LINES comes in pairs, RL_TRIANGLES come in groups of 3 vertices and RL_QUADS come in groups of 4 vertices
    // Checking current draw.mode when a new vertex is required and finish the batch only if the draw.mode draw.vertexCount is %2, %3 or %4
    if (RLGL.State.vertexCounter > (RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].elementCount*4 - 4))
    {
        if ((RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].mode == RL_LINES) &&
            (RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount%2 == 0))
        {
            // Reached the maximum number of vertices for RL_LINES drawing
            // Launch a draw call but keep current state for next vertices comming
            // NOTE: Adding +1 vertex to the check for some safety
            rlCheckRenderBatchLimit(2 + 1);
        }
        else if ((RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].mode == RL_TRIANGLES) &&
            (RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount%3 == 0))
        {
            rlCheckRenderBatchLimit(3 + 1);
        }
        else if ((RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].mode == RL_QUADS) &&
            (RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount%4 == 0))
        {
            rlCheckRenderBatchLimit(4 + 1);
        }
    }

    // Add vertices
    RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].vertices[3*RLGL.State.vertexCounter] = tx;
    RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].vertices[3*RLGL.State.vertexCounter + 1] = ty;
    RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].vertices[3*RLGL.State.vertexCounter + 2] = tz;

    // Add current texcoord
    RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].texcoords[2*RLGL.State.vertexCounter] = RLGL.State.texcoordx;
    RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].texcoords[2*RLGL.State.vertexCounter + 1] = RLGL.State.texcoordy;

    // Add current normal
    RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].normals[3*RLGL.State.vertexCounter] = RLGL.State.normalx;
    RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].normals[3*RLGL.State.vertexCounter + 1] = RLGL.State.normaly;
    RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].normals[3*RLGL.State.vertexCounter + 2] = RLGL.State.normalz;

    // Add current color
    RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].colors[4*RLGL.State.vertexCounter] = RLGL.State.colorr;
    RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].colors[4*RLGL.State.vertexCounter + 1] = RLGL.State.colorg;
    RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].colors[4*RLGL.State.vertexCounter + 2] = RLGL.State.colorb;
    RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].colors[4*RLGL.State.vertexCounter + 3] = RLGL.State.colora;

    RLGL.State.vertexCounter++;
    RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount++;
}

// Define one vertex (position)
void rlVertex2f(float x, float y)
{
    rlVertex3f(x, y, RLGL.currentBatch->currentDepth);
}

// Define one vertex (position)
void rlVertex2i(int x, int y)
{
    rlVertex3f((float)x, (float)y, RLGL.currentBatch->currentDepth);
}

// Define one vertex (texture coordinate)
// NOTE: Texture coordinates are limited to QUADS only
void rlTexCoord2f(float x, float y)
{
    RLGL.State.texcoordx = x;
    RLGL.State.texcoordy = y;
}

// Define one vertex (normal)
// NOTE: Normals limited to TRIANGLES only?
void rlNormal3f(float x, float y, float z)
{
    float normalx = x;
    float normaly = y;
    float normalz = z;
    if (RLGL.State.transformRequired)
    {
        normalx = RLGL.State.transform.m0*x + RLGL.State.transform.m4*y + RLGL.State.transform.m8*z;
        normaly = RLGL.State.transform.m1*x + RLGL.State.transform.m5*y + RLGL.State.transform.m9*z;
        normalz = RLGL.State.transform.m2*x + RLGL.State.transform.m6*y + RLGL.State.transform.m10*z;
    }

    // NOTE: Default behavior assumes the normal vector is in the correct space for what the shader expects,
    // it could be not normalized to 0.0f..1.0f, magnitud can be useed for some effects
    /*
    // WARNING: Vector normalization if required
    float length = sqrtf(normalx*normalx + normaly*normaly + normalz*normalz);
    if (length != 0.0f)
    {
        float ilength = 1.0f/length;
        normalx *= ilength;
        normaly *= ilength;
        normalz *= ilength;
    }
    */
    RLGL.State.normalx = normalx;
    RLGL.State.normaly = normaly;
    RLGL.State.normalz = normalz;
}

// Define one vertex (color)
void rlColor4ub(unsigned char x, unsigned char y, unsigned char z, unsigned char w)
{
    RLGL.State.colorr = x;
    RLGL.State.colorg = y;
    RLGL.State.colorb = z;
    RLGL.State.colora = w;
}

// Define one vertex (color)
void rlColor4f(float r, float g, float b, float a)
{
    rlColor4ub((unsigned char)(r*255), (unsigned char)(g*255), (unsigned char)(b*255), (unsigned char)(a*255));
}

// Define one vertex (color)
void rlColor3f(float x, float y, float z)
{
    rlColor4ub((unsigned char)(x*255), (unsigned char)(y*255), (unsigned char)(z*255), 255);
}

#endif

//--------------------------------------------------------------------------------------
// Module Functions Definition - OpenGL style functions (common to 1.1, 3.3+, ES2)
//--------------------------------------------------------------------------------------

// Set current texture to use
void rlSetTexture(unsigned int id)
{
    if (id == 0)
    {
#if defined(GRAPHICS_API_OPENGL_11)
        rlDisableTexture();
#else
        // NOTE: If quads batch limit is reached, force a draw call and next batch starts
        if (RLGL.State.vertexCounter >=
            RLGL.currentBatch->vertexBuffer[RLGL.currentBatch->currentBuffer].elementCount*4)
        {
            rlDrawRenderBatch(RLGL.currentBatch);
        }
        RLGL.State.currentTextureId = RLGL.State.defaultTextureId;
#endif
    }
    else
    {
#if defined(GRAPHICS_API_OPENGL_11)
        rlEnableTexture(id);
#else
        RLGL.State.currentTextureId = id;
        if (RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].textureId != id)
        {
            if (RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount > 0)
            {
                // Make sure current RLGL.currentBatch->draws[i].vertexCount is aligned a multiple of 4,
                // that way, following QUADS drawing will keep aligned with index processing
                // It implies adding some extra alignment vertex at the end of the draw,
                // those vertex are not processed but they are considered as an additional offset
                // for the next set of vertex to be drawn
                if (RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].mode == RL_LINES) RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexAlignment = ((RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount < 4)? RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount : RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount%4);
                else if (RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].mode == RL_TRIANGLES) RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexAlignment = ((RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount < 4)? 1 : (4 - (RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount%4)));
                else RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexAlignment = 0;

                if (!rlCheckRenderBatchLimit(RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexAlignment))
                {
                    RLGL.State.vertexCounter += RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexAlignment;

                    RLGL.currentBatch->drawCounter++;

                    RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].mode = RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 2].mode;

                }
            }

            if (RLGL.currentBatch->drawCounter >= RL_DEFAULT_BATCH_DRAWCALLS) rlDrawRenderBatch(RLGL.currentBatch);

            RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].textureId = id;
            RLGL.currentBatch->draws[RLGL.currentBatch->drawCounter - 1].vertexCount = 0;
        }
#endif
    }
}

// Select and active a texture slot
void rlActiveTextureSlot(int slot)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glActiveTexture(GL_TEXTURE0 + slot);
#endif
}

// Enable texture
void rlEnableTexture(unsigned int id)
{
#if defined(GRAPHICS_API_OPENGL_11)
    glEnable(GL_TEXTURE_2D);
#endif
    glBindTexture(GL_TEXTURE_2D, id);
}

// Disable texture
void rlDisableTexture(void)
{
#if defined(GRAPHICS_API_OPENGL_11)
    glDisable(GL_TEXTURE_2D);
#endif
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Enable texture cubemap
void rlEnableTextureCubemap(unsigned int id)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
#endif
}

// Disable texture cubemap
void rlDisableTextureCubemap(void)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
#endif
}

// Set texture parameters (wrap mode/filter mode)
void rlTextureParameters(unsigned int id, int param, int value)
{
    glBindTexture(GL_TEXTURE_2D, id);

    switch (param)
    {
        case RL_TEXTURE_WRAP_S:
        case RL_TEXTURE_WRAP_T:
        {
            if (value == RL_TEXTURE_WRAP_MIRROR_CLAMP)
            {
#if !defined(GRAPHICS_API_OPENGL_11)
                if (RLGL.ExtSupported.texMirrorClamp) glTexParameteri(GL_TEXTURE_2D, param, value);
                else TRACELOG(RL_LOG_WARNING, "GL: Clamp mirror wrap mode not supported (GL_MIRROR_CLAMP_EXT)");
#endif
            }
            else glTexParameteri(GL_TEXTURE_2D, param, value);
        } break;
        case RL_TEXTURE_MAG_FILTER:
        case RL_TEXTURE_MIN_FILTER: glTexParameteri(GL_TEXTURE_2D, param, value); break;
        case RL_TEXTURE_FILTER_ANISOTROPIC:
        {
#if !defined(GRAPHICS_API_OPENGL_11)
            // Reset anisotropy filter, in case it was set
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);

            if (value <= RLGL.ExtSupported.maxAnisotropyLevel) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)value);
            else if (RLGL.ExtSupported.maxAnisotropyLevel > 0.0f)
            {
                TRACELOG(RL_LOG_WARNING, "GL: Maximum anisotropic filter level supported is %iX", id, (int)RLGL.ExtSupported.maxAnisotropyLevel);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)value);
            }
            else TRACELOG(RL_LOG_WARNING, "GL: Anisotropic filtering not supported");
#endif
        } break;
#if defined(GRAPHICS_API_OPENGL_33)
        case RL_TEXTURE_MIPMAP_BIAS_RATIO: glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, value/100.0f);
#endif
        default: break;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

// Set cubemap parameters (wrap mode/filter mode)
void rlCubemapParameters(unsigned int id, int param, int value)
{
#if !defined(GRAPHICS_API_OPENGL_11)
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);

    // Reset anisotropy filter, in case it was set
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);

    switch (param)
    {
        case RL_TEXTURE_WRAP_S:
        case RL_TEXTURE_WRAP_T:
        {
            if (value == RL_TEXTURE_WRAP_MIRROR_CLAMP)
            {
                if (RLGL.ExtSupported.texMirrorClamp) glTexParameteri(GL_TEXTURE_CUBE_MAP, param, value);
                else TRACELOG(RL_LOG_WARNING, "GL: Clamp mirror wrap mode not supported (GL_MIRROR_CLAMP_EXT)");
            }
            else glTexParameteri(GL_TEXTURE_CUBE_MAP, param, value);
        } break;
        case RL_TEXTURE_MAG_FILTER:
        case RL_TEXTURE_MIN_FILTER: glTexParameteri(GL_TEXTURE_CUBE_MAP, param, value); break;
        case RL_TEXTURE_FILTER_ANISOTROPIC:
        {
            if (value <= RLGL.ExtSupported.maxAnisotropyLevel) glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)value);
            else if (RLGL.ExtSupported.maxAnisotropyLevel > 0.0f)
            {
                TRACELOG(RL_LOG_WARNING, "GL: Maximum anisotropic filter level supported is %iX", id, (int)RLGL.ExtSupported.maxAnisotropyLevel);
                glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)value);
            }
            else TRACELOG(RL_LOG_WARNING, "GL: Anisotropic filtering not supported");
        } break;
#if defined(GRAPHICS_API_OPENGL_33)
        case RL_TEXTURE_MIPMAP_BIAS_RATIO: glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_LOD_BIAS, value/100.0f);
#endif
        default: break;
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
#endif
}

// Enable shader program
void rlEnableShader(unsigned int id)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2))
    glUseProgram(id);
#endif
}

// Disable shader program
void rlDisableShader(void)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2))
    glUseProgram(0);
#endif
}

// Enable rendering to texture (fbo)
void rlEnableFramebuffer(unsigned int id)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2) || defined(GRAPHICS_API_OPENGL_SOFTWARE))
    glBindFramebuffer(GL_FRAMEBUFFER, id);
#endif
}

// return the active render texture (fbo)
unsigned int rlGetActiveFramebuffer(void)
{
    GLint fboId = 0;
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES3) || defined(GRAPHICS_API_OPENGL_SOFTWARE))
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fboId);
#endif
    return fboId;
}

// Disable rendering to texture
void rlDisableFramebuffer(void)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2) || defined(GRAPHICS_API_OPENGL_SOFTWARE))
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

// Blit active framebuffer to main framebuffer
void rlBlitFramebuffer(int srcX, int srcY, int srcWidth, int srcHeight, int dstX, int dstY, int dstWidth, int dstHeight, int bufferMask)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES3))
    glBlitFramebuffer(srcX, srcY, srcWidth, srcHeight, dstX, dstY, dstWidth, dstHeight, bufferMask, GL_NEAREST);
#endif
}

// Bind framebuffer object (fbo)
void rlBindFramebuffer(unsigned int target, unsigned int framebuffer)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2) || defined(GRAPHICS_API_OPENGL_SOFTWARE))
    glBindFramebuffer(target, framebuffer);
#endif
}

// Activate multiple draw color buffers
// NOTE: One color buffer is always active by default
void rlActiveDrawBuffers(int count)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES3))
    // NOTE: Maximum number of draw buffers supported is implementation dependent,
    // it can be queried with glGet*() but it must be at least 8
    //GLint maxDrawBuffers = 0;
    //glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

    if (count > 0)
    {
        if (count > 8) TRACELOG(RL_LOG_WARNING, "GL: Max color buffers limited to 8");
        else
        {
            unsigned int buffers[8] = {
                GL_COLOR_ATTACHMENT0,
                GL_COLOR_ATTACHMENT1,
                GL_COLOR_ATTACHMENT2,
                GL_COLOR_ATTACHMENT3,
                GL_COLOR_ATTACHMENT4,
                GL_COLOR_ATTACHMENT5,
                GL_COLOR_ATTACHMENT6,
                GL_COLOR_ATTACHMENT7,
            };

            glDrawBuffers(count, buffers);
        }
    }
    else TRACELOG(RL_LOG_WARNING, "GL: One color buffer active by default");
#endif
}

//----------------------------------------------------------------------------------
// General render state configuration
//----------------------------------------------------------------------------------

// Enable color blending
void rlEnableColorBlend(void) { glEnable(GL_BLEND); }

// Disable color blending
void rlDisableColorBlend(void) { glDisable(GL_BLEND); }

// Enable depth test
void rlEnableDepthTest(void) { glEnable(GL_DEPTH_TEST); }

// Disable depth test
void rlDisableDepthTest(void) { glDisable(GL_DEPTH_TEST); }

// Enable depth write
void rlEnableDepthMask(void) { glDepthMask(GL_TRUE); }

// Disable depth write
void rlDisableDepthMask(void) { glDepthMask(GL_FALSE); }

// Enable backface culling
void rlEnableBackfaceCulling(void) { glEnable(GL_CULL_FACE); }

// Disable backface culling
void rlDisableBackfaceCulling(void) { glDisable(GL_CULL_FACE); }

// Set color mask active for screen read/draw
void rlColorMask(bool r, bool g, bool b, bool a) { glColorMask(r, g, b, a); }

// Set face culling mode
void rlSetCullFace(int mode)
{
    switch (mode)
    {
        case RL_CULL_FACE_BACK: glCullFace(GL_BACK); break;
        case RL_CULL_FACE_FRONT: glCullFace(GL_FRONT); break;
        default: break;
    }
}

// Enable scissor test
void rlEnableScissorTest(void) { glEnable(GL_SCISSOR_TEST); }

// Disable scissor test
void rlDisableScissorTest(void) { glDisable(GL_SCISSOR_TEST); }

// Scissor test
void rlScissor(int x, int y, int width, int height) { glScissor(x, y, width, height); }

// Enable wire mode
void rlEnableWireMode(void)
{
#if defined(GRAPHICS_API_OPENGL_11) || defined(GRAPHICS_API_OPENGL_33)
    // NOTE: glPolygonMode() not available on OpenGL ES
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
}

// Disable wire mode
void rlDisableWireMode(void)
{
#if defined(GRAPHICS_API_OPENGL_11) || defined(GRAPHICS_API_OPENGL_33)
    // NOTE: glPolygonMode() not available on OpenGL ES
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}

// Enable point mode
void rlEnablePointMode(void)
{
#if defined(GRAPHICS_API_OPENGL_11) || defined(GRAPHICS_API_OPENGL_33)
    // NOTE: glPolygonMode() not available on OpenGL ES
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glEnable(GL_PROGRAM_POINT_SIZE);
#endif
}

// Disable point mode
void rlDisablePointMode(void)
{
#if defined(GRAPHICS_API_OPENGL_11) || defined(GRAPHICS_API_OPENGL_33)
    // NOTE: glPolygonMode() not available on OpenGL ES
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}

// Set the line drawing width
void rlSetLineWidth(float width) { glLineWidth(width); }

// Get the line drawing width
float rlGetLineWidth(void)
{
    float width = 0;
    glGetFloatv(GL_LINE_WIDTH, &width);
    return width;
}

// Set the point drawing size
void rlSetPointSize(float size)
{
#if defined(GRAPHICS_API_OPENGL_11)
    glPointSize(size);
#endif
}

// Get the point drawing size
float rlGetPointSize(void)
{
    float size = 1;
#if defined(GRAPHICS_API_OPENGL_11)
    glGetFloatv(GL_POINT_SIZE, &size);
#endif
    return size;

}

// Enable line aliasing
void rlEnableSmoothLines(void)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_11)
    glEnable(GL_LINE_SMOOTH);
#endif
}

// Disable line aliasing
void rlDisableSmoothLines(void)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_11)
    glDisable(GL_LINE_SMOOTH);
#endif
}

// Enable stereo rendering
void rlEnableStereoRender(void)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2))
    RLGL.State.stereoRender = true;
#endif
}

// Disable stereo rendering
void rlDisableStereoRender(void)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2))
    RLGL.State.stereoRender = false;
#endif
}

// Check if stereo render is enabled
bool rlIsStereoRenderEnabled(void)
{
#if (defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2))
    return RLGL.State.stereoRender;
#else
    return false;
#endif
}

// Clear color buffer with color
void rlClearColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    // Color values clamp to 0.0f(0) and 1.0f(255)
    float cr = (float)r/255;
    float cg = (float)g/255;
    float cb = (float)b/255;
    float ca = (float)a/255;

    glClearColor(cr, cg, cb, ca);
}

// Clear used screen buffers (color and depth)
void rlClearScreenBuffers(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // Clear used buffers: Color and Depth (Depth is used for 3D)
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);     // Stencil buffer not used...
}

// Check and log OpenGL error codes
void rlCheckErrors(void)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    int check = 1;
    while (check)
    {
        const GLenum err = glGetError();
        switch (err)
        {
            case GL_NO_ERROR: check = 0; break;
            case 0x0500: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_INVALID_ENUM"); break;
            case 0x0501: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_INVALID_VALUE"); break;
            case 0x0502: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_INVALID_OPERATION"); break;
            case 0x0503: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_STACK_OVERFLOW"); break;
            case 0x0504: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_STACK_UNDERFLOW"); break;
            case 0x0505: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_OUT_OF_MEMORY"); break;
            case 0x0506: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_INVALID_FRAMEBUFFER_OPERATION"); break;
            default: TRACELOG(RL_LOG_WARNING, "GL: Error detected: Unknown error code: %x", err); break;
        }
    }
#endif
}

// Set blend mode
void rlSetBlendMode(int mode)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    if ((RLGL.State.currentBlendMode != mode) || ((mode == RL_BLEND_CUSTOM || mode == RL_BLEND_CUSTOM_SEPARATE) && RLGL.State.glCustomBlendModeModified))
    {
        rlDrawRenderBatch(RLGL.currentBatch);

        switch (mode)
        {
            case RL_BLEND_ALPHA: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glBlendEquation(GL_FUNC_ADD); break;
            case RL_BLEND_ADDITIVE: glBlendFunc(GL_SRC_ALPHA, GL_ONE); glBlendEquation(GL_FUNC_ADD); break;
            case RL_BLEND_MULTIPLIED: glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA); glBlendEquation(GL_FUNC_ADD); break;
            case RL_BLEND_ADD_COLORS: glBlendFunc(GL_ONE, GL_ONE); glBlendEquation(GL_FUNC_ADD); break;
            case RL_BLEND_SUBTRACT_COLORS: glBlendFunc(GL_ONE, GL_ONE); glBlendEquation(GL_FUNC_SUBTRACT); break;
            case RL_BLEND_ALPHA_PREMULTIPLY: glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); glBlendEquation(GL_FUNC_ADD); break;
            case RL_BLEND_CUSTOM:
            {
                // NOTE: Using GL blend src/dst factors and GL equation configured with rlSetBlendFactors()
                glBlendFunc(RLGL.State.glBlendSrcFactor, RLGL.State.glBlendDstFactor); glBlendEquation(RLGL.State.glBlendEquation);
            } break;
            case RL_BLEND_CUSTOM_SEPARATE:
            {
                // NOTE: Using GL blend src/dst factors and GL equation configured with rlSetBlendFactorsSeparate()
                glBlendFuncSeparate(RLGL.State.glBlendSrcFactorRGB, RLGL.State.glBlendDestFactorRGB, RLGL.State.glBlendSrcFactorAlpha, RLGL.State.glBlendDestFactorAlpha);
                glBlendEquationSeparate(RLGL.State.glBlendEquationRGB, RLGL.State.glBlendEquationAlpha);
            } break;
            default: break;
        }

        RLGL.State.currentBlendMode = mode;
        RLGL.State.glCustomBlendModeModified = false;
    }
#endif
}

// Set blending mode factor and equation
void rlSetBlendFactors(int glSrcFactor, int glDstFactor, int glEquation)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    if ((RLGL.State.glBlendSrcFactor != glSrcFactor) ||
        (RLGL.State.glBlendDstFactor != glDstFactor) ||
        (RLGL.State.glBlendEquation != glEquation))
    {
        RLGL.State.glBlendSrcFactor = glSrcFactor;
        RLGL.State.glBlendDstFactor = glDstFactor;
        RLGL.State.glBlendEquation = glEquation;

        RLGL.State.glCustomBlendModeModified = true;
    }
#endif
}

// Set blending mode factor and equation separately for RGB and alpha
void rlSetBlendFactorsSeparate(int glSrcRGB, int glDstRGB, int glSrcAlpha, int glDstAlpha, int glEqRGB, int glEqAlpha)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    if ((RLGL.State.glBlendSrcFactorRGB != glSrcRGB) ||
        (RLGL.State.glBlendDestFactorRGB != glDstRGB) ||
        (RLGL.State.glBlendSrcFactorAlpha != glSrcAlpha) ||
        (RLGL.State.glBlendDestFactorAlpha != glDstAlpha) ||
        (RLGL.State.glBlendEquationRGB != glEqRGB) ||
        (RLGL.State.glBlendEquationAlpha != glEqAlpha))
    {
        RLGL.State.glBlendSrcFactorRGB = glSrcRGB;
        RLGL.State.glBlendDestFactorRGB = glDstRGB;
        RLGL.State.glBlendSrcFactorAlpha = glSrcAlpha;
        RLGL.State.glBlendDestFactorAlpha = glDstAlpha;
        RLGL.State.glBlendEquationRGB = glEqRGB;
        RLGL.State.glBlendEquationAlpha = glEqAlpha;

        RLGL.State.glCustomBlendModeModified = true;
    }
#endif
}

//----------------------------------------------------------------------------------
// Module Functions Definition - OpenGL Debug
//----------------------------------------------------------------------------------
#if defined(GRAPHICS_API_OPENGL_43) && RLGL_ENABLE_OPENGL_DEBUG_CONTEXT
static void GLAPIENTRY rlDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    // Ignore non-significant error/warning codes (NVidia drivers)
    // NOTE: Here there are the details with a sample output:
    // - #131169 - Framebuffer detailed info: The driver allocated storage for renderbuffer 2. (severity: low)
    // - #131185 - Buffer detailed info: Buffer object 1 (bound to GL_ELEMENT_ARRAY_BUFFER_ARB, usage hint is GL_ENUM_88e4)
    //             will use VIDEO memory as the source for buffer object operations. (severity: low)
    // - #131218 - Program/shader state performance warning: Vertex shader in program 7 is being recompiled based on GL state. (severity: medium)
    // - #131204 - Texture state usage warning: The texture object (0) bound to texture image unit 0 does not have
    //             a defined base level and cannot be used for texture mapping. (severity: low)
    if ((id == 131169) || (id == 131185) || (id == 131218) || (id == 131204)) return;

    const char *msgSource = NULL;
    switch (source)
    {
        case GL_DEBUG_SOURCE_API: msgSource = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: msgSource = "WINDOW_SYSTEM"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: msgSource = "SHADER_COMPILER"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: msgSource = "THIRD_PARTY"; break;
        case GL_DEBUG_SOURCE_APPLICATION: msgSource = "APPLICATION"; break;
        case GL_DEBUG_SOURCE_OTHER: msgSource = "OTHER"; break;
        default: break;
    }

    const char *msgType = NULL;
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR: msgType = "ERROR"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: msgType = "DEPRECATED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: msgType = "UNDEFINED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_PORTABILITY: msgType = "PORTABILITY"; break;
        case GL_DEBUG_TYPE_PERFORMANCE: msgType = "PERFORMANCE"; break;
        case GL_DEBUG_TYPE_MARKER: msgType = "MARKER"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP: msgType = "PUSH_GROUP"; break;
        case GL_DEBUG_TYPE_POP_GROUP: msgType = "POP_GROUP"; break;
        case GL_DEBUG_TYPE_OTHER: msgType = "OTHER"; break;
        default: break;
    }

    const char *msgSeverity = "DEFAULT";
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_LOW: msgSeverity = "LOW"; break;
        case GL_DEBUG_SEVERITY_MEDIUM: msgSeverity = "MEDIUM"; break;
        case GL_DEBUG_SEVERITY_HIGH: msgSeverity = "HIGH"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: msgSeverity = "NOTIFICATION"; break;
        default: break;
    }

    TRACELOG(RL_LOG_WARNING, "GL: OpenGL debug message: %s", message);
    TRACELOG(RL_LOG_WARNING, "    > Type: %s", msgType);
    TRACELOG(RL_LOG_WARNING, "    > Source = %s", msgSource);
    TRACELOG(RL_LOG_WARNING, "    > Severity = %s", msgSeverity);
}
#endif

//----------------------------------------------------------------------------------
// Module Functions Definition - rlgl functionality
//----------------------------------------------------------------------------------

// Initialize rlgl: OpenGL extensions, default buffers/shaders/textures, OpenGL states
void rlglInit(int width, int height)
{
    isGpuReady = true;

    // Enable OpenGL debug context if requested (and supported)
#if defined(GRAPHICS_API_OPENGL_43) && RLGL_ENABLE_OPENGL_DEBUG_CONTEXT
    if ((glDebugMessageCallback != NULL) && (glDebugMessageControl != NULL))
    {
        glDebugMessageCallback(rlDebugMessageCallback, 0);
        // glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_HIGH, 0, 0, GL_TRUE);

        // Debug context options:
        //  - GL_DEBUG_OUTPUT - Faster version but not useful for breakpoints
        //  - GL_DEBUG_OUTPUT_SYNCHRONUS - Callback is in sync with errors, so a breakpoint can be placed on the callback in order to get a stacktrace for the GL error
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }
#endif

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    // Init default white texture
    unsigned char pixels[4] = { 255, 255, 255, 255 };   // 1 pixel RGBA (4 bytes)
    RLGL.State.defaultTextureId = rlLoadTexture(pixels, 1, 1, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    RLGL.State.currentTextureId = RLGL.State.defaultTextureId;

    if (RLGL.State.defaultTextureId != 0) TRACELOG(RL_LOG_INFO, "TEXTURE: [ID %i] Default texture loaded successfully", RLGL.State.defaultTextureId);
    else TRACELOG(RL_LOG_WARNING, "TEXTURE: Failed to load default texture");

    // Init default Shader (customized for GL 3.3 and ES2)
    // Loaded: RLGL.State.defaultShaderId + RLGL.State.defaultShaderLocs
    rlLoadShaderDefault();
    RLGL.State.currentShaderId = RLGL.State.defaultShaderId;
    RLGL.State.currentShaderLocs = RLGL.State.defaultShaderLocs;

    // Init default vertex arrays buffers
    // Simulate that the default shader has the location RL_SHADER_LOC_VERTEX_NORMAL to bind the normal buffer for the default render batch
    RLGL.State.currentShaderLocs[RL_SHADER_LOC_VERTEX_NORMAL] = RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL;
    RLGL.defaultBatch = rlLoadRenderBatch(RL_DEFAULT_BATCH_BUFFERS, RL_DEFAULT_BATCH_BUFFER_ELEMENTS);
    RLGL.State.currentShaderLocs[RL_SHADER_LOC_VERTEX_NORMAL] = -1;
    RLGL.currentBatch = &RLGL.defaultBatch;

    // Init stack matrices (emulating OpenGL 1.1)
    for (int i = 0; i < RL_MAX_MATRIX_STACK_SIZE; i++) RLGL.State.stack[i] = rlMatrixIdentity();

    // Init internal matrices
    RLGL.State.transform = rlMatrixIdentity();
    RLGL.State.projection = rlMatrixIdentity();
    RLGL.State.modelview = rlMatrixIdentity();
    RLGL.State.currentMatrix = &RLGL.State.modelview;
#endif // GRAPHICS_API_OPENGL_33 || GRAPHICS_API_OPENGL_ES2

#if defined(GRAPHICS_API_OPENGL_SOFTWARE)
    // Initialize software renderer backend
    int result = swInit(width, height);
    if (result == 0)
    {
        TRACELOG(RL_LOG_ERROR, "RLSW: Software renderer initialization failed!");
        exit(-1);
    }
#endif

    // Initialize OpenGL default states
    //----------------------------------------------------------
    // Init state: Depth test
    glDepthFunc(GL_LEQUAL);                                 // Type of depth testing to apply
    glDisable(GL_DEPTH_TEST);                               // Disable depth testing for 2D (only used for 3D)

    // Init state: Blending mode
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);      // Color blending function (how colors are mixed)
    glEnable(GL_BLEND);                                     // Enable color blending (required to work with transparencies)

    // Init state: Culling
    // NOTE: All shapes/models triangles are drawn CCW
    glCullFace(GL_BACK);                                    // Cull the back face (default)
    glFrontFace(GL_CCW);                                    // Front face are defined counter clockwise (default)
    glEnable(GL_CULL_FACE);                                 // Enable backface culling

#if defined(GRAPHICS_API_OPENGL_11)
    // Init state: Color hints (deprecated in OpenGL 3.0+)
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);      // Improve quality of color and texture coordinate interpolation
    glShadeModel(GL_SMOOTH);                                // Smooth shading between vertex (vertex colors interpolation)
#endif
#if defined(GRAPHICS_API_OPENGL_33)
    // Init state: Cubemap seamless
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);                 // Seamless cubemaps (not supported on OpenGL ES 2.0)
#endif
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    // Store screen size into global variables
    RLGL.State.framebufferWidth = width;
    RLGL.State.framebufferHeight = height;
#endif

    // Init state: Color/Depth buffers clear
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);                   // Set clear color (black)
    glClearDepth(1.0f);                                     // Set clear depth value (default)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // Clear color and depth buffers (depth buffer required for 3D)

    TRACELOG(RL_LOG_INFO, "RLGL: Default OpenGL state initialized successfully");
    //----------------------------------------------------------
}

// Vertex Buffer Object deinitialization (memory free)
void rlglClose(void)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    rlUnloadRenderBatch(RLGL.defaultBatch);

    rlUnloadShaderDefault(); // Unload default shader

    glDeleteTextures(1, &RLGL.State.defaultTextureId); // Unload default texture
    TRACELOG(RL_LOG_INFO, "TEXTURE: [ID %i] Default texture unloaded successfully", RLGL.State.defaultTextureId);
#endif

#if defined(GRAPHICS_API_OPENGL_SOFTWARE)
    swClose(); // Unload sofware renderer resources
#endif
    isGpuReady = false;
}

// Load OpenGL extensions
// NOTE: External loader function must be provided
void rlLoadExtensions(void *loader)
{
#if defined(GRAPHICS_API_OPENGL_33)     // Also defined for GRAPHICS_API_OPENGL_21
    // NOTE: glad is generated and contains only required OpenGL 3.3 Core extensions (and lower versions)
    if (gladLoadGL((GLADloadfunc)loader) == 0) TRACELOG(RL_LOG_WARNING, "GLAD: Cannot load OpenGL extensions");
    else TRACELOG(RL_LOG_INFO, "GLAD: OpenGL extensions loaded successfully");

    // Get number of supported extensions
    GLint numExt = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
    TRACELOG(RL_LOG_INFO, "GL: Supported extensions count: %i", numExt);

#if RLGL_SHOW_GL_DETAILS_INFO
    // Get supported extensions list
    // WARNING: glGetStringi() not available on OpenGL 2.1
    TRACELOG(RL_LOG_INFO, "GL: OpenGL extensions:");
    for (int i = 0; i < numExt; i++) TRACELOG(RL_LOG_INFO, "    %s", glGetStringi(GL_EXTENSIONS, i));
#endif

#if defined(GRAPHICS_API_OPENGL_21)
    // Register supported extensions flags
    // Optional OpenGL 2.1 extensions
    RLGL.ExtSupported.vao = GLAD_GL_ARB_vertex_array_object;
    RLGL.ExtSupported.instancing = (GLAD_GL_EXT_draw_instanced && GLAD_GL_ARB_instanced_arrays);
    RLGL.ExtSupported.texNPOT = GLAD_GL_ARB_texture_non_power_of_two;
    RLGL.ExtSupported.texFloat32 = GLAD_GL_ARB_texture_float;
    RLGL.ExtSupported.texFloat16 = GLAD_GL_ARB_texture_float;
    RLGL.ExtSupported.texDepth = GLAD_GL_ARB_depth_texture;
    RLGL.ExtSupported.maxDepthBits = 32;
    RLGL.ExtSupported.texAnisoFilter = GLAD_GL_EXT_texture_filter_anisotropic;
    RLGL.ExtSupported.texMirrorClamp = GLAD_GL_EXT_texture_mirror_clamp;
#else
    // Register supported extensions flags
    // OpenGL 3.3 extensions supported by default (core)
    RLGL.ExtSupported.vao = true;
    RLGL.ExtSupported.instancing = true;
    RLGL.ExtSupported.texNPOT = true;
    RLGL.ExtSupported.texFloat32 = true;
    RLGL.ExtSupported.texFloat16 = true;
    RLGL.ExtSupported.texDepth = true;
    RLGL.ExtSupported.maxDepthBits = 32;
    RLGL.ExtSupported.texAnisoFilter = true;
    RLGL.ExtSupported.texMirrorClamp = true;
#endif

    // Optional OpenGL 3.3 extensions
    RLGL.ExtSupported.texCompASTC = GLAD_GL_KHR_texture_compression_astc_hdr && GLAD_GL_KHR_texture_compression_astc_ldr;
    RLGL.ExtSupported.texCompDXT = GLAD_GL_EXT_texture_compression_s3tc;  // Texture compression: DXT
    RLGL.ExtSupported.texCompETC2 = GLAD_GL_ARB_ES3_compatibility;        // Texture compression: ETC2/EAC
    #if defined(GRAPHICS_API_OPENGL_43)
    RLGL.ExtSupported.computeShader = GLAD_GL_ARB_compute_shader;
    RLGL.ExtSupported.ssbo = GLAD_GL_ARB_shader_storage_buffer_object;
    #endif

#endif // GRAPHICS_API_OPENGL_33

#if defined(GRAPHICS_API_OPENGL_ES3)
    // Register supported extensions flags
    // OpenGL ES 3.0 extensions supported by default (or it should be)
    RLGL.ExtSupported.vao = true;
    RLGL.ExtSupported.instancing = true;
    RLGL.ExtSupported.texNPOT = true;
    RLGL.ExtSupported.texFloat32 = true;
    RLGL.ExtSupported.texFloat16 = true;
    RLGL.ExtSupported.texDepth = true;
    RLGL.ExtSupported.texDepthWebGL = true;
    RLGL.ExtSupported.maxDepthBits = 24;
    RLGL.ExtSupported.texAnisoFilter = true;
    RLGL.ExtSupported.texMirrorClamp = true;
    // TODO: Check for additional OpenGL ES 3.0 supported extensions:
    //RLGL.ExtSupported.texCompDXT = true;
    //RLGL.ExtSupported.texCompETC1 = true;
    //RLGL.ExtSupported.texCompETC2 = true;
    //RLGL.ExtSupported.texCompPVRT = true;
    //RLGL.ExtSupported.texCompASTC = true;
    //RLGL.ExtSupported.maxAnisotropyLevel = true;
    //RLGL.ExtSupported.computeShader = true;
    //RLGL.ExtSupported.ssbo = true;

#elif defined(GRAPHICS_API_OPENGL_ES2)

    #if defined(PLATFORM_DESKTOP_GLFW) || defined(PLATFORM_DESKTOP_SDL)
    // TODO: Support GLAD loader for OpenGL ES 3.0
    if (gladLoadGLES2((GLADloadfunc)loader) == 0) TRACELOG(RL_LOG_WARNING, "GLAD: Cannot load OpenGL ES2.0 functions");
    else TRACELOG(RL_LOG_INFO, "GLAD: OpenGL ES 2.0 loaded successfully");
    #endif

    // Get supported extensions list
    GLint numExt = 0;
    const char **extList = (const char **)RL_CALLOC(512, sizeof(const char *)); // Allocate 512 strings pointers (2 KB)
    const char *extensions = (const char *)glGetString(GL_EXTENSIONS);  // One big const string

    // NOTE: String duplication rquired because glGetString() returns a const string
    int extensionsLength = (int)strlen(extensions); // Get extensions string size in bytes
    char *extensionsDup = (char *)RL_CALLOC(extensionsLength + 1, sizeof(char)); // Allocate space for copy with additional EOL byte
    strncpy(extensionsDup, extensions, extensionsLength);
    extList[numExt] = extensionsDup;

    for (int i = 0; i < extensionsLength; i++)
    {
        if (extensionsDup[i] == ' ')
        {
            extensionsDup[i] = '\0';
            numExt++;
            extList[numExt] = &extensionsDup[i + 1];
        }
    }

    TRACELOG(RL_LOG_INFO, "GL: Supported extensions count: %i", numExt);

#if RLGL_SHOW_GL_DETAILS_INFO
    TRACELOG(RL_LOG_INFO, "GL: OpenGL extensions:");
    for (int i = 0; i < numExt; i++) TRACELOG(RL_LOG_INFO, "    %s", extList[i]);
#endif

    // Check required extensions
    for (int i = 0; i < numExt; i++)
    {
        // Check VAO support
        // NOTE: Only check on OpenGL ES, OpenGL 3.3 has VAO support as core feature
        if (strcmp(extList[i], (const char *)"GL_OES_vertex_array_object") == 0)
        {
            // The extension is supported by our hardware and driver, try to get related functions pointers
            // NOTE: emscripten does not support VAOs natively, it uses emulation and it reduces overall performance...
            glGenVertexArrays = (PFNGLGENVERTEXARRAYSOESPROC)((rlglLoadProc)loader)("glGenVertexArraysOES");
            glBindVertexArray = (PFNGLBINDVERTEXARRAYOESPROC)((rlglLoadProc)loader)("glBindVertexArrayOES");
            glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSOESPROC)((rlglLoadProc)loader)("glDeleteVertexArraysOES");
            //glIsVertexArray = (PFNGLISVERTEXARRAYOESPROC)loader("glIsVertexArrayOES");     // NOTE: Fails in WebGL, omitted

            if ((glGenVertexArrays != NULL) && (glBindVertexArray != NULL) && (glDeleteVertexArrays != NULL)) RLGL.ExtSupported.vao = true;
        }

        // Check instanced rendering support
        if (strstr(extList[i], (const char *)"instanced_arrays") != NULL)   // Broad check for instanced_arrays
        {
            // Specific check
            if (strcmp(extList[i], (const char *)"GL_ANGLE_instanced_arrays") == 0)      // ANGLE
            {
                glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawArraysInstancedANGLE");
                glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawElementsInstancedANGLE");
                glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISOREXTPROC)((rlglLoadProc)loader)("glVertexAttribDivisorANGLE");
            }
            else if (strcmp(extList[i], (const char *)"GL_EXT_instanced_arrays") == 0)   // EXT
            {
                glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawArraysInstancedEXT");
                glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawElementsInstancedEXT");
                glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISOREXTPROC)((rlglLoadProc)loader)("glVertexAttribDivisorEXT");
            }
            else if (strcmp(extList[i], (const char *)"GL_NV_instanced_arrays") == 0)    // NVIDIA GLES
            {
                glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawArraysInstancedNV");
                glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawElementsInstancedNV");
                glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISOREXTPROC)((rlglLoadProc)loader)("glVertexAttribDivisorNV");
            }

            // The feature will only be marked as supported if the elements from GL_XXX_instanced_arrays are present
            if ((glDrawArraysInstanced != NULL) && (glDrawElementsInstanced != NULL) && (glVertexAttribDivisor != NULL)) RLGL.ExtSupported.instancing = true;
        }
        else if (strstr(extList[i], (const char *)"draw_instanced") != NULL)
        {
            // GL_ANGLE_draw_instanced doesn't exist
            if (strcmp(extList[i], (const char *)"GL_EXT_draw_instanced") == 0)
            {
                glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawArraysInstancedEXT");
                glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawElementsInstancedEXT");
            }
            else if (strcmp(extList[i], (const char *)"GL_NV_draw_instanced") == 0)
            {
                glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawArraysInstancedNV");
                glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)((rlglLoadProc)loader)("glDrawElementsInstancedNV");
            }

            // But the functions will at least be loaded if only GL_XX_EXT_draw_instanced exist
            if ((glDrawArraysInstanced != NULL) && (glDrawElementsInstanced != NULL) && (glVertexAttribDivisor != NULL)) RLGL.ExtSupported.instancing = true;
        }

        // Check NPOT textures support
        // NOTE: Only check on OpenGL ES, OpenGL 3.3 has NPOT textures full support as core feature
        if (strcmp(extList[i], (const char *)"GL_OES_texture_npot") == 0) RLGL.ExtSupported.texNPOT = true;

        // Check texture float support
        if (strcmp(extList[i], (const char *)"GL_OES_texture_float") == 0) RLGL.ExtSupported.texFloat32 = true;
        if (strcmp(extList[i], (const char *)"GL_OES_texture_half_float") == 0) RLGL.ExtSupported.texFloat16 = true;

        // Check depth texture support
        if (strcmp(extList[i], (const char *)"GL_OES_depth_texture") == 0) RLGL.ExtSupported.texDepth = true;
        if (strcmp(extList[i], (const char *)"GL_WEBGL_depth_texture") == 0) RLGL.ExtSupported.texDepthWebGL = true; // WebGL requires unsized internal format
        if (RLGL.ExtSupported.texDepthWebGL) RLGL.ExtSupported.texDepth = true;

        if (strcmp(extList[i], (const char *)"GL_OES_depth24") == 0) RLGL.ExtSupported.maxDepthBits = 24; // Not available on WebGL
        if (strcmp(extList[i], (const char *)"GL_OES_depth32") == 0) RLGL.ExtSupported.maxDepthBits = 32; // Not available on WebGL

        // Check texture compression support: DXT
        if ((strcmp(extList[i], (const char *)"GL_EXT_texture_compression_s3tc") == 0) ||
            (strcmp(extList[i], (const char *)"GL_WEBGL_compressed_texture_s3tc") == 0) ||
            (strcmp(extList[i], (const char *)"GL_WEBKIT_WEBGL_compressed_texture_s3tc") == 0)) RLGL.ExtSupported.texCompDXT = true;

        // Check texture compression support: ETC1
        if ((strcmp(extList[i], (const char *)"GL_OES_compressed_ETC1_RGB8_texture") == 0) ||
            (strcmp(extList[i], (const char *)"GL_WEBGL_compressed_texture_etc1") == 0)) RLGL.ExtSupported.texCompETC1 = true;

        // Check texture compression support: ETC2/EAC
        if (strcmp(extList[i], (const char *)"GL_ARB_ES3_compatibility") == 0) RLGL.ExtSupported.texCompETC2 = true;

        // Check texture compression support: PVR
        if (strcmp(extList[i], (const char *)"GL_IMG_texture_compression_pvrtc") == 0) RLGL.ExtSupported.texCompPVRT = true;

        // Check texture compression support: ASTC
        if (strcmp(extList[i], (const char *)"GL_KHR_texture_compression_astc_hdr") == 0) RLGL.ExtSupported.texCompASTC = true;

        // Check anisotropic texture filter support
        if (strcmp(extList[i], (const char *)"GL_EXT_texture_filter_anisotropic") == 0) RLGL.ExtSupported.texAnisoFilter = true;

        // Check clamp mirror wrap mode support
        if (strcmp(extList[i], (const char *)"GL_EXT_texture_mirror_clamp") == 0) RLGL.ExtSupported.texMirrorClamp = true;
    }

    // Free extensions pointers
    RL_FREE(extList);
    RL_FREE(extensionsDup);    // Duplicated string must be deallocated
#endif // GRAPHICS_API_OPENGL_ES2

    // Check OpenGL information and capabilities
    //------------------------------------------------------------------------------
    // Show current OpenGL and GLSL version
    TRACELOG(RL_LOG_INFO, "GL: OpenGL device information:");
    TRACELOG(RL_LOG_INFO, "    > Vendor:   %s", glGetString(GL_VENDOR));
    TRACELOG(RL_LOG_INFO, "    > Renderer: %s", glGetString(GL_RENDERER));
    TRACELOG(RL_LOG_INFO, "    > Version:  %s", glGetString(GL_VERSION));
    TRACELOG(RL_LOG_INFO, "    > GLSL:     %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    RLGL.loader = (rlglLoadProc)loader;

    // NOTE: Anisotropy levels capability is an extension
    #ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
        #define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
    #endif
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &RLGL.ExtSupported.maxAnisotropyLevel);

#if RLGL_SHOW_GL_DETAILS_INFO
    // Show some OpenGL GPU capabilities
    TRACELOG(RL_LOG_INFO, "GL: OpenGL capabilities:");
    GLint capability = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &capability);
    TRACELOG(RL_LOG_INFO, "    GL_MAX_TEXTURE_SIZE: %i", capability);
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &capability);
    TRACELOG(RL_LOG_INFO, "    GL_MAX_CUBE_MAP_TEXTURE_SIZE: %i", capability);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &capability);
    TRACELOG(RL_LOG_INFO, "    GL_MAX_TEXTURE_IMAGE_UNITS: %i", capability);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &capability);
    TRACELOG(RL_LOG_INFO, "    GL_MAX_VERTEX_ATTRIBS: %i", capability);
    #if !defined(GRAPHICS_API_OPENGL_ES2)
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &capability);
    TRACELOG(RL_LOG_INFO, "    GL_MAX_UNIFORM_BLOCK_SIZE: %i", capability);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &capability);
    TRACELOG(RL_LOG_INFO, "    GL_MAX_DRAW_BUFFERS: %i", capability);
    if (RLGL.ExtSupported.texAnisoFilter) TRACELOG(RL_LOG_INFO, "    GL_MAX_TEXTURE_MAX_ANISOTROPY: %.0f", RLGL.ExtSupported.maxAnisotropyLevel);
    #endif
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &capability);
    TRACELOG(RL_LOG_INFO, "    GL_NUM_COMPRESSED_TEXTURE_FORMATS: %i", capability);
    GLint *compFormats = (GLint *)RL_CALLOC(capability, sizeof(GLint));
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, compFormats);
    for (int i = 0; i < capability; i++) TRACELOG(RL_LOG_INFO, "        %s", rlGetCompressedFormatName(compFormats[i]));
    RL_FREE(compFormats);

#if defined(GRAPHICS_API_OPENGL_43)
    glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &capability);
    TRACELOG(RL_LOG_INFO, "    GL_MAX_VERTEX_ATTRIB_BINDINGS: %i", capability);
    glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &capability);
    TRACELOG(RL_LOG_INFO, "    GL_MAX_UNIFORM_LOCATIONS: %i", capability);
#endif

#else   // !RLGL_SHOW_GL_DETAILS_INFO

    // Show some basic info about GL supported features
    if (RLGL.ExtSupported.vao) TRACELOG(RL_LOG_INFO, "GL: VAO extension detected, VAO functions loaded successfully");
    else TRACELOG(RL_LOG_WARNING, "GL: VAO extension not found, VAO not supported");
    if (RLGL.ExtSupported.texNPOT) TRACELOG(RL_LOG_INFO, "GL: NPOT textures extension detected, full NPOT textures supported");
    else TRACELOG(RL_LOG_WARNING, "GL: NPOT textures extension not found, limited NPOT support (no-mipmaps, no-repeat)");
    if (RLGL.ExtSupported.texCompDXT) TRACELOG(RL_LOG_INFO, "GL: DXT compressed textures supported");
    if (RLGL.ExtSupported.texCompETC1) TRACELOG(RL_LOG_INFO, "GL: ETC1 compressed textures supported");
    if (RLGL.ExtSupported.texCompETC2) TRACELOG(RL_LOG_INFO, "GL: ETC2/EAC compressed textures supported");
    if (RLGL.ExtSupported.texCompPVRT) TRACELOG(RL_LOG_INFO, "GL: PVRT compressed textures supported");
    if (RLGL.ExtSupported.texCompASTC) TRACELOG(RL_LOG_INFO, "GL: ASTC compressed textures supported");
    if (RLGL.ExtSupported.computeShader) TRACELOG(RL_LOG_INFO, "GL: Compute shaders supported");
    if (RLGL.ExtSupported.ssbo) TRACELOG(RL_LOG_INFO, "GL: Shader storage buffer objects supported");
#endif

#endif // GRAPHICS_API_OPENGL_33 || GRAPHICS_API_OPENGL_ES2
}

// Get OpenGL procedure address
void *rlGetProcAddress(const char *procName)
{
    void *func = NULL;
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    func = RLGL.loader(procName);
#endif
    return func;
}

// Get current OpenGL version
int rlGetVersion(void)
{
    int glVersion = 0;

#if defined(GRAPHICS_API_OPENGL_SOFTWARE)
