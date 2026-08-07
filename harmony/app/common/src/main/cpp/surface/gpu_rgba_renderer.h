#pragma once

#include "common/bridge_types.h"
#include "common/string_utils.h"

#include <algorithm>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <native_window/external_window.h>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

namespace rdp_bridge {
struct RenderViewport {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

class GpuRgbaRenderer {
public:
    ~GpuRgbaRenderer()
    {
        Destroy();
    }

    void Destroy()
    {
        if (display_ != EGL_NO_DISPLAY) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (context_ != EGL_NO_CONTEXT) {
                if (texture_ != 0) {
                    eglMakeCurrent(display_, surface_, surface_, context_);
                    glDeleteTextures(1, &texture_);
                    texture_ = 0;
                    if (program_ != 0) {
                        glDeleteProgram(program_);
                        program_ = 0;
                    }
                    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                }
                eglDestroyContext(display_, context_);
            }
            if (surface_ != EGL_NO_SURFACE) {
                eglDestroySurface(display_, surface_);
            }
            eglTerminate(display_);
        }

        display_ = EGL_NO_DISPLAY;
        context_ = EGL_NO_CONTEXT;
        surface_ = EGL_NO_SURFACE;
        config_ = nullptr;
        window_ = nullptr;
        width_ = 0;
        height_ = 0;
        textureWidth_ = 0;
        textureHeight_ = 0;
        positionAttrib_ = -1;
        texCoordAttrib_ = -1;
        textureUniform_ = -1;
    }

