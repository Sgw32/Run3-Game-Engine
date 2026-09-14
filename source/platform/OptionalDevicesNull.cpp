#include <run3/core/Log.hpp>
#include <run3/platform/OptionalDevices.hpp>

namespace run3 {
namespace {

class NullSerialDevice final : public ISerialDevice {
public:
  bool open(unsigned, unsigned) override {
    logWarning("Serial device support is disabled; using no-op backend");
    return false;
  }
  void close() noexcept override {}
  bool isOpen() const noexcept override { return false; }
  std::size_t read(std::uint8_t *, std::size_t) override { return 0; }
  std::size_t write(const std::uint8_t *, std::size_t) override { return 0; }
};

class NullNamedPipeDevice final : public INamedPipeDevice {
public:
  bool connect(std::string_view) override {
    logWarning("Named-pipe device support is unavailable; using no-op backend");
    return false;
  }
  void close() noexcept override {}
  bool isConnected() const noexcept override { return false; }
  std::size_t read(std::uint8_t *, std::size_t) override { return 0; }
  std::size_t write(const std::uint8_t *, std::size_t) override { return 0; }
};

} // namespace

std::unique_ptr<ISerialDevice> createSerialDevice() {
  return std::make_unique<NullSerialDevice>();
}

std::unique_ptr<INamedPipeDevice> createNamedPipeDevice() {
  return std::make_unique<NullNamedPipeDevice>();
}

} // namespace run3
