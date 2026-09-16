#include "surface/avc420_gpu_compositor_internal_types.h"
#include "surface/avc_gpu_common.h"
#include "common/frame_utils.h"
#include "common/string_utils.h"
#include "session/session_display_settings.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <native_window/external_window.h>

namespace rdp_bridge {
namespace {

GLuint CompileShader(GLenum type, const char* source, std::vector<std::string>& logs)
{
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        logs.push_back("AVC420 native-buffer GPU GLES create shader failed: " +
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
    logs.push_back("AVC420 native-buffer GPU GLES shader compile failed type=" +
        std::to_string(type) + " log=" + (info.empty() ? "none" : info));
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
    logs.push_back("AVC420 native-buffer GPU GLES program link failed log=" +
        (info.empty() ? "none" : info));
    glDeleteProgram(program);
    return 0;
}

} // namespace

Avc420NativeBufferRenderer::~Avc420NativeBufferRenderer()
{
    Destroy();
}

void Avc420NativeBufferRenderer::Destroy()
{
    if (display_ != EGL_NO_DISPLAY) {
        if (context_ != EGL_NO_CONTEXT &&
            (pbufferSurface_ != EGL_NO_SURFACE || windowSurface_ != EGL_NO_SURFACE)) {
            EGLSurface currentSurface =
                pbufferSurface_ != EGL_NO_SURFACE ? pbufferSurface_ : windowSurface_;
            if (eglMakeCurrent(display_, currentSurface, currentSurface, context_)) {
                DeleteCompositeSurface();
                if (program_ != 0) {
                    glDeleteProgram(program_);
                }
                if (presentProgram_ != 0) {
                    glDeleteProgram(presentProgram_);
                }
                eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            }
        }
        if (windowSurface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, windowSurface_);
        }
        if (pbufferSurface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, pbufferSurface_);
        }
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
        }
        eglTerminate(display_);
    }

    display_ = EGL_NO_DISPLAY;
    config_ = nullptr;
    pbufferSurface_ = EGL_NO_SURFACE;
    windowSurface_ = EGL_NO_SURFACE;
    context_ = EGL_NO_CONTEXT;
    window_ = nullptr;
    targetWidth_ = 0;
    targetHeight_ = 0;
    surfaceWidth_ = 0;
    surfaceHeight_ = 0;
    program_ = 0;
    presentProgram_ = 0;
    compositeTexture_ = 0;
    compositeFramebuffer_ = 0;
    compositeWidth_ = 0;
    compositeHeight_ = 0;
    compositeReady_ = false;
    composites_ = 0;
    backgroundComposites_ = 0;
    compositeFailures_ = 0;
    imports_ = 0;
    importFailures_ = 0;
    presents_ = 0;
    rgbaUploadBuffer_.clear();
}

bool Avc420NativeBufferRenderer::Ensure(OHNativeWindow* window, uint32_t targetWidth, uint32_t targetHeight,
    uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<std::string>& logs)
{
    if (surfaceWidth == 0 || surfaceHeight == 0 ||
        (window != nullptr && (targetWidth == 0 || targetHeight == 0))) {
        logs.push_back("AVC420 native-buffer GPU renderer target invalid");
        return false;
    }

    const uint32_t effectiveTargetWidth = window == nullptr ? surfaceWidth : targetWidth;
    const uint32_t effectiveTargetHeight = window == nullptr ? surfaceHeight : targetHeight;

    if (!EnsureInitialized(logs)) {
        return false;
    }

    surfaceWidth_ = surfaceWidth;
    surfaceHeight_ = surfaceHeight;
    if (window == nullptr) {
        return true;
    }

    if (window_ != window || windowSurface_ == EGL_NO_SURFACE) {
        DestroyWindowSurface();
        windowSurface_ = CreateWindowSurface(window, logs);
        if (windowSurface_ == EGL_NO_SURFACE) {
            window_ = nullptr;
            targetWidth_ = 0;
            targetHeight_ = 0;
            return false;
        }
        window_ = window;
        logs.push_back("AVC420 native-buffer GPU renderer attached EGL window surface");
    }

    targetWidth_ = effectiveTargetWidth;
    targetHeight_ = effectiveTargetHeight;
    return true;
}