    bool Render(OHNativeWindow* nativeWindow, uint32_t targetWidth, uint32_t targetHeight,
        const RgbaFrame& frame, int32_t sourceStride, const RenderViewport& viewport,
        SurfacePaintResult& result)
    {
        if (!EnsureReady(nativeWindow, targetWidth, targetHeight, result)) {
            return false;
        }
        if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
            result.logs.push_back("EGL make current failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);

        if (!UploadTexture(frame, sourceStride, result)) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }

        glUseProgram(program_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glUniform1i(textureUniform_, 0);

        const GLfloat vertices[] = {
            -1.0F, -1.0F, 0.0F, 1.0F,
             1.0F, -1.0F, 1.0F, 1.0F,
            -1.0F,  1.0F, 0.0F, 0.0F,
             1.0F,  1.0F, 1.0F, 0.0F,
        };
        glVertexAttribPointer(static_cast<GLuint>(positionAttrib_), 2, GL_FLOAT, GL_FALSE,
            4 * sizeof(GLfloat), vertices);
        glEnableVertexAttribArray(static_cast<GLuint>(positionAttrib_));
        glVertexAttribPointer(static_cast<GLuint>(texCoordAttrib_), 2, GL_FLOAT, GL_FALSE,
            4 * sizeof(GLfloat), vertices + 2);
        glEnableVertexAttribArray(static_cast<GLuint>(texCoordAttrib_));

        const GLint viewportY = static_cast<GLint>(targetHeight - viewport.y - viewport.height);
        glViewport(static_cast<GLint>(viewport.x), viewportY, static_cast<GLsizei>(viewport.width),
            static_cast<GLsizei>(viewport.height));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        const GLenum glError = glGetError();
        if (glError != GL_NO_ERROR) {
            result.logs.push_back("GLES draw failed: " + Hex32(static_cast<uint32_t>(glError)));
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }
        if (!eglSwapBuffers(display_, surface_)) {
            result.logs.push_back("EGL swap buffers failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        result.ok = true;
        result.partial = false;
        const std::string frameLabel = frame.label.empty() ? "frame" : frame.label;
        result.message = "GLES texture rendered: " + frameLabel + " " +
            std::to_string(viewport.width) + "x" + std::to_string(viewport.height) +
            " viewport=" + std::to_string(viewport.x) + "," + std::to_string(viewport.y) +
            " source=" + std::to_string(frame.width) + "x" + std::to_string(frame.height) +
            " upload=" + (uploadMode_.empty() ?
                (usingStagingBuffer_ ? "staged" : "direct") : uploadMode_);
        result.logs.push_back(result.message);
        return true;
    }

private:
    bool EnsureReady(OHNativeWindow* nativeWindow, uint32_t targetWidth, uint32_t targetHeight,
        SurfacePaintResult& result)
    {
        if (nativeWindow == nullptr || targetWidth == 0 || targetHeight == 0) {
            result.logs.push_back("GLES target NativeWindow is invalid");
            return false;
        }
        if (display_ != EGL_NO_DISPLAY && window_ == nativeWindow &&
            width_ == targetWidth && height_ == targetHeight) {
            return true;
        }

        Destroy();
        display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display_ == EGL_NO_DISPLAY) {
            result.logs.push_back("EGL get display failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
            return false;
        }

        if (!eglInitialize(display_, nullptr, nullptr)) {
            result.logs.push_back("EGL initialize failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }
        if (!eglBindAPI(EGL_OPENGL_ES_API)) {
            result.logs.push_back("EGL bind GLES API failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }

        const EGLint configAttribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_NONE,
        };
        EGLint configCount = 0;
        if (!eglChooseConfig(display_, configAttribs, &config_, 1, &configCount) || configCount <= 0) {
            result.logs.push_back("EGL choose config failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }

        surface_ = eglCreateWindowSurface(display_, config_,
            reinterpret_cast<EGLNativeWindowType>(nativeWindow), nullptr);
        if (surface_ == EGL_NO_SURFACE) {
            result.logs.push_back("EGL create window surface failed: " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }

        const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, contextAttribs);
        if (context_ == EGL_NO_CONTEXT) {
            result.logs.push_back("EGL create context failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }

        if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
            result.logs.push_back("EGL make current failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }
        if (!EnsureProgram(result)) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            Destroy();
            return false;
        }

        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const GLenum glError = glGetError();
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (glError != GL_NO_ERROR) {
            result.logs.push_back("GLES texture setup failed: " + Hex32(static_cast<uint32_t>(glError)));
            Destroy();
            return false;
        }

        window_ = nativeWindow;
        width_ = targetWidth;
        height_ = targetHeight;
        result.logs.push_back("GLES renderer initialized: " + std::to_string(width_) + "x" +
            std::to_string(height_));
        return true;
    }

    bool EnsureProgram(SurfacePaintResult& result)
    {
        if (program_ != 0) {
            return true;
        }

        static constexpr const char* vertexShaderSource =
            "attribute vec2 aPosition;\n"
            "attribute vec2 aTexCoord;\n"
            "varying vec2 vTexCoord;\n"
            "void main() {\n"
            "  gl_Position = vec4(aPosition, 0.0, 1.0);\n"
            "  vTexCoord = aTexCoord;\n"
            "}\n";
        static constexpr const char* fragmentShaderSource =
            "precision mediump float;\n"
            "varying vec2 vTexCoord;\n"
            "uniform sampler2D uTexture;\n"
            "void main() {\n"
            "  gl_FragColor = texture2D(uTexture, vTexCoord);\n"
            "}\n";

        const GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource, result);
        if (vertexShader == 0) {
            return false;
        }
        const GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource, result);
        if (fragmentShader == 0) {
            glDeleteShader(vertexShader);
            return false;
        }

        program_ = glCreateProgram();
        glAttachShader(program_, vertexShader);
        glAttachShader(program_, fragmentShader);
        glLinkProgram(program_);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        GLint linked = GL_FALSE;
        glGetProgramiv(program_, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            result.logs.push_back("GLES shader link failed: " + ReadProgramInfoLog(program_));
            glDeleteProgram(program_);
            program_ = 0;
            return false;
        }

        positionAttrib_ = glGetAttribLocation(program_, "aPosition");
        texCoordAttrib_ = glGetAttribLocation(program_, "aTexCoord");
        textureUniform_ = glGetUniformLocation(program_, "uTexture");
        if (positionAttrib_ < 0 || texCoordAttrib_ < 0 || textureUniform_ < 0) {
            result.logs.push_back("GLES shader bindings missing");
            glDeleteProgram(program_);
            program_ = 0;
            return false;
        }
        return true;
    }

    static GLuint CompileShader(GLenum type, const char* source, SurfacePaintResult& result)
    {
        const GLuint shader = glCreateShader(type);
        if (shader == 0) {
            result.logs.push_back("GLES create shader failed: " + Hex32(static_cast<uint32_t>(glGetError())));
            return 0;
        }
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_TRUE) {
            return shader;
        }

        result.logs.push_back("GLES shader compile failed type=" + std::to_string(type) +
            " glError=" + Hex32(static_cast<uint32_t>(glGetError())) +
            " log=" + ReadShaderInfoLog(shader));
        glDeleteShader(shader);
        return 0;
    }

    static std::string ReadShaderInfoLog(GLuint shader)
    {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        if (length <= 1) {
            return "no info log";
        }
        std::string log(static_cast<size_t>(length), '\0');
        GLsizei written = 0;
        glGetShaderInfoLog(shader, length, &written, log.data());
        log.resize(static_cast<size_t>(std::max<GLsizei>(0, written)));
        return log;
    }

    static std::string ReadProgramInfoLog(GLuint program)
    {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        if (length <= 1) {
            return "no info log";
        }
        std::string log(static_cast<size_t>(length), '\0');
        GLsizei written = 0;
        glGetProgramInfoLog(program, length, &written, log.data());
        log.resize(static_cast<size_t>(std::max<GLsizei>(0, written)));
        return log;
    }

    bool UploadTexture(const RgbaFrame& frame, int32_t sourceStride, SurfacePaintResult& result)
    {
        const size_t tightRowBytes = static_cast<size_t>(frame.width) * 4U;
        const uint8_t* upload = nullptr;
        usingStagingBuffer_ = false;
        uploadMode_ = "full-direct";

        glBindTexture(GL_TEXTURE_2D, texture_);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        const bool textureSizeChanged = textureWidth_ != frame.width || textureHeight_ != frame.height;
        DirtyFrameStats dirty = frame.dirty;
        const bool uploadDirty = !textureSizeChanged && CanUploadDirty(frame, dirty);
        if (!uploadDirty) {
            upload = PrepareFullUpload(frame, sourceStride, tightRowBytes);
        }
        if (textureSizeChanged) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(frame.width),
                static_cast<GLsizei>(frame.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, upload);
            textureWidth_ = frame.width;
            textureHeight_ = frame.height;
        } else if (uploadDirty) {
            UploadDirtyTexture(frame, sourceStride, dirty);
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(frame.width),
                static_cast<GLsizei>(frame.height), GL_RGBA, GL_UNSIGNED_BYTE, upload);
        }

        const GLenum glError = glGetError();
        if (glError == GL_NO_ERROR) {
            return true;
        }
        result.logs.push_back("GLES texture upload failed: " + Hex32(static_cast<uint32_t>(glError)));
        return false;
    }

    const uint8_t* PrepareFullUpload(const RgbaFrame& frame, int32_t sourceStride, size_t tightRowBytes)
    {
        if (sourceStride == static_cast<int32_t>(tightRowBytes)) {
            usingStagingBuffer_ = false;
            uploadMode_ = "full-direct";
            return frame.data;
        }

        const size_t required = tightRowBytes * frame.height;
        uploadBuffer_.resize(required);
        for (uint32_t y = 0; y < frame.height; ++y) {
            std::memcpy(uploadBuffer_.data() + tightRowBytes * y,
                frame.data + static_cast<int64_t>(sourceStride) * y, tightRowBytes);
        }
        usingStagingBuffer_ = true;
        uploadMode_ = "full-staged";
        return uploadBuffer_.data();
    }

    bool CanUploadDirty(const RgbaFrame& frame, DirtyFrameStats& dirty) const
    {
        constexpr uint32_t kMaxDirtyUploadAreaPermille = 850;
        if (!dirty.valid || dirty.width == 0 || dirty.height == 0 ||
            dirty.areaPermille > kMaxDirtyUploadAreaPermille) {
            return false;
        }
        if (dirty.x >= frame.width || dirty.y >= frame.height) {
            return false;
        }
        if (dirty.x + dirty.width > frame.width) {
            dirty.width = frame.width - dirty.x;
        }
        if (dirty.y + dirty.height > frame.height) {
            dirty.height = frame.height - dirty.y;
        }
        return dirty.width > 0 && dirty.height > 0;
    }

    void UploadDirtyTexture(const RgbaFrame& frame, int32_t sourceStride, const DirtyFrameStats& dirty)
    {
        const size_t dirtyRowBytes = static_cast<size_t>(dirty.width) * 4U;
        const uint8_t* upload = nullptr;
        if (dirty.x == 0 && dirty.width == frame.width &&
            sourceStride == static_cast<int32_t>(dirtyRowBytes)) {
            upload = frame.data + static_cast<size_t>(dirty.y) * dirtyRowBytes;
            usingStagingBuffer_ = false;
            uploadMode_ = "dirty-direct";
        } else {
            const size_t required = dirtyRowBytes * dirty.height;
            uploadBuffer_.resize(required);
            for (uint32_t y = 0; y < dirty.height; ++y) {
                const uint8_t* src = frame.data +
                    static_cast<int64_t>(dirty.y + y) * sourceStride +
                    static_cast<size_t>(dirty.x) * 4U;
                std::memcpy(uploadBuffer_.data() + dirtyRowBytes * y, src, dirtyRowBytes);
            }
            upload = uploadBuffer_.data();
            usingStagingBuffer_ = true;
            uploadMode_ = "dirty-staged";
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(dirty.x),
            static_cast<GLint>(dirty.y), static_cast<GLsizei>(dirty.width),
            static_cast<GLsizei>(dirty.height), GL_RGBA, GL_UNSIGNED_BYTE, upload);
    }

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLConfig config_ = nullptr;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLSurface surface_ = EGL_NO_SURFACE;
    OHNativeWindow* window_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    GLuint program_ = 0;
    GLuint texture_ = 0;
    uint32_t textureWidth_ = 0;
    uint32_t textureHeight_ = 0;
    GLint positionAttrib_ = -1;
    GLint texCoordAttrib_ = -1;
    GLint textureUniform_ = -1;
    bool usingStagingBuffer_ = false;
    std::string uploadMode_;
    std::vector<uint8_t> uploadBuffer_;
};


} // namespace rdp_bridge
