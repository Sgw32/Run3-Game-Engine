#include <run3/core/Log.hpp>

#include <iostream>
#include <mutex>
#include <utility>

namespace run3 {
namespace {

std::mutex sinkMutex;
LogSink sink;

const char *name(LogLevel level) {
  switch (level) {
  case LogLevel::Debug:
    return "debug";
  case LogLevel::Info:
    return "info";
  case LogLevel::Warning:
    return "warning";
  case LogLevel::Error:
    return "error";
  }
  return "unknown";
}

} // namespace

void setLogSink(LogSink newSink) {
  std::lock_guard<std::mutex> lock(sinkMutex);
  sink = std::move(newSink);
}

void log(LogLevel level, std::string_view message) {
  std::lock_guard<std::mutex> lock(sinkMutex);
  if (sink) {
    sink(level, message);
    return;
  }
  std::clog << "[Run3 " << name(level) << "] " << message << '\n';
}

void logInfo(std::string_view message) { log(LogLevel::Info, message); }
void logWarning(std::string_view message) { log(LogLevel::Warning, message); }
void logError(std::string_view message) { log(LogLevel::Error, message); }

} // namespace run3
