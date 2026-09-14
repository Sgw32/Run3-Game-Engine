#include <run3/core/Log.hpp>
#include <run3/platform/OptionalDevices.hpp>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <algorithm>
#include <string>

namespace run3 {
namespace {

class Win32SerialDevice final : public ISerialDevice {
public:
  ~Win32SerialDevice() override { close(); }

  bool open(unsigned port, unsigned baud) override {
    close();
    const std::wstring name = L"\\\\.\\COM" + std::to_wstring(port);
    handle_ = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle_ == INVALID_HANDLE_VALUE) {
      logError("Unable to open requested Win32 serial port");
      return false;
    }
    DCB settings{};
    settings.DCBlength = sizeof(settings);
    if (!GetCommState(handle_, &settings)) {
      close();
      return false;
    }
    settings.BaudRate = baud;
    settings.ByteSize = 8;
    settings.Parity = NOPARITY;
    settings.StopBits = ONESTOPBIT;
    if (!SetCommState(handle_, &settings)) {
      close();
      return false;
    }
    return true;
  }

  void close() noexcept override {
    if (handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
      handle_ = INVALID_HANDLE_VALUE;
    }
  }
  bool isOpen() const noexcept override { return handle_ != INVALID_HANDLE_VALUE; }
  std::size_t read(std::uint8_t *destination, std::size_t capacity) override {
    if (!isOpen() || destination == nullptr || capacity == 0) return 0;
    DWORD count{};
    const DWORD requested = static_cast<DWORD>(
        std::min<std::size_t>(capacity, MAXDWORD));
    return ReadFile(handle_, destination, requested, &count, nullptr) ? count : 0;
  }
  std::size_t write(const std::uint8_t *source, std::size_t size) override {
    if (!isOpen() || source == nullptr || size == 0) return 0;
    DWORD count{};
    const DWORD requested = static_cast<DWORD>(std::min<std::size_t>(size, MAXDWORD));
    return WriteFile(handle_, source, requested, &count, nullptr) ? count : 0;
  }

private:
  HANDLE handle_{INVALID_HANDLE_VALUE};
};

class Win32NamedPipeDevice final : public INamedPipeDevice {
public:
  ~Win32NamedPipeDevice() override { close(); }

  bool connect(std::string_view name) override {
    close();
    const std::string fullName = "\\\\.\\pipe\\" + std::string(name);
    handle_ = CreateNamedPipeA(
        fullName.c_str(), PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 1024, 1024,
        NMPWAIT_USE_DEFAULT_WAIT, nullptr);
    if (handle_ == INVALID_HANDLE_VALUE) {
      logError("Unable to create requested Win32 named pipe");
      return false;
    }
    const bool connected = ConnectNamedPipe(handle_, nullptr) != FALSE ||
                           GetLastError() == ERROR_PIPE_CONNECTED;
    if (!connected) {
      logError("Unable to connect requested Win32 named pipe client");
      close();
    }
    return connected;
  }
  void close() noexcept override {
    if (handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
      handle_ = INVALID_HANDLE_VALUE;
    }
  }
  bool isConnected() const noexcept override {
    return handle_ != INVALID_HANDLE_VALUE;
  }
  std::size_t read(std::uint8_t *destination, std::size_t capacity) override {
    if (!isConnected() || destination == nullptr || capacity == 0) return 0;
    DWORD count{};
    const DWORD requested = static_cast<DWORD>(
        std::min<std::size_t>(capacity, MAXDWORD));
    return ReadFile(handle_, destination, requested, &count, nullptr) ? count : 0;
  }
  std::size_t write(const std::uint8_t *source, std::size_t size) override {
    if (!isConnected() || source == nullptr || size == 0) return 0;
    DWORD count{};
    const DWORD requested = static_cast<DWORD>(std::min<std::size_t>(size, MAXDWORD));
    return WriteFile(handle_, source, requested, &count, nullptr) ? count : 0;
  }

private:
  HANDLE handle_{INVALID_HANDLE_VALUE};
};

} // namespace

std::unique_ptr<ISerialDevice> createSerialDevice() {
  return std::make_unique<Win32SerialDevice>();
}

std::unique_ptr<INamedPipeDevice> createNamedPipeDevice() {
  return std::make_unique<Win32NamedPipeDevice>();
}

} // namespace run3
