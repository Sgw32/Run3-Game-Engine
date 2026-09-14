#pragma once

#include <run3/platform/OptionalDevices.hpp>

#include <Ogre.h>

#include <memory>
#include <string>

class NamedPipeServer {
public:
  explicit NamedPipeServer(Ogre::String managerName);
  NamedPipeServer();
  virtual ~NamedPipeServer();
  virtual void init();

  void setCurrentPitchAndRoll(float pitch, float roll) {
    pitch_ = pitch;
    roll_ = roll;
  }
  int createNamedPipe();
  int WriteToPipe(std::string data);
  int readMessage();
  void parseToMotorValues(std::string data);
  float getMotor1() const { return motors_[0]; }
  float getMotor2() const { return motors_[1]; }
  float getMotor3() const { return motors_[2]; }
  float getMotor4() const { return motors_[3]; }
  float getPitch() const { return pitch_; }
  float getRoll() const { return roll_; }
  virtual void upd(const Ogre::FrameEvent &event);
  virtual void upd(float elapsed);
  virtual void cleanup();

private:
  float pitch_{};
  float roll_{};
  float motors_[4]{};
  std::unique_ptr<run3::INamedPipeDevice> device_;
};
