#include "surface/avc444_gpu_compositor_internal_types.h"
#include "surface/avc_gpu_common.h"
#include "common/string_utils.h"
#include "session/session_display_settings.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

namespace rdp_bridge {
namespace {

GLuint CompileShader(GLenum type, const char* source, std::vector<std::string>& logs)
{
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        logs.push_back("Mapped-plane GPU GLES create shader failed: " +
            Hex32(static_cast<uint32_t>(glGetError())));
        return 0;
    }
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string info;
    if (length > 1) {
        info.resize(static_cast<size_t>(length), '\0');
        GLsizei written = 0;
        glGetShaderInfoLog(shader, length, &written, info.data());
        info.resize(static_cast<size_t>(std::max<GLsizei>(0, written)));
    }
    logs.push_back("Mapped-plane GPU GLES shader compile failed type=" + std::to_string(type) +
        " log=" + (info.empty() ? "none" : info));
    glDeleteShader(shader);
    return 0;
}

GLuint LinkProgram(const char* vertexSource, const char* fragmentSource,
    std::vector<std::string>& logs)
{
    const GLuint vertex = CompileShader(GL_VERTEX_SHADER, vertexSource, logs);
    if (vertex == 0) {
        return 0;
    }
    const GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource, logs);
    if (fragment == 0) {
        glDeleteShader(vertex);
        return 0;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return program;
    }

    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string info;
    if (length > 1) {
        info.resize(static_cast<size_t>(length), '\0');
        GLsizei written = 0;
        glGetProgramInfoLog(program, length, &written, info.data());
        info.resize(static_cast<size_t>(std::max<GLsizei>(0, written)));
    }
    logs.push_back("Mapped-plane GPU GLES program link failed log=" +
        (info.empty() ? "none" : info));
    glDeleteProgram(program);
    return 0;
}

} // namespace

Avc444GpuRenderer::~Avc444GpuRenderer()
{
    Destroy();
}

void Avc444GpuRenderer::Destroy()
{
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, surface_, surface_, context_);
        DeleteTextures();
        if (copyYProgram_ != 0) {
            glDeleteProgram(copyYProgram_);
        }
        if (lumaUvProgram_ != 0) {
            glDeleteProgram(lumaUvProgram_);
        }
        if (chromaV1Program_ != 0) {
            glDeleteProgram(chromaV1Program_);
        }
        if (chromaV2Program_ != 0) {
            glDeleteProgram(chromaV2Program_);
        }
        if (presentProgram_ != 0) {
            glDeleteProgram(presentProgram_);
        }
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
        }
        eglTerminate(display_);
    }

    display_ = EGL_NO_DISPLAY;
    config_ = nullptr;
    surface_ = EGL_NO_SURFACE;
    context_ = EGL_NO_CONTEXT;
    window_ = nullptr;
    targetWidth_ = 0;
    targetHeight_ = 0;
    surfaceWidth_ = 0;
    surfaceHeight_ = 0;
    copyYProgram_ = 0;
    lumaUvProgram_ = 0;
    chromaV1Program_ = 0;
    chromaV2Program_ = 0;
    presentProgram_ = 0;
    hasLuma_ = false;
    hasChroma_ = false;
    lastFramebufferSample_ = "readback:disabled";
}