bool Avc420NativeBufferRenderer::CompositeFrame(const NativeDecodedFrame& frame, const RECTANGLE_16* rects,
    uint32_t rectCount, std::vector<std::string>& logs, bool logSuccess)
{
    if (!frame.Valid()) {
        logs.push_back("AVC420 native-buffer GPU composite rejected invalid frame");
        return false;
    }
    if (!MakePbufferCurrent(logs)) {
        return false;
    }
    if (!EnsureCompositeSurface(logs)) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return false;
    }

    ImportedTexture imported;
    if (!ImportFrame(frame, imported, logs)) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, compositeFramebuffer_);
    glViewport(0, 0, static_cast<GLsizei>(surfaceWidth_),
        static_cast<GLsizei>(surfaceHeight_));
    glUseProgram(program_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, imported.texture);
    glUniform1i(glGetUniformLocation(program_, "uFrame"), 0);
    glUniform2f(glGetUniformLocation(program_, "uCropScale"), 1.0F, 1.0F);
    glUniform2f(glGetUniformLocation(program_, "uCropClamp"), 1.0F, 1.0F);

    const GLfloat nativeWidth = static_cast<GLfloat>(
        std::max<uint32_t>(surfaceWidth_, frame.nativeWidth > 0 ? frame.nativeWidth : frame.width));
    const GLfloat nativeHeight = static_cast<GLfloat>(
        std::max<uint32_t>(surfaceHeight_, frame.nativeHeight > 0 ? frame.nativeHeight : frame.height));
    uint32_t drawnRects = 0;
    for (uint32_t index = 0; index < rectCount; ++index) {
        const RECTANGLE_16& rect = rects[index];
        if (rect.left >= rect.right || rect.top >= rect.bottom ||
            rect.right > surfaceWidth_ || rect.bottom > surfaceHeight_) {
            continue;
        }
        DrawExternalRectToComposite(rect, nativeWidth, nativeHeight);
        ++drawnRects;
    }

    GLenum error = glGetError();
    ReleaseImport(imported);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (error != GL_NO_ERROR || drawnRects == 0) {
        ++compositeFailures_;
        logs.push_back("AVC420 native-buffer GPU composite failed: glError=" +
            Hex32(static_cast<uint32_t>(error)) +
            " drawnRects=" + std::to_string(drawnRects) +
            " inputRects=" + std::to_string(rectCount) + " " + NativeFrameText(frame));
        return false;
    }

    compositeReady_ = true;
    ++composites_;
    if (logSuccess) {
        logs.push_back("AVC420 native-buffer GPU composited dirty frame: rects=" +
            std::to_string(drawnRects) + "/" + std::to_string(rectCount) +
            " surface=" + std::to_string(surfaceWidth_) + "x" +
            std::to_string(surfaceHeight_) +
            " composites=" + std::to_string(composites_) +
            " " + NativeFrameText(frame));
    }
    return true;
}

