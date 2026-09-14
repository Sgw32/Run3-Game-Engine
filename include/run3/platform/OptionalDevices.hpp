#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace run3 {

class ISerialDevice {
public:
  virtual ~ISerialDevice() = default;
  virtual bool open(unsigned port, unsigned baud) = 0;
  virtual void close() noexcept = 0;
  virtual bool isOpen() const noexcept = 0;
  virtual std::size_t read(std::uint8_t *destination, std::size_t capacity) = 0;
  virtual std::size_t write(const std::uint8_t *source, std::size_t size) = 0;
};

class INamedPipeDevice {
public:
  virtual ~INamedPipeDevice() = default;
  virtual bool connect(std::string_view name) = 0;
  virtual void close() noexcept = 0;
  virtual bool isConnected() const noexcept = 0;
  virtual std::size_t read(std::uint8_t *destination, std::size_t capacity) = 0;
  virtual std::size_t write(const std::uint8_t *source, std::size_t size) = 0;
};

std::unique_ptr<ISerialDevice> createSerialDevice();
std::unique_ptr<INamedPipeDevice> createNamedPipeDevice();

} // namespace run3
