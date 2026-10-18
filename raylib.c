/**********************************************************************************************
*
*   raylib v6.0 - A simple and easy-to-use library to enjoy videogames programming (www.raylib.com)
*
*   FEATURES:
*       - NO external dependencies, all required libraries included with raylib
*       - Multiplatform: Windows, Linux, macOS, FreeBSD, Web, Android, Raspberry Pi, DRM native...
*       - Written in plain C code (C99) in PascalCase/camelCase notation
*       - Hardware accelerated with OpenGL (1.1, 2.1, 3.3, 4.3, ES2, ES3 - choose at compile)
*       - Software renderer optional, for systems with no GPU: [rlsw]
*       - Custom OpenGL abstraction layer (usable as standalone module): [rlgl]
*       - Multiple Fonts formats supported (TTF, OTF, FNT, BDF, Sprite fonts)
*       - Many texture formats supported, including compressed formats (DXT, ETC, ASTC)
*       - Full 3d support for 3d Shapes, Models, Billboards, Heightmaps and more!
*       - Flexible Materials system, supporting classic maps and PBR maps
*       - Animated 3D models supported (skeletal bones animation) (IQM, M3D, GLTF)
*       - Shaders support, including Model shaders and Postprocessing shaders
*       - Powerful math module for Vector, Matrix and Quaternion operations: [raymath]
*       - Audio loading and playing with streaming support (WAV, OGG, MP3, FLAC, QOA, XM, MOD)
*       - VR stereo rendering with configurable HMD device parameters
*       - Bindings to multiple programming languages available!
*
*   NOTES:
*       - One default Font is loaded on InitWindow()->LoadFontDefault() [core, text]
*       - One default Texture2D is loaded on rlglInit(), 1x1 white pixel R8G8B8A8 [rlgl] (OpenGL 3.3 or ES2)
*       - One default Shader is loaded on rlglInit()->rlLoadShaderDefault() [rlgl] (OpenGL 3.3 or ES2)
*       - One default RenderBatch is loaded on rlglInit()->rlLoadRenderBatch() [rlgl] (OpenGL 3.3 or ES2)
*
*   DEPENDENCIES:
*       [rcore] Depends on the selected platform backend, check rcore.c header for details
*       [rlgl] glad/glad_gles2 (David Herberth - github.com/Dav1dde/glad) for OpenGL extensions loading
*       [raudio] miniaudio (David Reid - github.com/mackron/miniaudio) for audio device/context management
*
*   OPTIONAL DEPENDENCIES (included):
*       [rcore] sinfl (Micha Mettke) for DEFLATE decompression algorithm
*       [rcore] sdefl (Micha Mettke) for DEFLATE compression algorithm
*       [rcore] rprand (Ramon Santamaria) for pseudo-random numbers generation
*       [rtextures] qoi (Dominic Szablewski - https://phoboslab.org) for QOI image management
*       [rtextures] stb_image (Sean Barrett) for images loading (BMP, TGA, PNG, JPEG, HDR...)
*       [rtextures] stb_image_write (Sean Barrett) for image writing (BMP, TGA, PNG, JPG)
*       [rtextures] stb_image_resize2 (Sean Barrett) for image resizing algorithms
*       [rtextures] stb_perlin (Sean Barrett) for Perlin Noise image generation
*       [rtextures] rltexgpu (Ramon Santamaria) for GPU-compressed texture formats
*       [rtext] stb_truetype (Sean Barrett) for ttf fonts loading
*       [rtext] stb_rect_pack (Sean Barrett) for rectangles packing
*       [rmodels] par_shapes (Philip Rideout) for parametric 3d shapes generation
*       [rmodels] tinyobj_loader_c (Syoyo Fujita) for models loading (OBJ, MTL)
*       [rmodels] cgltf (Johannes Kuhlmann) for models loading (glTF)
*       [rmodels] m3d (bzt) for models loading (M3D, https://bztsrc.gitlab.io/model3d)
*       [rmodels] vox_loader (Johann Nadalutti) for models loading (VOX)
*       [raudio] dr_wav (David Reid) for WAV audio file loading
*       [raudio] dr_flac (David Reid) for FLAC audio file loading
*       [raudio] dr_mp3 (David Reid) for MP3 audio file loading
*       [raudio] stb_vorbis (Sean Barrett) for OGG audio loading
*       [raudio] jar_xm (Joshua Reisenauer) for XM audio module loading
*       [raudio] jar_mod (Joshua Reisenauer) for MOD audio module loading
*       [raudio] qoa (Dominic Szablewski - https://phoboslab.org) for QOA audio management
*
*
*   LICENSE: zlib/libpng
*
*   raylib is licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software:
*
*   Copyright (c) 2013-2026 Ramon Santamaria (@raysan5)
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

#ifndef RAYLIB_H
#define RAYLIB_H

#include <stdarg.h>     // Required for: va_list - Only used by TraceLogCallback

#define RAYLIB_VERSION_MAJOR 6
#define RAYLIB_VERSION_MINOR 0
#define RAYLIB_VERSION_PATCH 0
#define RAYLIB_VERSION  "6.0"

// Function specifiers in case library is build/used as a shared library
// NOTE: Microsoft specifiers to tell compiler that symbols are imported/exported from a .dll
// NOTE: visibility("default") attribute makes symbols "visible" when compiled with -fvisibility=hidden
#if defined(_WIN32)
    #if defined(__TINYC__)
        #define __declspec(x) __attribute__((x))
    #endif
    #if defined(BUILD_LIBTYPE_SHARED)
        #define RLAPI __declspec(dllexport)     // Building the library as a Win32 shared library (.dll)
    #elif defined(USE_LIBTYPE_SHARED)
        #define RLAPI __declspec(dllimport)     // Using the library as a Win32 shared library (.dll)
    #endif
#else
    #if defined(BUILD_LIBTYPE_SHARED)
        #define RLAPI __attribute__((visibility("default"))) // Building as a Unix shared library (.so/.dylib)
    #endif
#endif

#ifndef RLAPI
    #define RLAPI       // Functions defined as 'extern' by default (implicit specifiers)
#endif

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

// Allow custom memory allocators
// NOTE: Require recompiling raylib sources
#ifndef RL_MALLOC
    #define RL_MALLOC(sz)       malloc(sz)
#endif
#ifndef RL_CALLOC
    #define RL_CALLOC(n,sz)     calloc(n,sz)
#endif
#ifndef RL_REALLOC
    #define RL_REALLOC(ptr,sz)  realloc(ptr,sz)
#endif
#ifndef RL_FREE
    #define RL_FREE(ptr)        free(ptr)
#endif

// NOTE: MSVC C++ compiler does not support compound literals (C99 feature)
// Plain structures in C++ (without constructors) can be initialized with { }
// This is called aggregate initialization (C++11 feature)
#if defined(__cplusplus)
    #define CLITERAL(type)      type
#else
    #define CLITERAL(type)      (type)
#endif

// Some compilers (mostly macos clang) default to C++98,
// where aggregate initialization can't be used
// So, give a more clear error stating how to fix this
#if !defined(_MSC_VER) && (defined(__cplusplus) && __cplusplus < 201103L)
    #error "C++11 or later is required. Add -std=c++11"
#endif

// NOTE: Set some defines with some data types declared by raylib
// Other modules (raymath, rlgl) also require some of those types, so,
// to be able to use those other modules as standalone (not depending on raylib)
// this defines are useful for internal check and avoid type (re)definitions
#define RL_COLOR_TYPE
#define RL_RECTANGLE_TYPE
#define RL_VECTOR2_TYPE
#define RL_VECTOR3_TYPE
#define RL_VECTOR4_TYPE
#define RL_QUATERNION_TYPE
#define RL_MATRIX_TYPE

// Some Basic Colors
// NOTE: Custom raylib color palette for amazing visuals on WHITE background
#define LIGHTGRAY  CLITERAL(Color){ 200, 200, 200, 255 }   // Light Gray
#define GRAY       CLITERAL(Color){ 130, 130, 130, 255 }   // Gray
#define DARKGRAY   CLITERAL(Color){ 80, 80, 80, 255 }      // Dark Gray
#define YELLOW     CLITERAL(Color){ 253, 249, 0, 255 }     // Yellow
#define GOLD       CLITERAL(Color){ 255, 203, 0, 255 }     // Gold
#define ORANGE     CLITERAL(Color){ 255, 161, 0, 255 }     // Orange
#define PINK       CLITERAL(Color){ 255, 109, 194, 255 }   // Pink
#define RED        CLITERAL(Color){ 230, 41, 55, 255 }     // Red
#define MAROON     CLITERAL(Color){ 190, 33, 55, 255 }     // Maroon
#define GREEN      CLITERAL(Color){ 0, 228, 48, 255 }      // Green
#define LIME       CLITERAL(Color){ 0, 158, 47, 255 }      // Lime
#define DARKGREEN  CLITERAL(Color){ 0, 117, 44, 255 }      // Dark Green
#define SKYBLUE    CLITERAL(Color){ 102, 191, 255, 255 }   // Sky Blue
#define BLUE       CLITERAL(Color){ 0, 121, 241, 255 }     // Blue
#define DARKBLUE   CLITERAL(Color){ 0, 82, 172, 255 }      // Dark Blue
#define PURPLE     CLITERAL(Color){ 200, 122, 255, 255 }   // Purple
#define VIOLET     CLITERAL(Color){ 135, 60, 190, 255 }    // Violet
#define DARKPURPLE CLITERAL(Color){ 112, 31, 126, 255 }    // Dark Purple
#define BEIGE      CLITERAL(Color){ 211, 176, 131, 255 }   // Beige
#define BROWN      CLITERAL(Color){ 127, 106, 79, 255 }    // Brown
#define DARKBROWN  CLITERAL(Color){ 76, 63, 47, 255 }      // Dark Brown

#define WHITE      CLITERAL(Color){ 255, 255, 255, 255 }   // White
#define BLACK      CLITERAL(Color){ 0, 0, 0, 255 }         // Black
#define BLANK      CLITERAL(Color){ 0, 0, 0, 0 }           // Blank (Transparent)
#define MAGENTA    CLITERAL(Color){ 255, 0, 255, 255 }     // Magenta
#define RAYWHITE   CLITERAL(Color){ 245, 245, 245, 255 }   // My own White (raylib logo)

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Boolean type
#if (defined(__STDC__) && __STDC_VERSION__ >= 199901L) || (defined(_MSC_VER) && _MSC_VER >= 1800)
    #include <stdbool.h>
#elif !defined(__cplusplus) && !defined(bool)
    typedef enum bool { false = 0, true = !false } bool;
    #define RL_BOOL_TYPE
#endif

// Vector2, 2 components
typedef struct Vector2 {
    float x;                // Vector x component
    float y;                // Vector y component
} Vector2;

// Vector3, 3 components
typedef struct Vector3 {
    float x;                // Vector x component
    float y;                // Vector y component
    float z;                // Vector z component
} Vector3;

// Vector4, 4 components
typedef struct Vector4 {
    float x;                // Vector x component
    float y;                // Vector y component
    float z;                // Vector z component
    float w;                // Vector w component
} Vector4;

// Quaternion, 4 components (Vector4 alias)
typedef Vector4 Quaternion;

// Matrix, 4x4 components, column major, OpenGL style, right-handed
typedef struct Matrix {
    float m0, m4, m8, m12;  // Matrix first row (4 components)
    float m1, m5, m9, m13;  // Matrix second row (4 components)
    float m2, m6, m10, m14; // Matrix third row (4 components)
    float m3, m7, m11, m15; // Matrix fourth row (4 components)
} Matrix;

// Color, 4 components, R8G8B8A8 (32bit)
typedef struct Color {
    unsigned char r;        // Color red value
    unsigned char g;        // Color green value
    unsigned char b;        // Color blue value
    unsigned char a;        // Color alpha value
