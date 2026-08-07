#include "surface/xcomponent_native_host.h"

#include "common/bridge_log.h"
#include "input/xcomponent_input_internal.h"
#include "input/xcomponent_native_gesture.h"
#include "napi/native_bridge_context.h"

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <arkui/native_interface.h>
#include <arkui/native_node.h>
#include <arkui/native_node_napi.h>

#include <mutex>

namespace rdp_bridge {
namespace {

constexpr char kRemoteSurfaceNodeId[] = "rdpRemoteSurface";

class NativeXComponentHost {
public:
    bool Attach(napi_env env, napi_value nodeContentValue, std::string& message)
    {
        ArkUI_NodeContentHandle content = nullptr;
        if (env == nullptr || nodeContentValue == nullptr ||
            OH_ArkUI_GetNodeContentFromNapiValue(env, nodeContentValue, &content) !=
                ARKUI_ERROR_CODE_NO_ERROR ||
            content == nullptr) {
            message = "XComponent NodeContent is invalid";
            BridgeLogger::Error(message);
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (content_ == content && node_ != nullptr && addedToContent_) {
            message = "native XComponent already attached";
            return true;
        }
        DetachLocked();

        OH_ArkUI_GetModuleInterface(ARKUI_NATIVE_NODE, ArkUI_NativeNodeAPI_1, nodeApi_);
        if (nodeApi_ == nullptr) {
            message = "ArkUI Native Node API 1 is unavailable";
            BridgeLogger::Error(message);
            return false;
        }
        content_ = content;
        node_ = nodeApi_->createNode(ARKUI_NODE_XCOMPONENT);
        if (node_ == nullptr || !ConfigureNodeAttributes()) {
            message = "failed to create or configure native XComponent node";
            BridgeLogger::Error(message);
            DetachLocked();
            return false;
        }

        component_ = OH_NativeXComponent_GetNativeXComponent(node_);
        if (component_ == nullptr || !RegisterNativeXComponentInstance(component_)) {
            message = "failed to register native XComponent callbacks";
            BridgeLogger::Error(message);
            DetachLocked();
            return false;
        }

        if (!BindXComponentNativeGestures(node_, message)) {
            DetachLocked();
            return false;
        }
        gesturesBound_ = true;
        if (OH_ArkUI_NodeContent_AddNode(content_, node_) != ARKUI_ERROR_CODE_NO_ERROR) {
            message = "failed to add native XComponent node to NodeContent";
            BridgeLogger::Error(message);
            DetachLocked();
            return false;
        }
        addedToContent_ = true;
        message = "native XComponent node and system gestures attached";
        BridgeLogger::Info(message);
        return true;
    }

    void Detach()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        DetachLocked();
    }

private:
    bool SetNumberAttribute(ArkUI_NodeAttributeType type, ArkUI_NumberValue value)
    {
        const ArkUI_AttributeItem item {&value, 1, nullptr, nullptr};
        return nodeApi_->setAttribute(node_, type, &item) == ARKUI_ERROR_CODE_NO_ERROR;
    }

    bool SetStringAttribute(ArkUI_NodeAttributeType type, const char* value)
    {
        const ArkUI_AttributeItem item {nullptr, 0, value, nullptr};
        return nodeApi_->setAttribute(node_, type, &item) == ARKUI_ERROR_CODE_NO_ERROR;
    }

    bool ConfigureNodeAttributes()
    {
        ArkUI_NumberValue percent {};
        percent.f32 = 1.0f;
        ArkUI_NumberValue enabled {};
        enabled.i32 = 1;
        ArkUI_NumberValue black {};
        black.u32 = 0xFF000000;
        ArkUI_NumberValue renderFit {};
        renderFit.i32 = ARKUI_RENDER_FIT_CENTER;
        return SetNumberAttribute(NODE_WIDTH_PERCENT, percent) &&
            SetNumberAttribute(NODE_HEIGHT_PERCENT, percent) &&
            SetNumberAttribute(NODE_BACKGROUND_COLOR, black) &&
            SetNumberAttribute(NODE_FOCUSABLE, enabled) &&
            SetNumberAttribute(NODE_FOCUS_ON_TOUCH, enabled) &&
            SetNumberAttribute(NODE_DEFAULT_FOCUS, enabled) &&
            SetNumberAttribute(NODE_RENDER_FIT, renderFit) &&
            SetStringAttribute(NODE_ID, kRemoteSurfaceNodeId);
    }

    void DetachLocked()
    {
        if (node_ == nullptr && content_ == nullptr) {
            return;
        }
        ReleaseAllXComponentInput("nativeXComponentDetach");
        if (addedToContent_ && content_ != nullptr && node_ != nullptr) {
            (void)OH_ArkUI_NodeContent_RemoveNode(content_, node_);
        }
        if (gesturesBound_) {
            UnbindXComponentNativeGestures();
        }
        if (nodeApi_ != nullptr && node_ != nullptr) {
            nodeApi_->disposeNode(node_);
        }
        content_ = nullptr;
        node_ = nullptr;
        component_ = nullptr;
        addedToContent_ = false;
        gesturesBound_ = false;
    }

    std::mutex mutex_;
    ArkUI_NativeNodeAPI_1* nodeApi_ = nullptr;
    ArkUI_NodeContentHandle content_ = nullptr;
    ArkUI_NodeHandle node_ = nullptr;
    OH_NativeXComponent* component_ = nullptr;
    bool addedToContent_ = false;
    bool gesturesBound_ = false;
};

NativeXComponentHost& XComponentHost()
{
    static NativeXComponentHost host;
    return host;
}

} // namespace

bool AttachNativeXComponentContent(napi_env env, napi_value nodeContent, std::string& message)
{
    return XComponentHost().Attach(env, nodeContent, message);
}

void DetachNativeXComponentContent()
{
    XComponentHost().Detach();
}

} // namespace rdp_bridge
