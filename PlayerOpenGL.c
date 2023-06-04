/* Compilation CMD
gcc PlayerOpenGL.c -o PlayerOpenGL -I/usr/include/gstreamer-1.0 -I/usr/include/glib-2.0 -I/usr/lib/aarch64-linux-gnu/glib-2.0/include/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/lib/i386-linux-gnu/glib-2.0/include/ -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 -lgstapp-1.0 -lGLEW -lGL -lX11 -lpthread -lavformat -lavcodec -lavutil
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/gl.h>

#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

typedef struct {
	unsigned long   flags;
	unsigned long   functions;
	unsigned long   decorations;
	long            inputMode;
	unsigned long   status;
} Hints;

typedef struct {
	float x;
	float y;
} Vector2;

typedef struct {
	Vector2 texCoordX1Y1;
	Vector2 texCoordX1Y2;
	Vector2 texCoordX2Y2;
	Vector2 texCoordX2Y1;
} TexCoord;

unsigned int winWidth = 800;
unsigned int winHeight = 600;
unsigned int videoWidth = 1280;
unsigned int videoHeight = 720;
unsigned int screenWidth = 1920;
unsigned int screenHeight = 1080;

static int32_t snglBuf[] = {GLX_RGBA, GLX_DEPTH_SIZE, 16, None};
static int32_t dblBuf[]  = {GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None};
GLboolean doubleBuffer = GL_TRUE;

GLboolean exitLoop = GL_FALSE;

unsigned char* frameData;
uint32_t pixelFormat = 4;

// FFMPEG Declarations
AVFormatContext* avFormatContext        = NULL;
AVCodecContext*  avCodecContext         = NULL;
AVFrame*         avFrame[2]             = {NULL, NULL};
int64_t          pts                    = 0;
int              vidStrId               = -1;
bool             ffmpegAvFrameIdx       = false;
bool             ffmpegThreadLive       = false;
//bool             newFrameFlag[2]        = {false, false};
pthread_mutex_t* ffmpegLoadFrameMutex   = NULL;
pthread_mutex_t* ffmpegBufferMutex[2]   = {NULL, NULL};
pthread_mutex_t* ffmpegNewFrameMutex[2] = {NULL, NULL};

uint8_t decimalZero[FONT_HEIGHT][FONT_WIDTH] =
{
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
};


uint8_t decimalOne[FONT_HEIGHT][FONT_WIDTH] =
{
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
};


uint8_t decimalTwo[FONT_HEIGHT][FONT_WIDTH] =
{
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
};


uint8_t decimalThree[FONT_HEIGHT][FONT_WIDTH] =
{
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
};


uint8_t decimalFour[FONT_HEIGHT][FONT_WIDTH] =
{
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
};


uint8_t decimalFive[FONT_HEIGHT][FONT_WIDTH] =
{
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
};


uint8_t decimalSix[FONT_HEIGHT][FONT_WIDTH] =
{
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
};


uint8_t decimalSeven[FONT_HEIGHT][FONT_WIDTH] =
{
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
};


uint8_t decimalEight[FONT_HEIGHT][FONT_WIDTH] =
{
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
};


uint8_t decimalNine[FONT_HEIGHT][FONT_WIDTH] =
{
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, },
{ 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
};


uint8_t decimalDot[FONT_HEIGHT][FONT_WIDTH] =
{
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
};





void checkStatus(uint32_t programId, GLboolean typeProgram) {
	int32_t compileStatus;
	int32_t infoLogLength;
	int32_t bufferSize;
	char* buffer;
	if(typeProgram) {
		glGetProgramiv(programId, GL_LINK_STATUS, &compileStatus);
		if(compileStatus != GL_TRUE) {
			glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLength);
			buffer = malloc(infoLogLength);
			glGetProgramInfoLog(programId, infoLogLength, &bufferSize, buffer);
			printf("%s \n", buffer);
			free(buffer);
		}
	} else {
		glGetShaderiv(programId, GL_COMPILE_STATUS, &compileStatus);
		if(compileStatus != GL_TRUE) {
			glGetShaderiv(programId, GL_INFO_LOG_LENGTH, &infoLogLength);
			buffer = malloc(infoLogLength);
			glGetShaderInfoLog(programId, infoLogLength, &bufferSize, buffer);
			printf("%s \n", buffer);
			free(buffer);
		}
	}
}

uint32_t CompileShader(uint32_t type, const char* source) {
	uint32_t id = glCreateShader(type);
	glShaderSource(id, 1, &source, NULL);
	glCompileShader(id);
	checkStatus(id, GL_FALSE);
	return id;
}

uint32_t CreateShader(const char* vertexShader, const char* fragmentShader) {
	uint32_t program = glCreateProgram();
	uint32_t vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	checkStatus(program, GL_TRUE);
	glValidateProgram(program);
	glDeleteShader(vs);
	glDeleteShader(fs);
	return program;
}


unsigned int generateVertexArrayObject(unsigned int programId, int texIndex, float zIndex, TexCoord * surfTexCoord, TexCoord * textTexCoord) {
	unsigned int quadVert = 4;
	uint32_t vertexElementSize = 2 * 4 * sizeof(GLfloat) + 2 * sizeof(GLfloat) + sizeof(GLfloat);
	GLuint indexArray[] = { 0, 1, 2, 3 };

	Vector2 texCoordX1Y1 = {0.0f, 0.0f};
	Vector2 texCoordX1Y2 = {0.0f, 1.0f};
	Vector2 texCoordX2Y2 = {1.0f, 1.0f};
	Vector2 texCoordX2Y1 = {1.0f, 0.0f};

	if (textTexCoord != NULL) {
		texCoordX1Y1 = textTexCoord->texCoordX1Y1;
		texCoordX1Y2 = textTexCoord->texCoordX1Y2;
		texCoordX2Y2 = textTexCoord->texCoordX2Y2;
		texCoordX2Y1 = textTexCoord->texCoordX2Y1;
	}

	Vector2 surfCoordX1Y1 = {-1.0f, +1.0f};
	Vector2 surfCoordX1Y2 = {-1.0f, -1.0f};
	Vector2 surfCoordX2Y2 = {+1.0f, -1.0f};
	Vector2 surfCoordX2Y1 = {+1.0f, +1.0f};

	if (surfTexCoord != NULL) {
		surfCoordX1Y1 = surfTexCoord->texCoordX1Y1;
		surfCoordX1Y2 = surfTexCoord->texCoordX1Y2;
		surfCoordX2Y2 = surfTexCoord->texCoordX2Y2;
		surfCoordX2Y1 = surfTexCoord->texCoordX2Y1;
	}


	GLfloat vertexArray[] = {
			// Position                |   Red| Green|  Blue| Alpha|     X|     Y|  Tex
			surfCoordX1Y1.x, surfCoordX1Y1.y, zIndex, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, texCoordX1Y1.x, texCoordX1Y1.y, (float)texIndex,
			surfCoordX1Y2.x, surfCoordX1Y2.y, zIndex, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, texCoordX1Y2.x, texCoordX1Y2.y, (float)texIndex,
			surfCoordX2Y2.x, surfCoordX2Y2.y, zIndex, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, texCoordX2Y2.x, texCoordX2Y2.y, (float)texIndex,
			surfCoordX2Y1.x, surfCoordX2Y1.y, zIndex, +1.0f, +1.0f, +1.0f, +1.0f, +1.0f, texCoordX2Y1.x, texCoordX2Y1.y, (float)texIndex
	};

	unsigned int vertexArrayId;
	unsigned int elementBufferId;
	unsigned int vertexBufferId;
	glGenVertexArrays(1, &vertexArrayId);
	glGenBuffers(1, &elementBufferId);
	glGenBuffers(1, &vertexBufferId);

	glBindVertexArray(vertexArrayId);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, quadVert * sizeof(GLuint), indexArray, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
	glBufferData(GL_ARRAY_BUFFER, quadVert * vertexElementSize, vertexArray, GL_STATIC_DRAW);

	unsigned int vertexTypeSize = vertexElementSize;
	GLuint vertexLoc = glGetAttribLocation(programId, "vertPosition");
	glEnableVertexAttribArray(vertexLoc);
	glVertexAttribPointer(vertexLoc, 4, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)(sizeof(GLfloat) * 0));
	GLuint colorLoc = glGetAttribLocation(programId, "vertColor");
	glEnableVertexAttribArray(colorLoc);
	glVertexAttribPointer(colorLoc, 4, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)(sizeof(GLfloat) * 4));
	GLuint texCoordLoc = glGetAttribLocation(programId, "vertTexCoord");
	glEnableVertexAttribArray(texCoordLoc);
	glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)(sizeof(GLfloat) * 8));
	GLuint texIndexLoc = glGetAttribLocation(programId, "vertTexIndex");
	glEnableVertexAttribArray(texIndexLoc);
	glVertexAttribPointer(texIndexLoc, 1, GL_FLOAT, GL_FALSE, vertexTypeSize, (void*)(sizeof(GLfloat) * 10));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

	return vertexArrayId;
}




bool ffmpegOpenVideoFile(AVFormatContext** avFmtCtx, AVCodecContext** avCodecCtx, uint32_t* width, uint32_t* height, int* vidStrId, const char* filename) {
  AVFormatContext* avFmtCxtLocal = avformat_alloc_context();

  av_register_all();
  avcodec_register_all();
  //avformat_network_init();

  if(avformat_open_input(&avFmtCxtLocal, filename, NULL, NULL) != 0){
    printf("Network source %s was not found \n", filename);
    exit(EXIT_FAILURE);
  }

  AVCodec *avCodec;
  AVCodecParameters* avCodecParameters;
  *vidStrId = -1;
  for (unsigned int i = 0; i < avFmtCxtLocal->nb_streams; i++) {
    avCodecParameters = avFmtCxtLocal->streams[i]->codecpar;
    avCodec = avcodec_find_decoder(avCodecParameters->codec_id);

    if (!avCodec) {
      continue;
    }
    if (avCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      *vidStrId = i;
      if (!avCodec) {
	exit(EXIT_FAILURE);
      }
      AVCodecContext* avCodecCtxLocal = avcodec_alloc_context3(avCodec);
      avcodec_parameters_to_context(avCodecCtxLocal, avCodecParameters);

      if (avcodec_open2(avCodecCtxLocal, avCodec, NULL) < 0) {
	exit(EXIT_FAILURE);
      }

      *avFmtCtx = avFmtCxtLocal;
      *avCodecCtx = avCodecCtxLocal;

      printf("avCodecCtxLocal->pix_fmt : %d \n", avCodecCtxLocal->pix_fmt);

      *width = avCodecCtxLocal->width;
      *height = avCodecCtxLocal->height;
      break;
    }
  }

  avFrame[0] = av_frame_alloc();
  avFrame[1] = av_frame_alloc();

  ffmpegThreadLive = true;

  return true;
}


int av_read_frame_return_value(AVFormatContext* avFmtCtx, AVPacket* avPacketLocal, int* responseValue) {
  int retValue = av_read_frame(avFmtCtx, avPacketLocal);
  *responseValue = retValue;
  return retValue;
}


int ffmpegLoadFrame(AVFormatContext* avFmtCtx, AVCodecContext* avCodCtx, int* vidStrId, int64_t* pts, bool bufferIdx) {
  bool ret = true;

  av_frame_free(&avFrame[bufferIdx]);
  avFrame[bufferIdx] = av_frame_alloc();
  AVPacket* avPacketLocal = av_packet_alloc();

  int response;
  int frameReadSuccessful = 1;
  while (av_read_frame_return_value(avFmtCtx, avPacketLocal, &frameReadSuccessful) >= 0) {
    if (frameReadSuccessful == AVERROR_EOF) {
      printf("AVERROR_EOF \n");
    }
    if (avPacketLocal->stream_index != *vidStrId) {
      av_packet_unref(avPacketLocal);
      continue;
    }
    response = avcodec_send_packet(avCodCtx, avPacketLocal);
    if (ret && response < 0) {
      printf("Failed to decode packet \n");
      ret = false;
    }
    response = avcodec_receive_frame(avCodCtx, avFrame[bufferIdx]);
    if (response == AVERROR(EAGAIN) ) {
      continue;
    } else if (response == AVERROR_EOF) {
      printf("AVERROR_EOF \n");
      continue;
    } else if (ret && response < 0) {
      printf("Failed to decode packet \n");
      ret = false;
    }

    av_packet_unref(avPacketLocal);
    break;
  }

  //memcpy(frameData, avFrame[ffmpegAvFrameIdx]->data[0], videoWidth * videoHeight * sizeof(unsigned char));
  //memcpy(lumaVideoData, avFrameLocal->data[0], srcVideoWidth * srcVideoHeight * sizeof(uint8_t));
  //memcpy(chromaRedVideoData, avFrameLocal->data[2], (srcVideoWidth / 2) * (srcVideoHeight / 2) * sizeof(uint8_t));
  //memcpy(chromaBlueVideoData, avFrameLocal->data[1], (srcVideoWidth / 2) * (srcVideoHeight / 2) * sizeof(uint8_t));
  
  pthread_mutex_lock(ffmpegNewFrameMutex[bufferIdx]);
  //if ( !newFrame ) {
  //  ffmpegAvFrameIdx = bufferIdx;
  //}
  
  //newFrameFlag[bufferIdx] = true;
  
  //printf("newFrame = true : Buffer %X \n", bufferIdx);
  
  //if (avFrame[!ffmpegAvFrameIdx] != NULL) {
  //  av_frame_free(&avFrame[!ffmpegAvFrameIdx]);
  //}
  av_packet_free(&avPacketLocal);

  return 0;
}

int ffmpegCloseVideoFile(AVFormatContext* avFmtCtx, AVCodecContext* avCodCtx) {
  printf("[ClientRTSP] closeVideoFile() \n");

  //pthread_mutex_unlock(ffmpegNewFrameMutex[0]);    
  //pthread_mutex_unlock(ffmpegBufferMutex[0]);
  //pthread_mutex_unlock(ffmpegNewFrameMutex[1]);    
  //pthread_mutex_unlock(ffmpegBufferMutex[1]);
  //pthread_mutex_unlock(ffmpegLoadFrameMutex);

  pthread_mutex_destroy(ffmpegNewFrameMutex[0]);
  pthread_mutex_destroy(ffmpegNewFrameMutex[1]);
  pthread_mutex_destroy(ffmpegBufferMutex[0]);
  pthread_mutex_destroy(ffmpegBufferMutex[1]);
  pthread_mutex_destroy(ffmpegLoadFrameMutex);
  printf("[ClientRTSP] pthread_mutex_destroy() \n");

  if(avFrame[0] != NULL) {
    av_frame_free(&avFrame[0]);
    printf("[ClientRTSP] av_frame_free(&avFrame) \n");
  }
  if(avFrame[1] != NULL) {
    av_frame_free(&avFrame[1]);
    printf("[ClientRTSP] av_frame_free(&avFrame) \n");
  }
  if(avFmtCtx != NULL) {
    avformat_close_input(&avFmtCtx);
    printf("[ClientRTSP] avformat_close_input(&avFmtCtx) \n");
  }
  if(avFmtCtx != NULL) {
    avformat_free_context(avFmtCtx);
    printf("[ClientRTSP] avformat_free_context(avFmtCtx) \n");
  }
  if(avCodCtx != NULL) {
    avcodec_free_context(&avCodCtx);
    printf("[ClientRTSP] avcodec_free_context(&avCodecCtx) \n");
  }

  return 0;
}

void *ffmpegBufferZeroThreadHandler(void *data) {
  char *name = (char*)data;
  printf("[ffmpegBufferZeroThreadHandler] Start Thread : %s \n", name);
  while (ffmpegThreadLive) {
    pthread_mutex_lock(ffmpegNewFrameMutex[0]);
    pthread_mutex_unlock(ffmpegNewFrameMutex[0]);
    
    pthread_mutex_lock(ffmpegBufferMutex[0]);
    
    pthread_mutex_lock(ffmpegLoadFrameMutex);
    ffmpegLoadFrame(avFormatContext, avCodecContext, &vidStrId, &pts, false);
    pthread_mutex_unlock(ffmpegLoadFrameMutex);

    pthread_mutex_unlock(ffmpegBufferMutex[0]);
  }
  printf("[ffmpegBufferZeroThreadHandler] Stop Thread : %s \n", name);
  return NULL;
}

void *ffmpegBufferOneThreadHandler(void *data) {
  char *name = (char*)data;
  printf("[ffmpegBufferOneThreadHandler] Start Thread : %s \n", name);
  while (ffmpegThreadLive) {
    pthread_mutex_lock(ffmpegNewFrameMutex[1]);
    pthread_mutex_unlock(ffmpegNewFrameMutex[1]);
    
    pthread_mutex_lock(ffmpegBufferMutex[1]);
    
    pthread_mutex_lock(ffmpegLoadFrameMutex);
    ffmpegLoadFrame(avFormatContext, avCodecContext, &vidStrId, &pts, true);
    pthread_mutex_unlock(ffmpegLoadFrameMutex);

    pthread_mutex_unlock(ffmpegBufferMutex[1]);    
  }
  printf("[ffmpegBufferOneThreadHandler] Stop Thread : %s \n", name);
  return NULL;
}

int main(int argc, char **argv) {
  printf("argc : %d \n", argc);
  if (argc < 2) {
    printf("Specify the file to open. \n");
    return 1;
  }
  printf("argv : %s \n", (char*)argv[1]);
  char* fileName = (char*)argv[1];

  bool fullscreen = true;
  if ( argc > 2 ) {
    printf("argv : %s \n", (char*)argv[2]);
    if ( strcmp ( (const char*)argv[2], "-window" ) == 0 ) {
      fullscreen = false;
    }
  }
  
  Display* dpy = XOpenDisplay(NULL);
  XVisualInfo* vis = glXChooseVisual(dpy, DefaultScreen(dpy), (doubleBuffer)? dblBuf : snglBuf);
  GLXContext glxContext = glXCreateContext(dpy, vis, 0, GL_TRUE);
  Colormap cmap = XCreateColormap(dpy, RootWindow(dpy, vis->screen), vis->visual, AllocNone);
  XSetWindowAttributes winAttrib;
  winAttrib.colormap = cmap;
  winAttrib.border_pixel = 0;
  winAttrib.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | PointerMotionMask;
  Screen* screen = ScreenOfDisplay(dpy, 0);
  unsigned long vMask = CWBorderPixel | CWColormap | CWEventMask;

  if ( fullscreen ) {
    winAttrib.override_redirect = true;
    winWidth = screen->width;
    winHeight = screen->height;
    vMask = CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;
  }


  Window win = XCreateWindow(dpy, RootWindow(dpy, vis->screen), 0, 0, winWidth, winHeight, 0, vis->depth, InputOutput, vis->visual, vMask, &winAttrib);
  XSetStandardProperties(dpy, win, "FV200", "main", None, NULL, 0, NULL);
  XSelectInput(dpy, win, winAttrib.event_mask);


  //unsigned long vMask = CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;
  //Window win = XCreateWindow(dpy, RootWindow(dpy, vis->screen), 0, 0, screen->width, screen->height, 0, vis->depth, InputOutput, vis->visual, vMask, &winAttrib);
  //XSetStandardProperties(dpy, win, "FV200", "main", None, argv, argc, NULL);


  if ( fullscreen ) {
    /*
    Atom wm_state   = XInternAtom (dpy, "_NET_WM_STATE", true );
    Atom wm_fullscreen = XInternAtom (dpy, "_NET_WM_STATE_FULLSCREEN", true );
    XChangeProperty(dpy, win, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&wm_fullscreen, 1);

    Atom wm_motif = XInternAtom (dpy, "_MOTIF_WM_HINTS", true );
    Hints hints = {2, 0, 0, 0, 0};
    XChangeProperty(dpy, win, wm_motif, XA_ATOM, 32, PropModeReplace, (unsigned char *)&hints, 5);

    static char data[1] = {0};
    XColor dummyColor;
    Pixmap blankPixmap = XCreateBitmapFromData(dpy, RootWindow(dpy, vis->screen), data, 1, 1);
    Cursor blankcursor = XCreatePixmapCursor(dpy, blankPixmap, blankPixmap, &dummyColor, &dummyColor, 0, 0);
    XFreePixmap(dpy, blankPixmap);
    XDefineCursor(dpy, win, blankcursor);

    */

    screenWidth = screen->width;
    screenHeight = screen->height;
  } else {
    screenWidth = winWidth;
    screenHeight = winHeight;
  }


  glXMakeCurrent(dpy, win, glxContext);
  XMapWindow(dpy, win);
  XFlush(dpy);
  
  int32_t x11Fd = ConnectionNumber(dpy);
  XEvent event;
  fd_set inFds;
  struct timeval timeVal;


  printf("screenWidth : %d \n", screenWidth);
  printf("screenHeight : %d \n", screenHeight);

  pthread_mutex_t ffmpegLoadFrameMutexInstance;
  ffmpegLoadFrameMutex = &ffmpegLoadFrameMutexInstance;
  pthread_mutex_init(ffmpegLoadFrameMutex, NULL);
  
  pthread_mutex_t ffmpegBufferMutexInstance0;
  ffmpegBufferMutex[0] = &ffmpegBufferMutexInstance0; 
  pthread_mutex_init(ffmpegBufferMutex[0], NULL);
 
  pthread_mutex_t ffmpegBufferMutexInstance1;
  ffmpegBufferMutex[1] = &ffmpegBufferMutexInstance1;
  pthread_mutex_init(ffmpegBufferMutex[1], NULL);
  
  pthread_mutex_t ffmpegNewFrameMutexInstance0;
  ffmpegNewFrameMutex[0] = &ffmpegNewFrameMutexInstance0;
  pthread_mutex_init(ffmpegNewFrameMutex[0], NULL);
  
  pthread_mutex_t ffmpegNewFrameMutexInstance1;
  ffmpegNewFrameMutex[1] = &ffmpegNewFrameMutexInstance1;
  pthread_mutex_init(ffmpegNewFrameMutex[1], NULL);
  
  if (!ffmpegOpenVideoFile(&avFormatContext, &avCodecContext, &videoWidth, &videoHeight, &vidStrId, fileName)) {
    printf("Couldn't open video file. \n");
    return 1;
  }

  printf("videoWidth : %d \n", videoWidth);
  printf("videoHeight : %d \n", videoHeight);

  frameData = (unsigned char*)malloc(videoWidth * videoHeight * pixelFormat * sizeof(unsigned char));
  memset(frameData, 0xFF, videoWidth * videoHeight * pixelFormat * sizeof(unsigned char));

  
  pthread_t ffmpegBufferZeroThread;
  pthread_create(&ffmpegBufferZeroThread, NULL, ffmpegBufferZeroThreadHandler, "ffmpegBufferZeroThread");

  pthread_t ffmpegBufferOneThread;
  pthread_create(&ffmpegBufferOneThread, NULL, ffmpegBufferOneThreadHandler, "ffmpegBufferOneThread");

  usleep(9000);

  
  // OpenGL Init
  glewInit();

  //glEnable(GL_DEPTH_TEST);
  //glEnable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);
  //glEnable(GL_BLEND);
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  GLuint decimalZeroHandle;
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &decimalZeroHandle);
  glBindTexture(GL_TEXTURE_2D, decimalZeroHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_WIDTH, FONT_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, decimalZero);


  GLuint decimalOneHandle;
  glActiveTexture(GL_TEXTURE1);
  glGenTextures(1, &decimalOneHandle);
  glBindTexture(GL_TEXTURE_2D, decimalOneHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_WIDTH, FONT_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, decimalOne);


  GLuint decimalTwoHandle;
  glActiveTexture(GL_TEXTURE2);
  glGenTextures(1, &decimalTwoHandle);
  glBindTexture(GL_TEXTURE_2D, decimalTwoHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_WIDTH, FONT_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, decimalTwo);


  GLuint decimalThreeHandle;
  glActiveTexture(GL_TEXTURE3);
  glGenTextures(1, &decimalThreeHandle);
  glBindTexture(GL_TEXTURE_2D, decimalThreeHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_WIDTH, FONT_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, decimalThree);


  GLuint decimalFourHandle;
  glActiveTexture(GL_TEXTURE4);
  glGenTextures(1, &decimalFourHandle);
  glBindTexture(GL_TEXTURE_2D, decimalFourHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_WIDTH, FONT_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, decimalFour);


  GLuint decimalFiveHandle;
  glActiveTexture(GL_TEXTURE5);
  glGenTextures(1, &decimalFiveHandle);
  glBindTexture(GL_TEXTURE_2D, decimalFiveHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_WIDTH, FONT_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, decimalFive);


  GLuint decimalSixHandle;
  glActiveTexture(GL_TEXTURE6);
  glGenTextures(1, &decimalSixHandle);
  glBindTexture(GL_TEXTURE_2D, decimalSixHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_WIDTH, FONT_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, decimalSix);


  GLuint decimalSevenHandle;
  glActiveTexture(GL_TEXTURE7);
  glGenTextures(1, &decimalSevenHandle);
  glBindTexture(GL_TEXTURE_2D, decimalSevenHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_WIDTH, FONT_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, decimalSeven);


  GLuint decimalEightHandle;
  glActiveTexture(GL_TEXTURE8);
  glGenTextures(1, &decimalEightHandle);
  glBindTexture(GL_TEXTURE_2D, decimalEightHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_WIDTH, FONT_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, decimalEight);


  GLuint decimalNineHandle;
  glActiveTexture(GL_TEXTURE9);
  glGenTextures(1, &decimalNineHandle);
  glBindTexture(GL_TEXTURE_2D, decimalNineHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_WIDTH, FONT_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, decimalNine);


  GLuint decimalDotHandle;
  glActiveTexture(GL_TEXTURE10);
  glGenTextures(1, &decimalDotHandle);
  glBindTexture(GL_TEXTURE_2D, decimalDotHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_WIDTH, FONT_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, decimalDot);


  GLuint texLumaHandle;
  glActiveTexture(GL_TEXTURE11);
  glGenTextures(1, &texLumaHandle);
  glBindTexture(GL_TEXTURE_2D, texLumaHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, videoWidth, videoHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);


  GLuint texChromaBlueHandle;
  glActiveTexture(GL_TEXTURE12);
  glGenTextures(1, &texChromaBlueHandle);
  glBindTexture(GL_TEXTURE_2D, texChromaBlueHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, videoWidth / 2, videoHeight / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);


  GLuint texChromaRedHandle;
  glActiveTexture(GL_TEXTURE13);
  glGenTextures(1, &texChromaRedHandle);
  glBindTexture(GL_TEXTURE_2D, texChromaRedHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, videoWidth / 2, videoHeight / 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);


  GLuint texGstFrameHandle;
  glActiveTexture(GL_TEXTURE15);
  glGenTextures(1, &texGstFrameHandle);
  glBindTexture(GL_TEXTURE_2D, texGstFrameHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, videoWidth, videoHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, frameData);
  //glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, videoWidth, videoHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

  const char* vertexShaderCode =
    "#version 120\n"
    "\n"
    "attribute vec4 vertPosition;\n"
    "attribute vec4 vertColor;\n"
    "attribute vec2 vertTexCoord;\n"
    "attribute float vertTexIndex;\n"
    "\n"
    "varying vec4 vertColorVar;\n"
    "varying vec2 vertTexCoordVar;\n"
    "varying float vertTexIndexVar;\n"
    "\n"
    "uniform mat4 modelMatrix;\n"
    "uniform mat4 viewMatrix;\n"
    "uniform mat4 projectionMatrix;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vertPosition;\n"
    "    vertColorVar = vertColor;\n"
    "    vertTexCoordVar = vertTexCoord;\n"
    "    vertTexIndexVar = vertTexIndex;\n"
    "}\n"
    "\n";

  const char* fragmentShaderCode =
    "#version 120\n"
    "\n"
    "uniform sampler2D selectTex[16];\n"
    "\n"
    "varying vec4 vertColorVar;\n"
    "varying vec2 vertTexCoordVar;\n"
    "varying float vertTexIndexVar;\n"
    "uniform int unitNumberTex;\n"
    "uniform int decimalNumberTex;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    int vertTexIndexInt = int( vertTexIndexVar );\n"
    "    if ( vertTexIndexInt < 0 ) {\n"
    "        gl_FragColor = vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 0 ) {\n"
    "        gl_FragColor = vec4(1.0, 1.0, 1.0, texture2D( selectTex[0], vertTexCoordVar ).a) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 1 ) {\n"
    "        gl_FragColor = texture2D( selectTex[1], vertTexCoordVar ) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 2 ) {\n"
    "        gl_FragColor = texture2D( selectTex[2], vertTexCoordVar ) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 3 ) {\n"
    "        gl_FragColor = texture2D( selectTex[3], vertTexCoordVar ) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 4 ) {\n"
    "        gl_FragColor = texture2D( selectTex[4], vertTexCoordVar ) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 5 ) {\n"
    "        gl_FragColor = texture2D( selectTex[5], vertTexCoordVar ) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 6 ) {\n"
    "        gl_FragColor = texture2D( selectTex[6], vertTexCoordVar ) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 7 ) {\n"
    "        gl_FragColor = texture2D( selectTex[7], vertTexCoordVar ) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 8 ) {\n"
    "        gl_FragColor = texture2D( selectTex[8], vertTexCoordVar ) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 9 ) {\n"
    "        gl_FragColor = texture2D( selectTex[9], vertTexCoordVar ) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 10 ) {\n"
    "        gl_FragColor = texture2D( selectTex[10], vertTexCoordVar ) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 11 ) {\n"

    "        float r, g, b, y, u, v;\n"
    "        y = 1.1643 * (texture2D( selectTex[11], vertTexCoordVar ).g - 0.0625 );\n"
    "        v = texture2D( selectTex[12], vertTexCoordVar ).r - 0.5;\n"
    "        u = texture2D( selectTex[13], vertTexCoordVar ).r - 0.5;\n"

    "        r = y + 1.5958 * v;\n"
    "        g = y - 0.39173 * u - 0.81290 * v;\n"
    "        b = y + 2.017 * u;\n"

    "        gl_FragColor = vec4(r, g, b, 1.0);\n"

    "    } else if ( vertTexIndexInt == 12 ) {\n"
    "        gl_FragColor = texture2D( selectTex[12], vertTexCoordVar ) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 13 ) {\n"
    "        gl_FragColor = texture2D( selectTex[13], vertTexCoordVar ) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 14 ) {\n"

    "        if ( unitNumberTex < 0 ) {\n"
    "            gl_FragColor = vertColorVar;\n"
    "        } else if ( unitNumberTex == 0 ) {\n"
    "            gl_FragColor = vec4( texture2D( selectTex[0], vertTexCoordVar ).a ) * vertColorVar;\n"
    "        } else if ( unitNumberTex == 1 ) {\n"
    "            gl_FragColor = vec4( texture2D( selectTex[1], vertTexCoordVar ).a ) * vertColorVar;\n"
    "        } else if ( unitNumberTex == 2 ) {\n"
    "            gl_FragColor = vec4( texture2D( selectTex[2], vertTexCoordVar ).a ) * vertColorVar;\n"
    "        } else if ( unitNumberTex == 3 ) {\n"
    "            gl_FragColor = vec4( texture2D( selectTex[3], vertTexCoordVar ).a ) * vertColorVar;\n"
    "        } else if ( unitNumberTex == 4 ) {\n"
    "            gl_FragColor = vec4( texture2D( selectTex[4], vertTexCoordVar ).a ) * vertColorVar;\n"
    "        } else if ( unitNumberTex == 5 ) {\n"
    "            gl_FragColor = vec4( texture2D( selectTex[5], vertTexCoordVar ).a ) * vertColorVar;\n"
    "        } else if ( unitNumberTex == 6 ) {\n"
    "            gl_FragColor = vec4( texture2D( selectTex[6], vertTexCoordVar ).a ) * vertColorVar;\n"
    "        } else if ( unitNumberTex == 7 ) {\n"
    "            gl_FragColor = vec4( texture2D( selectTex[7], vertTexCoordVar ).a ) * vertColorVar;\n"
    "        } else if ( unitNumberTex == 8 ) {\n"
    "            gl_FragColor = vec4( texture2D( selectTex[8], vertTexCoordVar ).a ) * vertColorVar;\n"
    "        } else if ( unitNumberTex == 9 ) {\n"
    "            gl_FragColor = vec4( texture2D( selectTex[9], vertTexCoordVar ).a ) * vertColorVar;\n"
    "        }\n"
    
    
    //    "        gl_FragColor = texture2D( selectTex[unitNumberTex], vertTexCoordVar ) * vertColorVar;\n"
    "    } else if ( vertTexIndexInt == 15 ) {\n"
    "        gl_FragColor = texture2D( selectTex[15], vertTexCoordVar ) * vertColorVar;\n"
    //"        gl_FragColor = texture2D( selectTex[decimalNumberTex], vertTexCoordVar ) * vertColorVar;\n"
    //    "    } else if ( vertTexIndexInt == 16 ) {\n"
    //    "        gl_FragColor = texture2D( selectTex[16], vertTexCoordVar ) * vertColorVar;\n"
    "    }\n"
    "}\n"
    "\n";

  GLuint programId = CreateShader(vertexShaderCode, fragmentShaderCode);
  glUseProgram(programId);

  GLfloat projectionMatrix[4][4] =
    {
      { +1.0f, +0.0f, +0.0f, +0.0f },
      { +0.0f, +1.0f, +0.0f, +0.0f },
      { +0.0f, +0.0f, -1.0f, +0.0f },
      { -0.0f, -0.0f, -0.0f, +1.0f }
    };

  GLfloat viewMatrix[4][4] =
    {
      { +1.0f, +0.0f, +0.0f, +0.0f },
      { +0.0f, +1.0f, +0.0f, +0.0f },
      { +0.0f, +0.0f, +1.0f, +0.0f },
      { +0.0f, +0.0f, +0.0f, +1.0f }
    };

  GLfloat modelMatrix[4][4] =
    {
      { +1.0f, +0.0f, +0.0f, +0.0f },
      { +0.0f, +1.0f, +0.0f, +0.0f },
      { +0.0f, +0.0f, +1.0f, +0.0f },
      { +0.0f, +0.0f, +0.0f, +1.0f }
    };

  glUniformMatrix4fv(glGetUniformLocation(programId, "modelMatrix"), 1, GL_FALSE, &modelMatrix[0][0]);
  glUniformMatrix4fv(glGetUniformLocation(programId, "viewMatrix"), 1, GL_FALSE, &viewMatrix[0][0]);
  glUniformMatrix4fv(glGetUniformLocation(programId, "projectionMatrix"), 1, GL_FALSE, &projectionMatrix[0][0]);

  int frameCount = 0;
  int32_t samplers[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
  glUniform1iv(glGetUniformLocation(programId, "selectTex"), sizeof(samplers) / sizeof(samplers[0]), samplers);
  glUniform1i(glGetUniformLocation(programId, "unitNumberTex"), frameCount);
  //glUniform1i(glGetUniformLocation(programId, "decimalNumberTex"), frameCount);

  float winAspectRatio = (float)screenWidth / (float)screenHeight;
  float srcAspectRatio = (float)videoWidth / (float)videoHeight;

  float videoPoxX1 = -1.0f;
  float videoPoxX2 = +1.0f;
  float videoPoxY1 = +1.0f;
  float videoPoxY2 = -1.0f;

  if ( srcAspectRatio > winAspectRatio) {
    videoPoxX1 = -1.0f;
    videoPoxX2 = +1.0f;
    videoPoxY1 = +1.0f * winAspectRatio / srcAspectRatio; // ( (float)videoHeight / (float)videoWidth );
    videoPoxY2 = -1.0f * winAspectRatio / srcAspectRatio; // * ( (float)videoHeight / (float)videoWidth );
  } else if (srcAspectRatio < winAspectRatio) {
    videoPoxX1 = -1.0f * srcAspectRatio / winAspectRatio; // ( (float)videoWidth / (float)videoHeight );
    videoPoxX2 = +1.0f * srcAspectRatio / winAspectRatio; // ( (float)videoWidth / (float)videoHeight );
    videoPoxY1 = +1.0f;
    videoPoxY2 = -1.0f;
  }

  
  Vector2 videoSurfCoordX1Y1 = {videoPoxX1, videoPoxY1};
  Vector2 videoSurfCoordX1Y2 = {videoPoxX1, videoPoxY2};
  Vector2 videoSurfCoordX2Y2 = {videoPoxX2, videoPoxY2};
  Vector2 videoSurfCoordX2Y1 = {videoPoxX2, videoPoxY1};
  TexCoord videoSurfTexCoord = {videoSurfCoordX1Y1, videoSurfCoordX1Y2, videoSurfCoordX2Y2, videoSurfCoordX2Y1};
  unsigned int videoVertexArrayId = generateVertexArrayObject(programId, 11, 0.0f, &videoSurfTexCoord, NULL);

  float unitPoxX1 = ( ( -1.0f * (float)screenWidth ) + 2.0f * FONT_WIDTH ) / 1.0f;
  float unitPoxX2 = ( ( -1.0f * (float)screenWidth ) + 4.0f * FONT_WIDTH ) / 1.0f;
  float unitPoxY1 = ( ( +1.0f * (float)screenHeight ) - 0.0f * FONT_HEIGHT ) / 1.0f;
  float unitPoxY2 = ( ( +1.0f * (float)screenHeight ) - 2.0f * FONT_HEIGHT ) / 1.0f;
  Vector2 unitSurfCoordX1Y1 = {unitPoxX1 / (float)screenWidth, unitPoxY1 / (float)screenHeight};
  Vector2 unitSurfCoordX1Y2 = {unitPoxX1 / (float)screenWidth, unitPoxY2 / (float)screenHeight};
  Vector2 unitSurfCoordX2Y2 = {unitPoxX2 / (float)screenWidth, unitPoxY2 / (float)screenHeight};
  Vector2 unitSurfCoordX2Y1 = {unitPoxX2 / (float)screenWidth, unitPoxY1 / (float)screenHeight};
  TexCoord unitSurfTexCoord = {unitSurfCoordX1Y1, unitSurfCoordX1Y2, unitSurfCoordX2Y2, unitSurfCoordX2Y1};
  unsigned int unitNumberVertexArrayId = generateVertexArrayObject(programId, 14, 0.1f, &unitSurfTexCoord, NULL);

  float decimalPoxX1 = ( ( -1.0f * (float)screenWidth ) + 0.0f * FONT_WIDTH ) / 1.0f;
  float decimalPoxX2 = ( ( -1.0f * (float)screenWidth ) + 2.0f * FONT_WIDTH ) / 1.0f;
  float decimalPoxY1 = ( ( +1.0f * (float)screenHeight ) - 0.0f * FONT_HEIGHT ) / 1.0f;
  float decimalPoxY2 = ( ( +1.0f * (float)screenHeight ) - 2.0f * FONT_HEIGHT ) / 1.0f;
  Vector2 decimalSurfCoordX1Y1 = {decimalPoxX1 / (float)screenWidth, decimalPoxY1 / (float)screenHeight};
  Vector2 decimalSurfCoordX1Y2 = {decimalPoxX1 / (float)screenWidth, decimalPoxY2 / (float)screenHeight};
  Vector2 decimalSurfCoordX2Y2 = {decimalPoxX2 / (float)screenWidth, decimalPoxY2 / (float)screenHeight};
  Vector2 decimalSurfCoordX2Y1 = {decimalPoxX2 / (float)screenWidth, decimalPoxY1 / (float)screenHeight};
  TexCoord decimalSurfTexCoord = {decimalSurfCoordX1Y1, decimalSurfCoordX1Y2, decimalSurfCoordX2Y2, decimalSurfCoordX2Y1};
  unsigned int decimalNumberVertexArrayId = generateVertexArrayObject(programId, 15, 0.1f, &decimalSurfTexCoord, NULL);

  while (!exitLoop) {
    FD_ZERO(&inFds);
    FD_SET(x11Fd, &inFds);
    timeVal.tv_usec = 100;
    timeVal.tv_sec = 0;
    select(x11Fd + 1, &inFds, NULL, NULL, &timeVal);

    while( XPending(dpy) ) {
      XNextEvent(dpy, &event);
    }

    if(event.type == KeyPress) {
      KeySym keysym;
      char buffer[1];
      if( (XLookupString((XKeyEvent*)&event, buffer, 1, &keysym, NULL) == 1) && (keysym == (KeySym)XK_Escape) ) {
	exitLoop = GL_TRUE;
      }
    }


    //printf("gstDecoderNewFramenewFrameFlag[ffmpegAvFrameIdx] \n");
    if ( true /* newFrameFlag[ffmpegAvFrameIdx] */ ) {
      //printf("gstDecoderNewFramenewFrameFlag[%X] : TRUE \n", ffmpegAvFrameIdx);
      pthread_mutex_lock(ffmpegBufferMutex[ffmpegAvFrameIdx]);
      
      glActiveTexture(GL_TEXTURE11);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, videoWidth, videoHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, avFrame[ffmpegAvFrameIdx]->data[0]);
      
      glActiveTexture(GL_TEXTURE12);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, videoWidth / 2, videoHeight / 2, 0, GL_RED, GL_UNSIGNED_BYTE, avFrame[ffmpegAvFrameIdx]->data[2]);

      glActiveTexture(GL_TEXTURE13);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, videoWidth / 2, videoHeight / 2, 0, GL_RED, GL_UNSIGNED_BYTE, avFrame[ffmpegAvFrameIdx]->data[1]);

      //newFrameFlag[ffmpegAvFrameIdx] = false;
      pthread_mutex_unlock(ffmpegNewFrameMutex[ffmpegAvFrameIdx]);
      pthread_mutex_unlock(ffmpegBufferMutex[ffmpegAvFrameIdx]);
      ffmpegAvFrameIdx = !ffmpegAvFrameIdx;

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glUniform1i(glGetUniformLocation(programId, "unitNumberTex"), frameCount % 10);
      //glUniform1i(glGetUniformLocation(programId, "decimalNumberTex"), ( (frameCount - (frameCount % 10) ) / 10) % 10);

      glBindVertexArray(videoVertexArrayId);
      glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);

      glBindVertexArray(unitNumberVertexArrayId);
      glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);

      //glBindVertexArray(decimalNumberVertexArrayId);
      //glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, 0);
      //glBindVertexArray(0);

      frameCount += 1;


      if (doubleBuffer) {
	glXSwapBuffers(dpy, win);
      } else {
	glFlush();
      }
    }
  }

  ffmpegThreadLive = false;
  pthread_mutex_unlock(ffmpegNewFrameMutex[0]);
  usleep(9000);
  pthread_mutex_unlock(ffmpegBufferMutex[0]);
  usleep(9000);
  pthread_mutex_unlock(ffmpegLoadFrameMutex);
  usleep(9000);
  
  pthread_mutex_unlock(ffmpegNewFrameMutex[1]);    
  usleep(9000);
  pthread_mutex_unlock(ffmpegBufferMutex[1]);
  usleep(9000);
  pthread_mutex_unlock(ffmpegLoadFrameMutex);
  usleep(9000);
  
  ffmpegCloseVideoFile(avFormatContext, avCodecContext);
  
  glUseProgram(0);
  XCloseDisplay(dpy);

}
