#pragma once

#include <functional>
#include <string_view>

namespace run3 {

enum class LogLevel { Debug, Info, Warning, Error };
using LogSink = std::function<void(LogLevel, std::string_view)>;

void setLogSink(LogSink sink);
void log(LogLevel level, std::string_view message);
void logInfo(std::string_view message);
void logWarning(std::string_view message);
void logError(std::string_view message);

} // namespace run3