bool Avc444GpuRenderer::Ensure(OHNativeWindow* window, uint32_t targetWidth, uint32_t targetHeight,
    uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<std::string>& logs)
{
    if (surfaceWidth == 0 || surfaceHeight == 0 ||
        (window != nullptr && (targetWidth == 0 || targetHeight == 0))) {
        logs.push_back("Mapped-plane GPU renderer target invalid");
        return false;
    }

    const uint32_t effectiveTargetWidth = window == nullptr ? surfaceWidth : targetWidth;
    const uint32_t effectiveTargetHeight = window == nullptr ? surfaceHeight : targetHeight;

    if (display_ != EGL_NO_DISPLAY && surfaceWidth_ == surfaceWidth &&
        surfaceHeight_ == surfaceHeight && window == nullptr) {
        return true;
    }

    if (display_ != EGL_NO_DISPLAY && window_ == window &&
        targetWidth_ == effectiveTargetWidth && targetHeight_ == effectiveTargetHeight &&
        surfaceWidth_ == surfaceWidth && surfaceHeight_ == surfaceHeight) {
        return true;
    }

    if (display_ != EGL_NO_DISPLAY && surfaceWidth_ == surfaceWidth &&
        surfaceHeight_ == surfaceHeight && window != nullptr) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        surface_ = eglCreateWindowSurface(display_, config_,
            reinterpret_cast<EGLNativeWindowType>(window), nullptr);
        if (surface_ == EGL_NO_SURFACE) {
            logs.push_back("Mapped-plane GPU renderer attach window surface failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }
        window_ = window;
        targetWidth_ = effectiveTargetWidth;
        targetHeight_ = effectiveTargetHeight;
        logs.push_back("Mapped-plane GPU renderer attached XComponent window: target=" +
            std::to_string(targetWidth_) + "x" + std::to_string(targetHeight_) +
            " surface=" + std::to_string(surfaceWidth_) + "x" +
            std::to_string(surfaceHeight_));
        return true;
    }

    Destroy();
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) {
        logs.push_back("Mapped-plane GPU renderer eglGetDisplay failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        Destroy();
        return false;
    }
    if (!eglInitialize(display_, nullptr, nullptr)) {
        logs.push_back("Mapped-plane GPU renderer eglInitialize failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        Destroy();
        return false;
    }
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        logs.push_back("Mapped-plane GPU renderer eglBindAPI failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        Destroy();
        return false;
    }

    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE,
    };
    EGLint configCount = 0;
    if (!eglChooseConfig(display_, configAttribs, &config_, 1, &configCount) ||
        configCount <= 0) {
        logs.push_back("Mapped-plane GPU renderer eglChooseConfig failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        Destroy();
        return false;
    }

    if (window == nullptr) {
        const EGLint pbufferAttribs[] = {
            EGL_WIDTH, 1,
            EGL_HEIGHT, 1,
            EGL_NONE,
        };
        surface_ = eglCreatePbufferSurface(display_, config_, pbufferAttribs);
    } else {
        surface_ = eglCreateWindowSurface(display_, config_,
            reinterpret_cast<EGLNativeWindowType>(window), nullptr);
    }
    if (surface_ == EGL_NO_SURFACE) {
        logs.push_back("Mapped-plane GPU renderer eglCreateSurface failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        Destroy();
        return false;
    }

    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, contextAttribs);
    if (context_ == EGL_NO_CONTEXT) {
        logs.push_back("Mapped-plane GPU renderer eglCreateContext ES3 failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        Destroy();
        return false;
    }
    if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
        logs.push_back("Mapped-plane GPU renderer eglMakeCurrent failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        Destroy();
        return false;
    }

    if (!CreatePrograms(logs) || !CreateTextures(surfaceWidth, surfaceHeight, logs)) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        Destroy();
        return false;
    }

    window_ = window;
    targetWidth_ = effectiveTargetWidth;
    targetHeight_ = effectiveTargetHeight;
    surfaceWidth_ = surfaceWidth;
    surfaceHeight_ = surfaceHeight;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    logs.push_back("Mapped-plane GPU renderer initialized: target=" +
        std::to_string(targetWidth_) + "x" + std::to_string(targetHeight_) +
        " surface=" + std::to_string(surfaceWidth_) + "x" +
        std::to_string(surfaceHeight_) +
        (window == nullptr ? " offscreen-pbuffer" : " window") +
        " GLES3 mapped-plane shader path present=direct-yuv-rgb");
    return true;
}

bool Avc444GpuRenderer::ReadyToPresent() const
{
    return hasLuma_ && hasChroma_;
}

std::string Avc444GpuRenderer::DebugState() const
{
    std::ostringstream out;
    out << "renderer=window:" << (window_ != nullptr ? "yes" : "no")
        << ",eglSurface:" << (surface_ != EGL_NO_SURFACE ? "yes" : "no")
        << ",eglContext:" << (context_ != EGL_NO_CONTEXT ? "yes" : "no")
        << ",target:" << targetWidth_ << "x" << targetHeight_
        << ",surface:" << surfaceWidth_ << "x" << surfaceHeight_
        << ",luma:" << (hasLuma_ ? "yes" : "no")
        << ",chroma:" << (hasChroma_ ? "yes" : "no")
        << ",mode:avc444"
        << ",source:mapped-plane"
        << "," << lastFramebufferSample_;
    return out.str();
}

bool Avc444GpuRenderer::HasWindowTarget() const
{
    return window_ != nullptr && surface_ != EGL_NO_SURFACE;
}

void Avc444GpuRenderer::InvalidateComposedState()
{
    hasLuma_ = false;
    hasChroma_ = false;
}

bool Avc444GpuRenderer::ApplyLuma(const DecodedFrame& frame, const RECTANGLE_16* rects,
    uint32_t rectCount, std::vector<std::string>& logs)
{
    if (!MakeCurrent(logs) || !UploadSource(frame, logs)) {
        return false;
    }
    const bool hadChroma = hasChroma_;
    const bool fullSurfaceLuma =
        RectsCoverFullSurface(rects, rectCount, surfaceWidth_, surfaceHeight_);

    glUseProgram(copyYProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcYTexture_);
    glUniform1i(glGetUniformLocation(copyYProgram_, "uSrcY"), 0);
    glUniform1i(glGetUniformLocation(copyYProgram_, "uSrcHeight"),
        static_cast<GLint>(frame.height));
    glUniform1i(glGetUniformLocation(copyYProgram_, "uSurfaceHeight"),
        static_cast<GLint>(surfaceHeight_));
    DrawRectsToTexture(yTexture_, rects, rectCount);

    glUseProgram(lumaUvProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcUVTexture_);
    glUniform1i(glGetUniformLocation(lumaUvProgram_, "uSrcUV"), 0);
    glUniform1i(glGetUniformLocation(lumaUvProgram_, "uSrcUComponent"), frame.nv21 ? 1 : 0);
    glUniform1i(glGetUniformLocation(lumaUvProgram_, "uSrcVComponent"), frame.nv21 ? 0 : 1);
    glUniform1i(glGetUniformLocation(lumaUvProgram_, "uTargetPlane"), 0);
    glUniform1i(glGetUniformLocation(lumaUvProgram_, "uSurfaceHeight"),
        static_cast<GLint>(surfaceHeight_));
    const GLint lumaRectLeft = glGetUniformLocation(lumaUvProgram_, "uRectLeft");
    const GLint lumaRectTop = glGetUniformLocation(lumaUvProgram_, "uRectTop");
    DrawRectsToTexture(uTexture_, rects, rectCount, lumaRectLeft, lumaRectTop);

    glUniform1i(glGetUniformLocation(lumaUvProgram_, "uTargetPlane"), 1);
    DrawRectsToTexture(vTexture_, rects, rectCount, lumaRectLeft, lumaRectTop);

    const GLenum error = glGetError();
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (error != GL_NO_ERROR) {
        logs.push_back("AVC444 GPU luma shader failed glError=" +
            Hex32(static_cast<uint32_t>(error)));
        return false;
    }
    hasLuma_ = true;
    hasChroma_ = true;
    if (!hadChroma) {
        logs.push_back(
            "AVC444 GPU luma update initialized base chroma from AVC444_LUMA stream, "
            "matching FreeRDP LumaToYUV444"
            " fullSurface=" + std::string(fullSurfaceLuma ? "yes" : "no"));
    }
    return true;
}

bool Avc444GpuRenderer::ApplyChromaV1(const DecodedFrame& frame, const RECTANGLE_16* rects,
    uint32_t rectCount, std::vector<std::string>& logs)
{
    const uint32_t requiredYHeight = RequiredChromaV1SourceYHeight(rects, rectCount);
    if (requiredYHeight > frame.yUploadHeight) {
        logs.push_back("AVC444 GPU chroma-v1 source rejected: requiredYHeight=" +
            std::to_string(requiredYHeight) + " uploadedYHeight=" +
            std::to_string(frame.yUploadHeight) +
            " frame=" + FramePlaneText(frame));
        return false;
    }
    if (!MakeCurrent(logs) || !UploadSource(frame, logs)) {
        return false;
    }

    glUseProgram(chromaV1Program_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcYTexture_);
    glUniform1i(glGetUniformLocation(chromaV1Program_, "uSrcY"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, srcUVTexture_);
    glUniform1i(glGetUniformLocation(chromaV1Program_, "uSrcUV"), 1);
    glUniform1i(glGetUniformLocation(chromaV1Program_, "uSrcUComponent"), frame.nv21 ? 1 : 0);
    glUniform1i(glGetUniformLocation(chromaV1Program_, "uSrcVComponent"), frame.nv21 ? 0 : 1);
    glUniform1i(glGetUniformLocation(chromaV1Program_, "uSurfaceHeight"),
        static_cast<GLint>(surfaceHeight_));
    glUniform1i(glGetUniformLocation(chromaV1Program_, "uSrcYHeight"),
        static_cast<GLint>(frame.yUploadHeight));

    PingPongChromaPlane(true, rects, rectCount, chromaV1Program_);
    PingPongChromaPlane(false, rects, rectCount, chromaV1Program_);

    const GLenum error = glGetError();
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (error != GL_NO_ERROR) {
        logs.push_back("AVC444 GPU chroma-v1 shader failed glError=" +
            Hex32(static_cast<uint32_t>(error)));
        return false;
    }
    hasChroma_ = true;
    return true;
}

bool Avc444GpuRenderer::ApplyChromaV2(const DecodedFrame& frame, const RECTANGLE_16* rects,
    uint32_t rectCount, std::vector<std::string>& logs)
{
    if (!MakeCurrent(logs) || !UploadSource(frame, logs)) {
        return false;
    }

    glUseProgram(chromaV2Program_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcYTexture_);
    glUniform1i(glGetUniformLocation(chromaV2Program_, "uSrcY"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, srcUVTexture_);
    glUniform1i(glGetUniformLocation(chromaV2Program_, "uSrcUV"), 1);
    glUniform1i(glGetUniformLocation(chromaV2Program_, "uSrcUComponent"), frame.nv21 ? 1 : 0);
    glUniform1i(glGetUniformLocation(chromaV2Program_, "uSrcVComponent"), frame.nv21 ? 0 : 1);
    glUniform1i(glGetUniformLocation(chromaV2Program_, "uSurfaceWidth"),
        static_cast<GLint>(surfaceWidth_));
    glUniform1i(glGetUniformLocation(chromaV2Program_, "uSurfaceHeight"),
        static_cast<GLint>(surfaceHeight_));
    glUniform1i(glGetUniformLocation(chromaV2Program_, "uAlignedWidth"),
        static_cast<GLint>(frame.alignedWidth));

    PingPongChromaPlane(true, rects, rectCount, chromaV2Program_);
    PingPongChromaPlane(false, rects, rectCount, chromaV2Program_);

    const GLenum error = glGetError();
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (error != GL_NO_ERROR) {
        logs.push_back("AVC444 GPU chroma-v2 shader failed glError=" +
            Hex32(static_cast<uint32_t>(error)));
        return false;
    }
    hasChroma_ = true;
    return true;
}

bool Avc444GpuRenderer::Present(std::vector<std::string>& logs, bool logSuccess)
{
    const auto displayGeneration = DisplaySettings().Generation();
    const std::string modePrefix = "AVC444 GPU ";
    if (!hasLuma_ || !hasChroma_) {
        logs.push_back(modePrefix + "present skipped: luma=" +
            std::string(hasLuma_ ? "yes" : "no") +
            " chroma=" + std::string(hasChroma_ ? "yes" : "no"));
        return false;
    }
    if (!MakeCurrent(logs)) {
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, static_cast<GLsizei>(targetWidth_), static_cast<GLsizei>(targetHeight_));
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    const RenderViewport viewport = FitFrameIntoTarget(
        targetWidth_, targetHeight_, surfaceWidth_, surfaceHeight_);
    if (viewport.width == 0 || viewport.height == 0) {
        logs.push_back(modePrefix + "present viewport invalid");
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return false;
    }

    glUseProgram(presentProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, yTexture_);
    glUniform1i(glGetUniformLocation(presentProgram_, "uY"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, uTexture_);
    glUniform1i(glGetUniformLocation(presentProgram_, "uU"), 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, vTexture_);
    glUniform1i(glGetUniformLocation(presentProgram_, "uV"), 2);
    glUniform1i(glGetUniformLocation(presentProgram_, "uSurfaceWidth"),
        static_cast<GLint>(surfaceWidth_));
    glUniform1i(glGetUniformLocation(presentProgram_, "uSurfaceHeight"),
        static_cast<GLint>(surfaceHeight_));

    const GLfloat vertices[] = {
        -1.0F, -1.0F, 0.0F, 1.0F,
         1.0F, -1.0F, 1.0F, 1.0F,
        -1.0F,  1.0F, 0.0F, 0.0F,
         1.0F,  1.0F, 1.0F, 0.0F,
    };
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices + 2);
    glEnableVertexAttribArray(1);

    const GLint viewportY = static_cast<GLint>(targetHeight_ - viewport.y - viewport.height);
    glViewport(static_cast<GLint>(viewport.x), viewportY, static_cast<GLsizei>(viewport.width),
        static_cast<GLsizei>(viewport.height));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        logs.push_back(modePrefix + "present draw failed glError=" +
            Hex32(static_cast<uint32_t>(error)));
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return false;
    }
    if (logSuccess && IsGpuReadbackEnabled()) {
        lastFramebufferSample_ = SampleFramebuffer(viewport);
    } else if (logSuccess) {
        lastFramebufferSample_ = "readback:disabled";
    }
    if (!eglSwapBuffers(display_, surface_)) {
        logs.push_back(modePrefix + "present swap failed eglError=" +
            Hex32(static_cast<uint32_t>(eglGetError())));
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return false;
    }
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    DisplaySettings().Presented(displayGeneration, surfaceWidth_, surfaceHeight_);
    const uint32_t leftBar = viewport.x;
    const uint32_t topBar = viewport.y;
    const uint32_t rightBar = targetWidth_ - viewport.x - viewport.width;
    const uint32_t bottomBar = targetHeight_ - viewport.y - viewport.height;
    if (logSuccess) {
        logs.push_back(modePrefix + "compositor presented: target=" +
            std::to_string(targetWidth_) + "x" + std::to_string(targetHeight_) +
            " surface=" +
            std::to_string(surfaceWidth_) + "x" + std::to_string(surfaceHeight_) +
            " viewport=" + std::to_string(viewport.x) + "," + std::to_string(viewport.y) +
            " " + std::to_string(viewport.width) + "x" + std::to_string(viewport.height) +
            " letterboxLTRB=" + std::to_string(leftBar) + "," +
            std::to_string(topBar) + "," + std::to_string(rightBar) + "," +
            std::to_string(bottomBar) + " " + lastFramebufferSample_);
    }
    return true;
}

bool Avc444GpuRenderer::MakeCurrent(std::vector<std::string>& logs)
{
    if (display_ == EGL_NO_DISPLAY || surface_ == EGL_NO_SURFACE ||
        context_ == EGL_NO_CONTEXT) {
        logs.push_back("Mapped-plane GPU renderer is not initialized");
        return false;
    }
    if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
        logs.push_back("Mapped-plane GPU renderer eglMakeCurrent failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        return false;
    }
    return true;
}

void Avc444GpuRenderer::DeleteTextures()
{
    std::array<GLuint*, 8> textures {
        &srcYTexture_, &srcUVTexture_, &yTexture_, &uTexture_,
        &vTexture_, &uScratchTexture_, &vScratchTexture_, &framebuffer_
    };
    for (GLuint* value : textures) {
        if (*value == 0) {
            continue;
        }
        if (value == &framebuffer_) {
            glDeleteFramebuffers(1, value);
        } else {
            glDeleteTextures(1, value);
        }
        *value = 0;
    }
    srcYWidth_ = 0;
    srcYHeight_ = 0;
    srcUVWidth_ = 0;
    srcUVHeight_ = 0;
}

bool Avc444GpuRenderer::CreatePrograms(std::vector<std::string>& logs)
{
    static constexpr const char* updateVertex =
        "#version 300 es\n"
        "layout(location = 0) in vec2 aPosition;\n"
        "void main() { gl_Position = vec4(aPosition, 0.0, 1.0); }\n";
    static constexpr const char* presentVertex =
        "#version 300 es\n"
        "layout(location = 0) in vec2 aPosition;\n"
        "layout(location = 1) in vec2 aTexCoord;\n"
        "out vec2 vTexCoord;\n"
        "void main() {\n"
        "  gl_Position = vec4(aPosition, 0.0, 1.0);\n"
        "  vTexCoord = aTexCoord;\n"
        "}\n";
    static constexpr const char* copyYFragment =
        "#version 300 es\n"
        "precision highp float;\n"
        "uniform highp sampler2D uSrcY;\n"
        "uniform int uSrcHeight;\n"
        "uniform int uSurfaceHeight;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "  highp int x = int(floor(gl_FragCoord.x));\n"
        "  highp int y = int(floor(float(uSurfaceHeight) - gl_FragCoord.y));\n"
        "  float value = texelFetch(uSrcY, ivec2(x, y), 0).r;\n"
        "  fragColor = vec4(value, 0.0, 0.0, 1.0);\n"
        "}\n";
    static constexpr const char* lumaUvFragment =
        "#version 300 es\n"
        "precision highp float;\n"
        "uniform highp sampler2D uSrcUV;\n"
        "uniform int uSrcUComponent;\n"
        "uniform int uSrcVComponent;\n"
        "uniform int uTargetPlane;\n"
        "uniform int uSurfaceHeight;\n"
        "uniform int uRectLeft;\n"
        "uniform int uRectTop;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "  highp int x = int(floor(gl_FragCoord.x));\n"
        "  highp int y = int(floor(float(uSurfaceHeight) - gl_FragCoord.y));\n"
        "  int relX = x - uRectLeft;\n"
        "  int relY = y - uRectTop;\n"
        "  int srcX = uRectLeft / 2 + relX / 2;\n"
        "  int srcY = uRectTop / 2 + relY / 2;\n"
        "  vec2 uv = texelFetch(uSrcUV, ivec2(srcX, srcY), 0).rg;\n"
        "  int component = (uTargetPlane == 0) ? uSrcUComponent : uSrcVComponent;\n"
        "  float value = (component == 0) ? uv.r : uv.g;\n"
        "  fragColor = vec4(value, 0.0, 0.0, 1.0);\n"
        "}\n";
    static constexpr const char* chromaV1Fragment =
        "#version 300 es\n"
        "precision highp float;\n"
        "uniform highp sampler2D uPrev;\n"
        "uniform highp sampler2D uSrcY;\n"
        "uniform highp sampler2D uSrcUV;\n"
        "uniform int uSrcUComponent;\n"
        "uniform int uSrcVComponent;\n"
        "uniform int uTargetPlane;\n"
        "uniform int uSurfaceHeight;\n"
        "uniform int uSrcYHeight;\n"
        "uniform int uRectLeft;\n"
        "uniform int uRectTop;\n"
        "uniform int uRectRight;\n"
        "uniform int uRectBottom;\n"
        "out vec4 fragColor;\n"
        "float sampleUV(int x, int y, int component) {\n"
        "  vec2 uv = texelFetch(uSrcUV, ivec2(x, y), 0).rg;\n"
        "  return (component == 0) ? uv.r : uv.g;\n"
        "}\n"
        "void main() {\n"
        "  int x = int(floor(gl_FragCoord.x));\n"
        "  int y = int(floor(float(uSurfaceHeight) - gl_FragCoord.y));\n"
        "  int relX = x - uRectLeft;\n"
        "  int relY = y - uRectTop;\n"
        "  int rectWidth = uRectRight - uRectLeft;\n"
        "  int rectHeight = uRectBottom - uRectTop;\n"
        "  int stateY = uSurfaceHeight - 1 - y;\n"
        "  float value = texelFetch(uPrev, ivec2(x, stateY), 0).r;\n"
        "  if ((relY & 1) == 1) {\n"
        "    int oddRow = relY / 2;\n"
        "    int group = oddRow / 8;\n"
        "    int inGroup = oddRow - group * 8;\n"
        "    int srcY = uRectTop + group * 16 + inGroup + ((uTargetPlane == 1) ? 8 : 0);\n"
        "    srcY = min(srcY, uSrcYHeight - 1);\n"
        "    value = texelFetch(uSrcY, ivec2(uRectLeft + relX, srcY), 0).r;\n"
        "  } else if ((relX & 1) == 1 && relY < (rectHeight / 2) * 2 && relX < (rectWidth / 2) * 2) {\n"
        "    int srcX = uRectLeft / 2 + relX / 2;\n"
        "    int srcY = uRectTop / 2 + relY / 2;\n"
        "    int component = (uTargetPlane == 0) ? uSrcUComponent : uSrcVComponent;\n"
        "    value = sampleUV(srcX, srcY, component);\n"
        "  }\n"
        "  fragColor = vec4(value, 0.0, 0.0, 1.0);\n"
        "}\n";
    static constexpr const char* chromaV2Fragment =
        "#version 300 es\n"
        "precision highp float;\n"
        "uniform highp sampler2D uPrev;\n"
        "uniform highp sampler2D uSrcY;\n"
        "uniform highp sampler2D uSrcUV;\n"
        "uniform int uSrcUComponent;\n"
        "uniform int uSrcVComponent;\n"
        "uniform int uTargetPlane;\n"
        "uniform int uSurfaceWidth;\n"
        "uniform int uSurfaceHeight;\n"
        "uniform int uAlignedWidth;\n"
        "uniform int uRectLeft;\n"
        "uniform int uRectTop;\n"
        "out vec4 fragColor;\n"
        "float sampleUV(int x, int y, int component) {\n"
        "  vec2 uv = texelFetch(uSrcUV, ivec2(x, y), 0).rg;\n"
        "  return (component == 0) ? uv.r : uv.g;\n"
        "}\n"
        "void main() {\n"
        "  int x = int(floor(gl_FragCoord.x));\n"
        "  int y = int(floor(float(uSurfaceHeight) - gl_FragCoord.y));\n"
        "  int relX = x - uRectLeft;\n"
        "  int relY = y - uRectTop;\n"
        "  int stateY = uSurfaceHeight - 1 - y;\n"
        "  float value = texelFetch(uPrev, ivec2(x, stateY), 0).r;\n"
        "  if ((relX & 1) == 1) {\n"
        "    int srcX = uRectLeft / 2 + relX / 2 + ((uTargetPlane == 1) ? (uAlignedWidth / 2) : 0);\n"
        "    value = texelFetch(uSrcY, ivec2(srcX, y), 0).r;\n"
        "  } else if ((relY & 1) == 1 && (relX & 3) == 0) {\n"
        "    int srcX = uRectLeft / 4 + relX / 4 + ((uTargetPlane == 1) ? (uAlignedWidth / 4) : 0);\n"
        "    int srcY = uRectTop / 2 + relY / 2;\n"
        "    value = sampleUV(srcX, srcY, uSrcUComponent);\n"
        "  } else if ((relY & 1) == 1 && (relX & 3) == 2) {\n"
        "    int srcX = uRectLeft / 4 + relX / 4 + ((uTargetPlane == 1) ? (uAlignedWidth / 4) : 0);\n"
        "    int srcY = uRectTop / 2 + relY / 2;\n"
        "    value = sampleUV(srcX, srcY, uSrcVComponent);\n"
        "  }\n"
        "  fragColor = vec4(value, 0.0, 0.0, 1.0);\n"
        "}\n";
    static constexpr const char* presentFragment =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 vTexCoord;\n"
        "uniform highp sampler2D uY;\n"
        "uniform highp sampler2D uU;\n"
        "uniform highp sampler2D uV;\n"
        "uniform int uSurfaceWidth;\n"
        "uniform int uSurfaceHeight;\n"
        "out vec4 fragColor;\n"
        "float fetchPlane(sampler2D tex, int x, int y) {\n"
        "  x = clamp(x, 0, uSurfaceWidth - 1);\n"
        "  y = clamp(y, 0, uSurfaceHeight - 1);\n"
        "  return texelFetch(tex, ivec2(x, uSurfaceHeight - 1 - y), 0).r;\n"
        "}\n"
        "float conditionalClipAverage(float value, float original) {\n"
        "  float clipped = clamp(value, 0.0, 1.0);\n"
        "  return (abs(clipped - original) < (30.0 / 255.0)) ? original : clipped;\n"
        "}\n"
        "float fetchPlaneWithAverageUndo(sampler2D tex, int x, int y) {\n"
        "  float original = fetchPlane(tex, x, y);\n"
        "  if (((x & 1) == 0) && ((y & 1) == 0) && x + 1 < uSurfaceWidth && y + 1 < uSurfaceHeight) {\n"
        "    float sub = fetchPlane(tex, x + 1, y) + fetchPlane(tex, x, y + 1) + "
            "fetchPlane(tex, x + 1, y + 1);\n"
        "    return conditionalClipAverage(4.0 * original - sub, original);\n"
        "  }\n"
        "  return original;\n"
        "}\n"
        "void main() {\n"
        "  int x = clamp(int(floor(vTexCoord.x * float(uSurfaceWidth))), 0, uSurfaceWidth - 1);\n"
        "  int y = clamp(int(floor(vTexCoord.y * float(uSurfaceHeight))), 0, uSurfaceHeight - 1);\n"
        "  float yy = fetchPlane(uY, x, y);\n"
        "  float uu = fetchPlaneWithAverageUndo(uU, x, y);\n"
        "  float vv = fetchPlaneWithAverageUndo(uV, x, y);\n"
        "  float d = uu - (128.0 / 255.0);\n"
        "  float e = vv - (128.0 / 255.0);\n"
        "  vec3 rgb = vec3(yy + 1.57421875 * e,\n"
        "                 yy - 0.1875 * d - 0.46875 * e,\n"
        "                 yy + 1.85546875 * d);\n"
        "  fragColor = vec4(clamp(rgb, 0.0, 1.0), 1.0);\n"
        "}\n";
    copyYProgram_ = LinkProgram(updateVertex, copyYFragment, logs);
    lumaUvProgram_ = LinkProgram(updateVertex, lumaUvFragment, logs);
    chromaV1Program_ = LinkProgram(updateVertex, chromaV1Fragment, logs);
    chromaV2Program_ = LinkProgram(updateVertex, chromaV2Fragment, logs);
    presentProgram_ = LinkProgram(presentVertex, presentFragment, logs);
    if (copyYProgram_ == 0 || lumaUvProgram_ == 0 ||
        chromaV1Program_ == 0 || chromaV2Program_ == 0 || presentProgram_ == 0) {
        return false;
    }
    return true;
}

void Avc444GpuRenderer::ConfigureTexture(GLenum target)
{
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

bool Avc444GpuRenderer::CreateTextures(uint32_t surfaceWidth, uint32_t surfaceHeight,
    std::vector<std::string>& logs)
{
    glGenFramebuffers(1, &framebuffer_);
    glGenTextures(1, &srcYTexture_);
    glGenTextures(1, &srcUVTexture_);
    glGenTextures(1, &yTexture_);
    glGenTextures(1, &uTexture_);
    glGenTextures(1, &vTexture_);
    glGenTextures(1, &uScratchTexture_);
    glGenTextures(1, &vScratchTexture_);

    const std::array<GLuint, 5> stateTextures {
        yTexture_, uTexture_, vTexture_, uScratchTexture_, vScratchTexture_
    };
    for (GLuint texture : stateTextures) {
        glBindTexture(GL_TEXTURE_2D, texture);
        ConfigureTexture(GL_TEXTURE_2D);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<GLsizei>(surfaceWidth),
            static_cast<GLsizei>(surfaceHeight), 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, srcYTexture_);
    ConfigureTexture(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, srcUVTexture_);
    ConfigureTexture(GL_TEXTURE_2D);
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR || framebuffer_ == 0 || srcYTexture_ == 0 || srcUVTexture_ == 0 ||
        yTexture_ == 0 || uTexture_ == 0 || vTexture_ == 0 || uScratchTexture_ == 0 ||
        vScratchTexture_ == 0) {
        logs.push_back("Mapped-plane GPU texture allocation failed glError=" +
            Hex32(static_cast<uint32_t>(error)));
        return false;
    }
    return true;
}

bool Avc444GpuRenderer::UploadSource(const DecodedFrame& frame, std::vector<std::string>& logs)
{
    if (!frame.PlanesValid()) {
        logs.push_back("Mapped-plane GPU source upload rejected: invalid decoded frame");
        return false;
    }
    if (frame.yUploadWidth == 0 || frame.yUploadHeight == 0 ||
        frame.uvUploadWidth == 0 || frame.uvUploadHeight == 0) {
        logs.push_back("Mapped-plane GPU source upload rejected: invalid upload dimensions");
        return false;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, srcYTexture_);
    ConfigureTexture(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(frame.y.rowStride));
    if (srcYWidth_ != frame.yUploadWidth || srcYHeight_ != frame.yUploadHeight) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<GLsizei>(frame.yUploadWidth),
            static_cast<GLsizei>(frame.yUploadHeight), 0, GL_RED, GL_UNSIGNED_BYTE,
            frame.y.data);
        srcYWidth_ = frame.yUploadWidth;
        srcYHeight_ = frame.yUploadHeight;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(frame.yUploadWidth),
            static_cast<GLsizei>(frame.yUploadHeight), GL_RED, GL_UNSIGNED_BYTE,
            frame.y.data);
    }

    glBindTexture(GL_TEXTURE_2D, srcUVTexture_);
    ConfigureTexture(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(frame.uv.rowStride / 2U));
    if (srcUVWidth_ != frame.uvUploadWidth || srcUVHeight_ != frame.uvUploadHeight) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, static_cast<GLsizei>(frame.uvUploadWidth),
            static_cast<GLsizei>(frame.uvUploadHeight), 0, GL_RG, GL_UNSIGNED_BYTE,
            frame.uv.data);
        srcUVWidth_ = frame.uvUploadWidth;
        srcUVHeight_ = frame.uvUploadHeight;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(frame.uvUploadWidth),
            static_cast<GLsizei>(frame.uvUploadHeight), GL_RG, GL_UNSIGNED_BYTE,
            frame.uv.data);
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        logs.push_back("Mapped-plane GPU source texture upload failed glError=" +
            Hex32(static_cast<uint32_t>(error)));
        return false;
    }
    return true;
}

