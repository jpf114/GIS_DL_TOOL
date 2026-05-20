#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>

#include <mutex>
#include <functional>
#include <string>

namespace gis_ai::gui {

template<typename Mutex>
class GuiLogSink : public spdlog::sinks::base_sink<Mutex> {
public:
    using LogCallback = std::function<void(const std::string& message, spdlog::level::level_enum level)>;

    void setCallback(LogCallback callback) {
        callback_ = std::move(callback);
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (!callback_) return;

        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

        std::string text(formatted.data(), formatted.size());
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
            text.pop_back();

        callback_(text, msg.level);
    }

    void flush_() override {}

private:
    LogCallback callback_;
};

using GuiLogSinkMt = GuiLogSink<std::mutex>;
using GuiLogSinkSt = GuiLogSink<spdlog::details::null_mutex>;

}