bool Avc420NativeBufferRenderer::CompositeRgbaFrame(const RgbaFrame& frame, std::vector<std::string>& logs,
    bool logSuccess)
{
    const int32_t sourceStride = frame.strideBytes > 0 ? frame.strideBytes :
        static_cast<int32_t>(frame.width * 4U);
    if (frame.data == nullptr || frame.width == 0 || frame.height == 0 ||
        sourceStride < static_cast<int32_t>(frame.width * 4U)) {
        logs.push_back("AVC420 native-buffer GPU rejected invalid GDI background frame");
        return false;
    }
    if (frame.width != surfaceWidth_ || frame.height != surfaceHeight_) {
        logs.push_back("AVC420 native-buffer GPU rejected GDI background size mismatch: frame=" +
            std::to_string(frame.width) + "x" + std::to_string(frame.height) +
            " surface=" + std::to_string(surfaceWidth_) + "x" +
            std::to_string(surfaceHeight_));
        return false;
    }
    if (!MakePbufferCurrent(logs)) {
        return false;
    }
    if (!EnsureCompositeSurface(logs)) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return false;
    }

    DirtyFrameStats dirty = frame.dirty;
    if (!dirty.valid || dirty.width == 0 || dirty.height == 0 ||
        dirty.x >= frame.width || dirty.y >= frame.height) {
        dirty.valid = true;
        dirty.rectCount = 1;
        dirty.x = 0;
        dirty.y = 0;
        dirty.width = frame.width;
        dirty.height = frame.height;
        dirty.areaPermille = 1000;
    }
    if (dirty.x + dirty.width > frame.width) {
        dirty.width = frame.width - dirty.x;
    }
    if (dirty.y + dirty.height > frame.height) {
        dirty.height = frame.height - dirty.y;
    }
    if (dirty.width == 0 || dirty.height == 0) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return false;
    }

    const size_t rowBytes = static_cast<size_t>(dirty.width) * 4U;
    rgbaUploadBuffer_.resize(rowBytes * dirty.height);
    for (uint32_t row = 0; row < dirty.height; ++row) {
        const uint32_t srcY = dirty.y + dirty.height - 1U - row;
        const uint8_t* src = frame.data +
            static_cast<int64_t>(srcY) * sourceStride +
            static_cast<size_t>(dirty.x) * 4U;
        std::memcpy(rgbaUploadBuffer_.data() + rowBytes * row, src, rowBytes);
    }

    const uint32_t dstY = frame.height - dirty.y - dirty.height;
    glBindTexture(GL_TEXTURE_2D, compositeTexture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(dirty.x),
        static_cast<GLint>(dstY), static_cast<GLsizei>(dirty.width),
        static_cast<GLsizei>(dirty.height), GL_RGBA, GL_UNSIGNED_BYTE,
        rgbaUploadBuffer_.data());
    const GLenum error = glGetError();
    glBindTexture(GL_TEXTURE_2D, 0);
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (error != GL_NO_ERROR) {
        ++compositeFailures_;
        logs.push_back("AVC420 native-buffer GPU GDI background upload failed: glError=" +
            Hex32(static_cast<uint32_t>(error)) + " " + DescribeDirtyStats(dirty));
        return false;
    }

    compositeReady_ = true;
    ++backgroundComposites_;
    if (logSuccess) {
        logs.push_back("AVC420 native-buffer GPU composited GDI background: " +
            DescribeDirtyStats(dirty) +
            " composites=" + std::to_string(backgroundComposites_) +
            " surface=" + std::to_string(surfaceWidth_) + "x" +
            std::to_string(surfaceHeight_));
    }
    return true;
}

