#include "NamedPipeServer.h"

#include <run3/core/Log.hpp>

#include <array>
#include <cstdint>
#include <sstream>

namespace {
constexpr std::size_t bufferSize = 1024;
}

NamedPipeServer::NamedPipeServer(Ogre::String managerName)
    : device_(run3::createNamedPipeDevice()) {
  run3::logInfo(managerName + " manager initialized");
}

NamedPipeServer::NamedPipeServer()
    : device_(run3::createNamedPipeDevice()) {}

NamedPipeServer::~NamedPipeServer() { cleanup(); }
void NamedPipeServer::init() { createNamedPipe(); }

int NamedPipeServer::createNamedPipe() {
  return device_->connect("Tiltrotor") ? 0 : 1;
}

int NamedPipeServer::WriteToPipe(std::string data) {
  const auto *bytes = reinterpret_cast<const std::uint8_t *>(data.data());
  return device_->write(bytes, data.size()) == data.size() ? 0 : 1;
}

int NamedPipeServer::readMessage() {
  std::array<std::uint8_t, bufferSize> bytes{};
  const std::size_t count = device_->read(bytes.data(), bytes.size() - 1);
  if (count == 0) return 0;
  parseToMotorValues(std::string(reinterpret_cast<const char *>(bytes.data()), count));
  return 1;
}

void NamedPipeServer::parseToMotorValues(std::string data) {
  std::istringstream stream(std::move(data));
  for (float &motor : motors_) {
    if (!(stream >> motor)) {
      run3::logWarning("Named-pipe motor message is incomplete");
      return;
    }
  }
}

void NamedPipeServer::upd(const Ogre::FrameEvent &) { upd(0.0F); }

void NamedPipeServer::upd(float) {
  std::ostringstream stream;
  stream << pitch_ << ' ' << roll_;
  if (WriteToPipe(stream.str()) == 0) readMessage();
}

void NamedPipeServer::cleanup() { device_->close(); }