void Avc444GpuRenderer::DrawRectsToTexture(GLuint texture, const RECTANGLE_16* rects,
    uint32_t rectCount, GLint rectLeftLocation, GLint rectTopLocation,
    GLint rectRightLocation, GLint rectBottomLocation)
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    const GLfloat vertices[] = {
        -1.0F, -1.0F,
         1.0F, -1.0F,
        -1.0F,  1.0F,
         1.0F,  1.0F,
    };
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), vertices);
    glEnableVertexAttribArray(0);

    for (uint32_t i = 0; i < rectCount; ++i) {
        const RECTANGLE_16& rect = rects[i];
        const uint32_t width = rect.right - rect.left;
        const uint32_t height = rect.bottom - rect.top;
        if (rectLeftLocation >= 0) {
            glUniform1i(rectLeftLocation, static_cast<GLint>(rect.left));
        }
        if (rectTopLocation >= 0) {
            glUniform1i(rectTopLocation, static_cast<GLint>(rect.top));
        }
        if (rectRightLocation >= 0) {
            glUniform1i(rectRightLocation, static_cast<GLint>(rect.right));
        }
        if (rectBottomLocation >= 0) {
            glUniform1i(rectBottomLocation, static_cast<GLint>(rect.bottom));
        }
        glViewport(static_cast<GLint>(rect.left),
            static_cast<GLint>(surfaceHeight_ - rect.bottom),
            static_cast<GLsizei>(width), static_cast<GLsizei>(height));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Avc444GpuRenderer::CopyPlaneTexture(GLuint source, GLuint target)
{
    GLint activeTexture = GL_TEXTURE0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, source, 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, target);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
        static_cast<GLsizei>(surfaceWidth_), static_cast<GLsizei>(surfaceHeight_));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(static_cast<GLenum>(activeTexture));
}

void Avc444GpuRenderer::PingPongChromaPlane(bool uPlane, const RECTANGLE_16* rects,
    uint32_t rectCount, GLuint program)
{
    GLuint& current = uPlane ? uTexture_ : vTexture_;
    GLuint scratch = uPlane ? uScratchTexture_ : vScratchTexture_;

    CopyPlaneTexture(current, scratch);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, current);
    glUniform1i(glGetUniformLocation(program, "uPrev"), 2);
    glUniform1i(glGetUniformLocation(program, "uTargetPlane"), uPlane ? 0 : 1);
    DrawRectsToTexture(scratch, rects, rectCount,
        glGetUniformLocation(program, "uRectLeft"),
        glGetUniformLocation(program, "uRectTop"),
        glGetUniformLocation(program, "uRectRight"),
        glGetUniformLocation(program, "uRectBottom"));
    std::swap(current, scratch);
    if (uPlane) {
        uScratchTexture_ = scratch;
    } else {
        vScratchTexture_ = scratch;
    }
}

} // namespace rdp_bridge