bool Avc420NativeBufferRenderer::PresentComposite(std::vector<std::string>& logs, bool logSuccess)
{
    const auto displayGeneration = DisplaySettings().Generation();
    if (!compositeReady_ || compositeTexture_ == 0 ||
        compositeWidth_ == 0 || compositeHeight_ == 0) {
        logs.push_back("AVC420 native-buffer GPU composite present skipped: retained surface missing");
        return false;
    }
    if (window_ == nullptr || windowSurface_ == EGL_NO_SURFACE ||
        targetWidth_ == 0 || targetHeight_ == 0) {
        logs.push_back("AVC420 native-buffer GPU composite present skipped: window target missing");
        return false;
    }
    if (!MakeWindowCurrent(logs)) {
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, static_cast<GLsizei>(targetWidth_), static_cast<GLsizei>(targetHeight_));
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    bool snappedViewport = false;
    const RenderViewport viewport = FitAvc420PresentViewport(
        targetWidth_, targetHeight_, compositeWidth_, compositeHeight_, snappedViewport);
    if (viewport.width == 0 || viewport.height == 0) {
        logs.push_back("AVC420 native-buffer GPU composite present viewport invalid");
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return false;
    }

    glUseProgram(presentProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, compositeTexture_);
    glUniform1i(glGetUniformLocation(presentProgram_, "uFrame"), 0);

    const GLfloat vertices[] = {
        -1.0F, -1.0F, 0.0F, 0.0F,
         1.0F, -1.0F, 1.0F, 0.0F,
        -1.0F,  1.0F, 0.0F, 1.0F,
         1.0F,  1.0F, 1.0F, 1.0F,
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
        logs.push_back("AVC420 native-buffer GPU composite present draw failed glError=" +
            Hex32(static_cast<uint32_t>(error)));
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return false;
    }

    if (!eglSwapBuffers(display_, windowSurface_)) {
        logs.push_back("AVC420 native-buffer GPU composite present swap failed eglError=" +
            Hex32(static_cast<uint32_t>(eglGetError())));
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return false;
    }

    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    ++presents_;
    DisplaySettings().Presented(displayGeneration, compositeWidth_, compositeHeight_);
    if (logSuccess) {
        const uint32_t leftBar = viewport.x;
        const uint32_t topBar = viewport.y;
        const uint32_t rightBar = targetWidth_ - viewport.x - viewport.width;
        const uint32_t bottomBar = targetHeight_ - viewport.y - viewport.height;
        logs.push_back("AVC420 native-buffer GPU presented retained composite: target=" +
            std::to_string(targetWidth_) + "x" + std::to_string(targetHeight_) +
            " surface=" + std::to_string(compositeWidth_) + "x" +
            std::to_string(compositeHeight_) +
            " viewport=" + std::to_string(viewport.x) + "," +
            std::to_string(viewport.y) + " " + std::to_string(viewport.width) +
            "x" + std::to_string(viewport.height) +
            " letterboxLTRB=" + std::to_string(leftBar) + "," +
            std::to_string(topBar) + "," + std::to_string(rightBar) + "," +
            std::to_string(bottomBar) +
            " snapFill=" + std::string(snappedViewport ? "yes" : "no") +
            " composites=" + std::to_string(composites_) +
            " failures=" + std::to_string(compositeFailures_) +
            " retainFrames=yes");
    }
    return true;
}

void Avc420NativeBufferRenderer::DetachWindowSurface(const std::string& reason, std::vector<std::string>& logs)
{
    if (display_ == EGL_NO_DISPLAY || windowSurface_ == EGL_NO_SURFACE ||
        window_ == nullptr) {
        return;
    }
    DestroyWindowSurface();
    logs.push_back("AVC420 native-buffer GPU renderer detached window after " + reason +
        "; EGL context and pbuffer preserved");
}

std::string Avc420NativeBufferRenderer::DebugState() const
{
    std::ostringstream out;
    out << "renderer=window:" << (window_ != nullptr ? "yes" : "no")
        << ",windowSurface:" << (windowSurface_ != EGL_NO_SURFACE ? "yes" : "no")
        << ",pbufferSurface:" << (pbufferSurface_ != EGL_NO_SURFACE ? "yes" : "no")
        << ",eglContext:" << (context_ != EGL_NO_CONTEXT ? "yes" : "no")
        << ",target:" << targetWidth_ << "x" << targetHeight_
        << ",surface:" << surfaceWidth_ << "x" << surfaceHeight_
        << ",source:native-buffer-oes"
        << ",imports:" << imports_ << "/" << importFailures_
        << ",composite:" << (compositeReady_ ? "yes" : "no")
        << ",compositeSize:" << compositeWidth_ << "x" << compositeHeight_
        << ",composites:" << composites_ << "/" << backgroundComposites_ <<
            "/" << compositeFailures_
        << ",presents:" << presents_
        << ",retainFrames:yes";
    return out.str();
}

