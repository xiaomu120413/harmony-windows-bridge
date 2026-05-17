#pragma once

#include "bridge_types.h"
#include "string_utils.h"

#include <cstdint>
#include <string>

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <native_image/native_image.h>
#include <native_window/external_window.h>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

namespace rdp_bridge {
class GpuAvc444SurfacePool {
public:
    ~GpuAvc444SurfacePool()
    {
        Destroy();
    }

    void Destroy()
    {
        if (display_ != EGL_NO_DISPLAY) {
            if (context_ != EGL_NO_CONTEXT && pbufferSurface_ != EGL_NO_SURFACE) {
                eglMakeCurrent(display_, pbufferSurface_, pbufferSurface_, context_);
                DestroyDecodeSurface(luma_);
                DestroyDecodeSurface(chroma_);
                eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            } else {
                DestroyDecodeSurface(luma_);
                DestroyDecodeSurface(chroma_);
            }

            if (context_ != EGL_NO_CONTEXT) {
                eglDestroyContext(display_, context_);
            }
            if (pbufferSurface_ != EGL_NO_SURFACE) {
                eglDestroySurface(display_, pbufferSurface_);
            }
            eglTerminate(display_);
        }

        display_ = EGL_NO_DISPLAY;
        config_ = nullptr;
        context_ = EGL_NO_CONTEXT;
        pbufferSurface_ = EGL_NO_SURFACE;
        width_ = 0;
        height_ = 0;
    }

    bool Ensure(uint32_t width, uint32_t height, Avc444SurfaceTargets& targets, std::string& error)
    {
        if (width == 0 || height == 0) {
            error = "AVC444 decode surface size is invalid";
            return false;
        }
        if (!EnsureContext(error)) {
            return false;
        }
        if (!eglMakeCurrent(display_, pbufferSurface_, pbufferSurface_, context_)) {
            error = "AVC444 pbuffer make current failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
            Destroy();
            return false;
        }

        if (width_ != width || height_ != height || luma_.window == nullptr || chroma_.window == nullptr) {
            DestroyDecodeSurface(luma_);
            DestroyDecodeSurface(chroma_);
            width_ = 0;
            height_ = 0;
            if (!CreateDecodeSurface("luma", luma_, error) ||
                !CreateDecodeSurface("chroma", chroma_, error)) {
                eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                Destroy();
                return false;
            }
            width_ = width;
            height_ = height;
        }

        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        targets.lumaWindow = luma_.window;
        targets.chromaWindow = chroma_.window;
        targets.width = width_;
        targets.height = height_;
        targets.lumaTexture = luma_.texture;
        targets.chromaTexture = chroma_.texture;
        targets.lumaSurfaceId = luma_.surfaceId;
        targets.chromaSurfaceId = chroma_.surfaceId;
        return true;
    }

private:
    struct DecodeSurface {
        GLuint texture = 0;
        OH_NativeImage* image = nullptr;
        OHNativeWindow* window = nullptr;
        uint64_t surfaceId = 0;
    };

    bool EnsureContext(std::string& error)
    {
        if (display_ != EGL_NO_DISPLAY && context_ != EGL_NO_CONTEXT &&
            pbufferSurface_ != EGL_NO_SURFACE) {
            return true;
        }

        Destroy();
        display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display_ == EGL_NO_DISPLAY) {
            error = "AVC444 EGL get display failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
            return false;
        }
        if (!eglInitialize(display_, nullptr, nullptr)) {
            error = "AVC444 EGL initialize failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
            Destroy();
            return false;
        }
        if (!eglBindAPI(EGL_OPENGL_ES_API)) {
            error = "AVC444 EGL bind GLES API failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
            Destroy();
            return false;
        }

        const EGLint configAttribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT | EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_NONE,
        };
        EGLint configCount = 0;
        if (!eglChooseConfig(display_, configAttribs, &config_, 1, &configCount) || configCount <= 0) {
            error = "AVC444 EGL choose config failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
            Destroy();
            return false;
        }

        const EGLint pbufferAttribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
        pbufferSurface_ = eglCreatePbufferSurface(display_, config_, pbufferAttribs);
        if (pbufferSurface_ == EGL_NO_SURFACE) {
            error = "AVC444 EGL create pbuffer failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
            Destroy();
            return false;
        }

        const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, contextAttribs);
        if (context_ == EGL_NO_CONTEXT) {
            error = "AVC444 EGL create context failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
            Destroy();
            return false;
        }
        return true;
    }

    bool CreateDecodeSurface(const char* name, DecodeSurface& surface, std::string& error)
    {
        glGenTextures(1, &surface.texture);
        if (surface.texture == 0) {
            error = std::string("AVC444 ") + name + " texture allocation failed";
            return false;
        }

        glBindTexture(GL_TEXTURE_EXTERNAL_OES, surface.texture);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        const GLenum glError = glGetError();
        if (glError != GL_NO_ERROR) {
            error = std::string("AVC444 ") + name +
                " external texture setup failed: " + Hex32(static_cast<uint32_t>(glError));
            DestroyDecodeSurface(surface);
            return false;
        }

        surface.image = OH_NativeImage_Create(surface.texture, GL_TEXTURE_EXTERNAL_OES);
        if (surface.image == nullptr) {
            error = std::string("AVC444 ") + name + " NativeImage create failed";
            DestroyDecodeSurface(surface);
            return false;
        }

        surface.window = OH_NativeImage_AcquireNativeWindow(surface.image);
        if (surface.window == nullptr) {
            error = std::string("AVC444 ") + name + " NativeImage window acquire failed";
            DestroyDecodeSurface(surface);
            return false;
        }

        (void)OH_NativeImage_GetSurfaceId(surface.image, &surface.surfaceId);
        return true;
    }

    static void DestroyDecodeSurface(DecodeSurface& surface)
    {
        if (surface.image != nullptr) {
            OH_NativeImage_Destroy(&surface.image);
        }
        if (surface.texture != 0) {
            GLuint texture = surface.texture;
            glDeleteTextures(1, &texture);
        }
        surface.texture = 0;
        surface.window = nullptr;
        surface.surfaceId = 0;
    }

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLConfig config_ = nullptr;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLSurface pbufferSurface_ = EGL_NO_SURFACE;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    DecodeSurface luma_;
    DecodeSurface chroma_;
};
} // namespace rdp_bridge
