#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

// Shared AVC GPU compositor utilities extracted from
// avc420_gpu_compositor_internal.cpp and avc444_gpu_compositor_internal.cpp
// to eliminate verbatim duplication (M11). Behaviour is unchanged; the symbols
// previously lived in each file's anonymous namespace and are now provided
// once under rdp_bridge so both translation units reference the same definition.

namespace rdp_bridge {

constexpr const char* kAvcMime = "video/avc";
constexpr int64_t kInputTimeoutUs = 20000;
constexpr uint64_t kTimingSampleInterval = 60U;

bool ShouldLogFrequent(uint64_t count);
bool ShouldSampleTiming(uint64_t count);
uint64_t NowMicros();

uint32_t ReadBe32(const uint8_t* data);
bool FindAnnexBStartCode(const uint8_t* data, uint32_t size, uint32_t from,
    uint32_t& start, uint32_t& prefixLength);
bool ExtractH264ParameterSets(const uint8_t* data, uint32_t size,
    std::vector<uint8_t>& parameterSets, std::string* nalSummary);

struct TimingBucket {
    uint64_t count = 0;
    uint64_t totalUs = 0;
    uint64_t maxUs = 0;

    void Add(uint64_t valueUs)
    {
        ++count;
        totalUs += valueUs;
        maxUs = std::max(maxUs, valueUs);
    }

    void Reset()
    {
        count = 0;
        totalUs = 0;
        maxUs = 0;
    }

    std::string Text(const char* name) const
    {
        std::ostringstream out;
        out << name << ":";
        if (count == 0) {
            out << "0/0/0";
        } else {
            out << (totalUs / count) << "/" << maxUs << "/" << count;
        }
        return out.str();
    }
};

class ScopedTiming {
public:
    ScopedTiming(TimingBucket& bucket, bool enabled)
        : bucket_(enabled ? &bucket : nullptr), startUs_(enabled ? NowMicros() : 0)
    {}
    ScopedTiming(TimingBucket& bucket, uint64_t startUs, bool enabled)
        : bucket_(enabled ? &bucket : nullptr), startUs_(enabled ? startUs : 0)
    {}

    ~ScopedTiming()
    {
        if (bucket_ == nullptr) {
            return;
        }
        const uint64_t nowUs = NowMicros();
        bucket_->Add(nowUs >= startUs_ ? nowUs - startUs_ : 0);
    }

private:
    TimingBucket* bucket_ = nullptr;
    uint64_t startUs_ = 0;
};

} // namespace rdp_bridge
