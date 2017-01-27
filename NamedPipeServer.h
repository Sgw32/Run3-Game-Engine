#pragma once
#include <Ogre.h>
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <strsafe.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "Manager_Template.h"

using namespace std;
using namespace Ogre;
//Name given to the pipe
#define g_szPipeName "\\\\.\\Pipe\\Tiltrotor"
//Pipe name format - \\.\pipe\pipename

#define BUFFER_SIZE 1024 //1k
#define ACK_MESG_RECV "Message received successfully"

//class NamedPipeServer : public Singleton<NamedPipeServer>, public managerTemplate
class NamedPipeServer
{
public:
	NamedPipeServer(String manName){LogManager::getSingleton().logMessage(manName+" manager initialized!");mPitch=0;
	mRoll=0;}
	NamedPipeServer();
	virtual ~NamedPipeServer();
	virtual void init();

	void setCurrentPitchAndRoll(float pitch, float roll)
	{
		mPitch=pitch;
		mRoll=roll;
	}

	int createNamedPipe();
	int WriteToPipe(string data);
	int readMessage();

	void parseToMotorValues(string dta);

	float getMotor1(){return motor1;};
	float getMotor2(){return motor2;};
	float getMotor3(){return motor3;};
	float getMotor4(){return motor4;};

	float getPitch();
	float getRoll();

	virtual void upd(const FrameEvent& evt);
	virtual void upd(float evt);
	virtual void cleanup();
private:
	float mPitch,mRoll;
	float motor1,motor2,motor3,motor4;
	HANDLE hPipe;
};