#pragma once

#include <run3/platform/OptionalDevices.hpp>

#include <memory>

class CSerial {
public:
  CSerial();
  ~CSerial();

  bool Open(int port = 2, int baud = 9600);
  bool Close();
  int ReadData(void *destination, int capacity);
  int SendData(const char *source, int size);
  int ReadDataWaiting();
  bool IsOpened() const;

private:
  std::unique_ptr<run3::ISerialDevice> device_;
};
