// OpenGL ES 2.0 code

#include <nativehelper/jni.h>
#define LOG_TAG "GLImageJNI egl_image.cpp"
#include <utils/Log.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "SkBitmap.h"
#include <ui/GraphicBuffer.h>

namespace android {
static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    ALOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        ALOGI("after %s() glError (0x%x)\n", op, error);
    }
}

static void checkEglError(const char* op, EGLBoolean returnVal = EGL_TRUE) {
    if (returnVal != EGL_TRUE) {
        ALOGI("%s() returned %d\n", op, returnVal);
    }

    for (EGLint error = eglGetError(); error != EGL_SUCCESS; error
            = eglGetError()) {
        ALOGI("after %s() eglError (0x%x)\n", op, error);
    }
}

static const char gVertexShader[] = "attribute vec4 vPosition;\n"
    "attribute vec2 aTextureCoord;\n"
    "varying vec2 vTextureCoord;\n"
    "void main() {\n"
    "  gl_Position = vPosition;\n"
    "  vTextureCoord = aTextureCoord;\n"
    "}\n";
/*
static const char gFragmentShader[] = "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
    "}\n";
*/

static const char gFragmentShader[] = "#extension GL_OES_EGL_image_external : require\n"
    "precision mediump float;\n"
    "uniform samplerExternalOES baseSampler;\n"
    "varying vec2 vTextureCoord;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(baseSampler, vTextureCoord);\n"
    "}\n";


GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    ALOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    ALOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram;
GLuint gvPositionHandle;
GLuint gvTextureHandle;

sp<GraphicBuffer> texBuffer;

static GLuint texId = 0;
int texWidth;
int texHeight;
void * vaddr = NULL;

void SetupTexture(int w, int h ){
    texWidth = w;
    texHeight = h;
    texBuffer = new GraphicBuffer(w, h, HAL_PIXEL_FORMAT_RGBA_8888,
            GraphicBuffer::USAGE_HW_TEXTURE |
                    GraphicBuffer::USAGE_SW_WRITE_RARELY);
    EGLClientBuffer clientBuffer = (EGLClientBuffer)texBuffer->getNativeBuffer();
    EGLImageKHR img = eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
            clientBuffer, 0);
    checkEglError("eglCreateImageKHR");
    if (img == EGL_NO_IMAGE_KHR) {
        return ;
    }

    glGenTextures(1, &texId);
    checkGlError("glGenTextures");
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texId);
    checkGlError("glBindTexture");
    glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)img);
    checkGlError("glEGLImageTargetTexture2DOES");

    ALOGI("setupGraphics finish(%d, %d)", w, h);
}

bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    ALOGI("setupGraphics(%d, %d)", w, h);
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        ALOGE("Could not create program.");
        return false;
    }
    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");
    ALOGI("glGetAttribLocation(\"vPosition\") = %d\n",
            gvPositionHandle);

    gvTextureHandle = glGetAttribLocation(gProgram, "aTextureCoord");
    checkGlError("glGetAttribLocation aTextureCoord");
        ALOGI("glGetAttribLocation(\"vTextureCoord\") = %d\n",
                gvTextureHandle);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");

    SetupTexture(w,h);

    return true;
}

const GLfloat gTriangleVertices[] = { -1.0f, 1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,1.0f, 1.0f};

const GLfloat gTriangleTexture[] = { 0.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 0.0f };

void renderFrame(SkBitmap bitmap) {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");

    glVertexAttribPointer(gvTextureHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleTexture);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvTextureHandle);
    checkGlError("glEnableVertexAttribArray");


    if (texBuffer != NULL) {
        texBuffer->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, &vaddr);
        ALOGI("texBuffer->lock() ");
                   /*char src[] = {255,255,0,255,255,0,0,255,255,0,0,255,255,255,0,255,255,0,0,255,255,0,0,255};
                    char* dst = (char*)vaddr;
                   for (int y = 0; y < 1080; ++y) {
                       memcpy(dst+y*1920*4, src, 4*6);
                   }*/
        bitmap.lockPixels();
        int bpp = bitmap.bytesPerPixel();
        int w = bitmap.width();
        int h = bitmap.height();
         ALOGI("bitmap(w %d, h %d bpp %d ) ",w,h,bpp);
        int wsize = w * bpp;
        int twsize = texWidth * bpp;
        if (texWidth > w) {
            // glTexSubImage2D
            char * dst = (char *)vaddr;
            char * src = (char *)bitmap.getPixels();
            for (int y = 0; y < h; ++y ) {
                memcpy(dst, src, wsize);
                dst += twsize;
                src += wsize;
            }
        } else if (texWidth < w){
           // glTexSubImage2D
            char * dst = (char *)vaddr;
            char * src = (char *)bitmap.getPixels();
           for (int y = 0; y < texHeight; ++y, dst += twsize, src += wsize) {
               memcpy(dst, src, twsize);
           }
        } else{
            // glTexImage2D
            memcpy(vaddr, bitmap.getPixels(), wsize * h);
        }
        bitmap.lockPixels();
        texBuffer->unlock();

    }


    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    checkGlError("glDrawArrays");
}


extern "C" {
    JNIEXPORT void JNICALL Java_com_wei_collections_GLImageLib_nativeClassInit(JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_wei_collections_GLImageLib_init(JNIEnv * env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_wei_collections_GLImageLib_draw(JNIEnv * env, jobject obj, jobject bitmap);
};

static jfieldID nativeBitmapID = 0;
JNIEXPORT void JNICALL Java_com_wei_collections_GLImageLib_nativeClassInit(JNIEnv * env, jobject obj)
{
        jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
        nativeBitmapID = env->GetFieldID(bitmapClass, "mNativeBitmap", "I");
}

JNIEXPORT void JNICALL Java_com_wei_collections_GLImageLib_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_wei_collections_GLImageLib_draw(JNIEnv * env, jobject obj, jobject jbitmap)
{
    SkBitmap const * nativeBitmap =
            (SkBitmap const *)env->GetIntField(jbitmap, nativeBitmapID);
    const SkBitmap& bitmap(*nativeBitmap);
    renderFrame(bitmap);
}

}