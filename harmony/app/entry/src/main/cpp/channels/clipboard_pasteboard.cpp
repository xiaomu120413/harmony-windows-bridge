#include "channels/clipboard_pasteboard.h"

#include "bridge_log.h"

#include <database/pasteboard/oh_pasteboard_err_code.h>
#include <database/udmf/udmf.h>
#include <database/udmf/udmf_err_code.h>
#include <database/udmf/uds.h>

namespace rdp_bridge {

ClipboardPasteboard::~ClipboardPasteboard()
{
    Uninitialize();
}

bool ClipboardPasteboard::Initialize(const LogFn& log, const ChangeFn& onLocalChange)
{
    log_ = log;
    onLocalChange_ = onLocalChange;

    pasteboard_ = OH_Pasteboard_Create();
    if (pasteboard_ == nullptr) {
        Log("HarmonyOS Pasteboard create failed; cliprdr will advertise no local text");
        return true;
    }
    Log("HarmonyOS Pasteboard created for cliprdr text bridge");

    observer_ = OH_PasteboardObserver_Create();
    if (observer_ == nullptr) {
        Log("HarmonyOS Pasteboard observer create failed; local clipboard changes require reconnect");
        return true;
    }

    int rc = OH_PasteboardObserver_SetData(observer_, this, OnPasteboardChanged, OnPasteboardFinalize);
    if (rc != ERR_OK) {
        Log("HarmonyOS Pasteboard observer setup failed: " + std::to_string(rc));
        return true;
    }

    rc = OH_Pasteboard_Subscribe(pasteboard_, NOTIFY_LOCAL_DATA_CHANGE, observer_);
    if (rc == ERR_OK) {
        subscribed_ = true;
        Log("HarmonyOS Pasteboard observer subscribed");
    } else {
        Log("HarmonyOS Pasteboard subscribe warning: " + std::to_string(rc));
    }

    return true;
}

void ClipboardPasteboard::Uninitialize()
{
    if (pasteboard_ != nullptr && observer_ != nullptr && subscribed_) {
        (void)OH_Pasteboard_Unsubscribe(pasteboard_, NOTIFY_LOCAL_DATA_CHANGE, observer_);
        subscribed_ = false;
    }
    if (observer_ != nullptr) {
        (void)OH_PasteboardObserver_Destroy(observer_);
        observer_ = nullptr;
    }
    if (pasteboard_ != nullptr) {
        OH_Pasteboard_Destroy(pasteboard_);
        pasteboard_ = nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        ignoreLocalChanges_ = 0;
    }
    onLocalChange_ = nullptr;
    log_ = nullptr;
}

bool ClipboardPasteboard::ReadPlainText(std::string& text, std::string& error)
{
    text.clear();
    if (pasteboard_ == nullptr) {
        error = "pasteboard unavailable";
        return false;
    }

    int status = ERR_OK;
    OH_UdmfData* data = OH_Pasteboard_GetData(pasteboard_, &status);
    if (status != ERR_OK || data == nullptr) {
        error = "OH_Pasteboard_GetData status=" + std::to_string(status);
        return false;
    }

    OH_UdsPlainText* primaryPlainText = OH_UdsPlainText_Create();
    if (primaryPlainText != nullptr) {
        const int primaryRc = OH_UdmfData_GetPrimaryPlainText(data, primaryPlainText);
        if (primaryRc == UDMF_E_OK) {
            const char* content = OH_UdsPlainText_GetContent(primaryPlainText);
            if (content != nullptr) {
                text = content;
            }
        }
        OH_UdsPlainText_Destroy(primaryPlainText);
        if (!text.empty()) {
            OH_UdmfData_Destroy(data);
            return true;
        }
    }

    const int recordCount = OH_UdmfData_GetRecordCount(data);
    for (int index = 0; index < recordCount; ++index) {
        OH_UdmfRecord* record = OH_UdmfData_GetRecord(data, static_cast<unsigned int>(index));
        if (record == nullptr) {
            continue;
        }
        OH_UdsPlainText* plainText = OH_UdsPlainText_Create();
        if (plainText == nullptr) {
            continue;
        }
        const int rc = OH_UdmfRecord_GetPlainText(record, plainText);
        if (rc == UDMF_E_OK) {
            const char* content = OH_UdsPlainText_GetContent(plainText);
            if (content != nullptr) {
                text = content;
            }
        }
        OH_UdsPlainText_Destroy(plainText);
        if (!text.empty()) {
            break;
        }
    }

    OH_UdmfData_Destroy(data);
    if (text.empty()) {
        error = "pasteboard has no plain text record";
        return false;
    }
    return true;
}

bool ClipboardPasteboard::WritePlainText(const std::string& text, std::string& error)
{
    if (pasteboard_ == nullptr) {
        error = "pasteboard unavailable";
        return false;
    }

    OH_UdsPlainText* plainText = OH_UdsPlainText_Create();
    OH_UdmfRecord* record = OH_UdmfRecord_Create();
    OH_UdmfData* data = OH_UdmfData_Create();
    if (plainText == nullptr || record == nullptr || data == nullptr) {
        error = "UDMF allocation failed";
        if (plainText != nullptr) {
            OH_UdsPlainText_Destroy(plainText);
        }
        if (record != nullptr) {
            OH_UdmfRecord_Destroy(record);
        }
        if (data != nullptr) {
            OH_UdmfData_Destroy(data);
        }
        return false;
    }

    int rc = OH_UdsPlainText_SetContent(plainText, text.c_str());
    if (rc == UDMF_E_OK) {
        rc = OH_UdmfRecord_AddPlainText(record, plainText);
    }
    if (rc == UDMF_E_OK) {
        rc = OH_UdmfData_AddRecord(data, record);
    }
    if (rc != UDMF_E_OK) {
        error = "UDMF plain text setup failed: " + std::to_string(rc);
        OH_UdsPlainText_Destroy(plainText);
        OH_UdmfRecord_Destroy(record);
        OH_UdmfData_Destroy(data);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++ignoreLocalChanges_;
    }
    rc = OH_Pasteboard_SetData(pasteboard_, data);
    OH_UdsPlainText_Destroy(plainText);
    OH_UdmfRecord_Destroy(record);
    OH_UdmfData_Destroy(data);
    if (rc != ERR_OK) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (ignoreLocalChanges_ > 0) {
            --ignoreLocalChanges_;
        }
        error = "OH_Pasteboard_SetData status=" + std::to_string(rc);
        return false;
    }
    return true;
}

void ClipboardPasteboard::OnPasteboardChanged(void* context, Pasteboard_NotifyType type)
{
    auto* bridge = static_cast<ClipboardPasteboard*>(context);
    if (bridge != nullptr) {
        bridge->HandlePasteboardChanged(type);
    }
}

void ClipboardPasteboard::OnPasteboardFinalize(void*)
{
}

void ClipboardPasteboard::HandlePasteboardChanged(Pasteboard_NotifyType type)
{
    if (type != NOTIFY_LOCAL_DATA_CHANGE) {
        return;
    }

    ChangeFn onLocalChange;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (ignoreLocalChanges_ > 0) {
            --ignoreLocalChanges_;
            return;
        }
        onLocalChange = onLocalChange_;
    }
    if (onLocalChange) {
        onLocalChange();
    }
}

void ClipboardPasteboard::Log(const std::string& line)
{
    if (log_) {
        log_(line);
    } else {
        EmitHilogInfo(line);
    }
}

} // namespace rdp_bridge