EGLSurface Avc420NativeBufferRenderer::CreatePbufferSurface(std::vector<std::string>& logs)
{
    const EGLint pbufferAttribs[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE,
    };
    EGLSurface pbuffer = eglCreatePbufferSurface(display_, config_, pbufferAttribs);
    if (pbuffer == EGL_NO_SURFACE) {
        logs.push_back("AVC420 native-buffer GPU renderer create pbuffer failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
    }
    return pbuffer;
}

EGLSurface Avc420NativeBufferRenderer::CreateWindowSurface(OHNativeWindow* window, std::vector<std::string>& logs)
{
    EGLSurface windowSurface = eglCreateWindowSurface(display_, config_,
        reinterpret_cast<EGLNativeWindowType>(window), nullptr);
    if (windowSurface == EGL_NO_SURFACE) {
        logs.push_back("AVC420 native-buffer GPU renderer attach window surface failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
    }
    return windowSurface;
}

void Avc420NativeBufferRenderer::DestroyWindowSurface()
{
    if (display_ == EGL_NO_DISPLAY) {
        return;
    }
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (windowSurface_ != EGL_NO_SURFACE) {
        eglDestroySurface(display_, windowSurface_);
    }
    windowSurface_ = EGL_NO_SURFACE;
    window_ = nullptr;
    targetWidth_ = 0;
    targetHeight_ = 0;
}

void Avc420NativeBufferRenderer::DeleteCompositeSurface()
{
    if (compositeFramebuffer_ != 0) {
        glDeleteFramebuffers(1, &compositeFramebuffer_);
        compositeFramebuffer_ = 0;
    }
    if (compositeTexture_ != 0) {
        glDeleteTextures(1, &compositeTexture_);
        compositeTexture_ = 0;
    }
    compositeWidth_ = 0;
    compositeHeight_ = 0;
    compositeReady_ = false;
}

bool Avc420NativeBufferRenderer::EnsureCompositeSurface(std::vector<std::string>& logs)
{
    if (surfaceWidth_ == 0 || surfaceHeight_ == 0) {
        logs.push_back("AVC420 native-buffer GPU composite surface size invalid");
        return false;
    }
    if (compositeTexture_ != 0 && compositeFramebuffer_ != 0 &&
        compositeWidth_ == surfaceWidth_ && compositeHeight_ == surfaceHeight_) {
        return true;
    }

    DeleteCompositeSurface();
    glGenTextures(1, &compositeTexture_);
    glBindTexture(GL_TEXTURE_2D, compositeTexture_);
    ConfigureTexture(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(surfaceWidth_),
        static_cast<GLsizei>(surfaceHeight_), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glGenFramebuffers(1, &compositeFramebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, compositeFramebuffer_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        compositeTexture_, 0);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        ++compositeFailures_;
        logs.push_back("AVC420 native-buffer GPU composite framebuffer incomplete: status=" +
            Hex32(static_cast<uint32_t>(status)) +
            " surface=" + std::to_string(surfaceWidth_) + "x" +
            std::to_string(surfaceHeight_));
        DeleteCompositeSurface();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glViewport(0, 0, static_cast<GLsizei>(surfaceWidth_),
        static_cast<GLsizei>(surfaceHeight_));
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    compositeWidth_ = surfaceWidth_;
    compositeHeight_ = surfaceHeight_;
    compositeReady_ = false;
    logs.push_back("AVC420 native-buffer GPU allocated retained composite surface: " +
        std::to_string(compositeWidth_) + "x" + std::to_string(compositeHeight_));
    return true;
}

void Avc420NativeBufferRenderer::DrawExternalRectToComposite(const RECTANGLE_16& rect,
    GLfloat nativeWidth, GLfloat nativeHeight)
{
    const GLfloat surfaceWidth = static_cast<GLfloat>(surfaceWidth_);
    const GLfloat surfaceHeight = static_cast<GLfloat>(surfaceHeight_);
    const GLfloat left = static_cast<GLfloat>(rect.left);
    const GLfloat top = static_cast<GLfloat>(rect.top);
    const GLfloat right = static_cast<GLfloat>(rect.right);
    const GLfloat bottom = static_cast<GLfloat>(rect.bottom);

    const GLfloat x0 = (left / surfaceWidth) * 2.0F - 1.0F;
    const GLfloat x1 = (right / surfaceWidth) * 2.0F - 1.0F;
    const GLfloat y0 = 1.0F - (top / surfaceHeight) * 2.0F;
    const GLfloat y1 = 1.0F - (bottom / surfaceHeight) * 2.0F;
    const GLfloat u0 = left / nativeWidth;
    const GLfloat u1 = right / nativeWidth;
    const GLfloat v0 = top / nativeHeight;
    const GLfloat v1 = bottom / nativeHeight;

    const GLfloat vertices[] = {
        x0, y1, u0, v1,
        x1, y1, u1, v1,
        x0, y0, u0, v0,
        x1, y0, u1, v0,
    };
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices + 2);
    glEnableVertexAttribArray(1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

bool Avc420NativeBufferRenderer::EnsureInitialized(std::vector<std::string>& logs)
{
    if (display_ != EGL_NO_DISPLAY && pbufferSurface_ != EGL_NO_SURFACE &&
        context_ != EGL_NO_CONTEXT && program_ != 0) {
        return true;
    }

    Destroy();
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) {
        logs.push_back("AVC420 native-buffer GPU renderer eglGetDisplay failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        Destroy();
        return false;
    }
    if (!eglInitialize(display_, nullptr, nullptr)) {
        logs.push_back("AVC420 native-buffer GPU renderer eglInitialize failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        Destroy();
        return false;
    }
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        logs.push_back("AVC420 native-buffer GPU renderer eglBindAPI failed " +
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
        logs.push_back("AVC420 native-buffer GPU renderer eglChooseConfig failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        Destroy();
        return false;
    }

    pbufferSurface_ = CreatePbufferSurface(logs);
    if (pbufferSurface_ == EGL_NO_SURFACE) {
        Destroy();
        return false;
    }

    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, contextAttribs);
    if (context_ == EGL_NO_CONTEXT) {
        logs.push_back("AVC420 native-buffer GPU renderer eglCreateContext ES3 failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        Destroy();
        return false;
    }
    if (!MakePbufferCurrent(logs)) {
        Destroy();
        return false;
    }

    if (!CreateProgram(logs)) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        Destroy();
        return false;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    logs.push_back("AVC420 native-buffer GPU renderer initialized: "
        "path=OH_NativeBuffer->EGLImage->GL_TEXTURE_EXTERNAL_OES "
        "pbuffer=persistent retainFrames=yes");
    return true;
}

bool Avc420NativeBufferRenderer::CreateProgram(std::vector<std::string>& logs)
{
    static constexpr const char* vertex =
        "#version 300 es\n"
        "layout(location = 0) in vec2 aPosition;\n"
        "layout(location = 1) in vec2 aTexCoord;\n"
        "out vec2 vTexCoord;\n"
        "void main() {\n"
        "  gl_Position = vec4(aPosition, 0.0, 1.0);\n"
        "  vTexCoord = aTexCoord;\n"
        "}\n";
    static constexpr const char* fragment =
        "#version 300 es\n"
        "#extension GL_OES_EGL_image_external_essl3 : require\n"
        "precision highp float;\n"
        "in vec2 vTexCoord;\n"
        "uniform samplerExternalOES uFrame;\n"
        "uniform vec2 uCropScale;\n"
        "uniform vec2 uCropClamp;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "  vec2 uv = min(vTexCoord * uCropScale, uCropClamp);\n"
        "  fragColor = texture(uFrame, uv);\n"
        "}\n";
    static constexpr const char* presentFragment =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 vTexCoord;\n"
        "uniform sampler2D uFrame;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "  fragColor = texture(uFrame, vTexCoord);\n"
        "}\n";

    program_ = LinkProgram(vertex, fragment, logs);
    presentProgram_ = LinkProgram(vertex, presentFragment, logs);
    return program_ != 0 && presentProgram_ != 0;
}

void Avc420NativeBufferRenderer::ConfigureTexture(GLenum target)
{
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

bool Avc420NativeBufferRenderer::MakeCurrent(EGLSurface surface, const char* label, std::vector<std::string>& logs)
{
    if (display_ == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE ||
        context_ == EGL_NO_CONTEXT) {
        logs.push_back("AVC420 native-buffer GPU renderer " +
            std::string(label) + " surface is not initialized");
        return false;
    }
    if (!eglMakeCurrent(display_, surface, surface, context_)) {
        logs.push_back("AVC420 native-buffer GPU renderer eglMakeCurrent(" +
            std::string(label) + ") failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        return false;
    }
    return true;
}

bool Avc420NativeBufferRenderer::MakePbufferCurrent(std::vector<std::string>& logs)
{
    return MakeCurrent(pbufferSurface_, "pbuffer", logs);
}

bool Avc420NativeBufferRenderer::MakeWindowCurrent(std::vector<std::string>& logs)
{
    return MakeCurrent(windowSurface_, "window", logs);
}

bool Avc420NativeBufferRenderer::ImportFrame(const NativeDecodedFrame& frame, ImportedTexture& imported,
    std::vector<std::string>& logs)
{
    if (frame.nativeBuffer == nullptr) {
        ++importFailures_;
        logs.push_back("AVC420 native-buffer GPU import rejected missing OH_NativeBuffer");
        return false;
    }

    const auto createImageKhr =
        reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
    const auto destroyImageKhr =
        reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
    const auto imageTargetTexture =
        reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
            eglGetProcAddress("glEGLImageTargetTexture2DOES"));
    if (createImageKhr == nullptr || destroyImageKhr == nullptr ||
        imageTargetTexture == nullptr) {
        ++importFailures_;
        logs.push_back("AVC420 native-buffer GPU import failed: EGLImage/OES entry point unavailable");
        return false;
    }

    imported.windowBuffer =
        OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(frame.nativeBuffer);
    if (imported.windowBuffer == nullptr) {
        ++importFailures_;
        logs.push_back("AVC420 native-buffer GPU import failed: "
            "OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer returned null " +
            NativeFrameText(frame));
        return false;
    }

    const EGLint imageAttribs[] = {EGL_NONE};
    imported.image = createImageKhr(display_, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_OHOS,
        reinterpret_cast<EGLClientBuffer>(imported.windowBuffer), imageAttribs);
    if (imported.image == EGL_NO_IMAGE_KHR) {
        const EGLint error = eglGetError();
        ReleaseImport(imported);
        ++importFailures_;
        logs.push_back("AVC420 native-buffer GPU import failed: "
            "eglCreateImageKHR(EGL_NATIVE_BUFFER_OHOS) eglError=" +
            Hex32(static_cast<uint32_t>(error)) + " " + NativeFrameText(frame));
        return false;
    }

    glGenTextures(1, &imported.texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, imported.texture);
    ConfigureTexture(GL_TEXTURE_EXTERNAL_OES);
    imageTargetTexture(GL_TEXTURE_EXTERNAL_OES,
        reinterpret_cast<GLeglImageOES>(imported.image));
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR || imported.texture == 0) {
        ReleaseImport(imported);
        ++importFailures_;
        logs.push_back("AVC420 native-buffer GPU import failed: "
            "glEGLImageTargetTexture2DOES glError=" +
            Hex32(static_cast<uint32_t>(error)) + " " + NativeFrameText(frame));
        return false;
    }

    ++imports_;
    return true;
}

void Avc420NativeBufferRenderer::ReleaseImport(ImportedTexture& imported)
{
    if (imported.texture != 0) {
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
        glDeleteTextures(1, &imported.texture);
        imported.texture = 0;
    }
    if (imported.image != EGL_NO_IMAGE_KHR && display_ != EGL_NO_DISPLAY) {
        const auto destroyImageKhr =
            reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(
                eglGetProcAddress("eglDestroyImageKHR"));
        if (destroyImageKhr != nullptr) {
            destroyImageKhr(display_, imported.image);
        }
        imported.image = EGL_NO_IMAGE_KHR;
    }
    if (imported.windowBuffer != nullptr) {
        OH_NativeWindow_DestroyNativeWindowBuffer(imported.windowBuffer);
        imported.windowBuffer = nullptr;
    }
}

} // namespace rdp_bridge
