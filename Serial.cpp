#include "Serial.h"

#include <algorithm>
#include <cstdint>

CSerial::CSerial() : device_(run3::createSerialDevice()) {}
CSerial::~CSerial() { Close(); }

bool CSerial::Open(int port, int baud) {
  return port > 0 && baud > 0 &&
         device_->open(static_cast<unsigned>(port), static_cast<unsigned>(baud));
}

bool CSerial::Close() {
  device_->close();
  return true;
}

int CSerial::ReadData(void *destination, int capacity) {
  if (capacity <= 0) return 0;
  return static_cast<int>(device_->read(static_cast<std::uint8_t *>(destination),
                                        static_cast<std::size_t>(capacity)));
}

int CSerial::SendData(const char *source, int size) {
  if (size <= 0) return 0;
  return static_cast<int>(device_->write(
      reinterpret_cast<const std::uint8_t *>(source),
      static_cast<std::size_t>(size)));
}

int CSerial::ReadDataWaiting() {
  // The portable interface intentionally avoids backend-specific queue APIs.
  return 0;
}

bool CSerial::IsOpened() const { return device_->isOpen(); }
