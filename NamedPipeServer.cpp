#include "NamedPipeServer.h"

//template<> NamedPipeServer *Singleton<NamedPipeServer>::ms_Singleton=0;

NamedPipeServer::NamedPipeServer()
{
	mPitch=0;
	mRoll=0;
	motor1=motor2=motor3=motor4=0;
}

NamedPipeServer::~NamedPipeServer()
{
	CloseHandle(hPipe);
}

void NamedPipeServer::init()
{
	createNamedPipe();
}


int NamedPipeServer::createNamedPipe()
{
	hPipe = CreateNamedPipe( 
          g_szPipeName,             // pipe name 
          PIPE_ACCESS_DUPLEX,       // read/write access 
          PIPE_TYPE_MESSAGE |       // message type pipe 
          PIPE_READMODE_MESSAGE |   // message-read mode 
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          BUFFER_SIZE,              // output buffer size 
          BUFFER_SIZE,              // input buffer size 
          NMPWAIT_USE_DEFAULT_WAIT, // client time-out 
          NULL);                    // default security attribute 
     
     if (INVALID_HANDLE_VALUE == hPipe) 
     {
		 LogManager::getSingleton().logMessage("Error occurred while creating the pipe.");
          //printf("\nError occurred while " 
         //        "creating the pipe: %d", GetLastError()); 
          return 1;  //Error
     }
     else
     {
		  LogManager::getSingleton().logMessage("CreateNamedPipe() was successful.");
        //  printf("\nCreateNamedPipe() was successful.");
     }

     LogManager::getSingleton().logMessage("Waiting for client.");
    // printf("\nWaiting for client connection...");
     
     //Wait for the client to connect
     BOOL bClientConnected = ConnectNamedPipe(hPipe, NULL);
     
     if (FALSE == bClientConnected)
     {
		 LogManager::getSingleton().logMessage("Error occurred while connecting.");
          //printf("\nError occurred while connecting" 
          //       " to the client: %d", GetLastError()); 
          CloseHandle(hPipe);
          return 1;  //Error
     }
     else
     {
		 LogManager::getSingleton().logMessage("ConnectNamedPipe() was successful.");
          //printf("\nConnectNamedPipe() was successful.");
     }
	// system("PAUSE");
	 return TRUE;
}


int NamedPipeServer::WriteToPipe(string data)
{
	DWORD cbBytes;
	BOOL bResult;

	bResult = WriteFile( 
          hPipe,                // handle to pipe 
          data.c_str(),             // buffer to write from 
          strlen(data.c_str())+1,   // number of bytes to write, include the NULL 
          &cbBytes,             // number of bytes written 
          NULL);                // not overlapped I/O 
     

     if ( (!bResult) || (strlen(data.c_str())+1 != cbBytes))
     {
          LogManager::getSingleton().logMessage("\nError occurred while writing to the client!");
          //CloseHandle(hPipe);
          return 1;  //Error
     }
     else
     {
          LogManager::getSingleton().logMessage("STRING " + data);
     }
	 return 0;
}


int NamedPipeServer::readMessage()
{
	BOOL bResult;
	char szBuffer[BUFFER_SIZE];
	DWORD cbBytes;

	bResult = ReadFile( 
          hPipe,                // handle to pipe 
          szBuffer,             // buffer to receive data 
          sizeof(szBuffer),     // size of buffer 
          &cbBytes,             // number of bytes read 
          NULL);                // not overlapped I/O 
     
     if ( (!bResult) || (0 == cbBytes)) 
     {
       //   printf("\nError occurred while reading" 
        //         " from the server: %d", GetLastError()); 
          //CloseHandle(hPipe);
		 LogManager::getSingleton().logMessage("No data!");
          return 0;  //Error
     }
     else
     {
		 //LogManager::getSingleton().logMessage("ReadFile() was successful.");
     }
	 LogManager::getSingleton().logMessage("MSG NAMED PIPE:");
	  string data = string(szBuffer);
	  //LogManager::getSingleton().logMessage(data);
	  parseToMotorValues(data);
     //printf("\nServer sent the following message: %s", szBuffer);
	// cout<<endl;
	 return 1;
}

void NamedPipeServer::parseToMotorValues(string dta)
{
	 string parsed,input=dta;
	 stringstream input_stringstream(input);

	 if(getline(input_stringstream,parsed,' '))
	{
		 // do some processing.
		motor1=StringConverter::parseReal(parsed);
	}

	  if(getline(input_stringstream,parsed,' '))
	{
		 // do some processing.
		motor2=StringConverter::parseReal(parsed);
	}

	 if(getline(input_stringstream,parsed,' '))
	{
		 // do some processing.
		motor3=StringConverter::parseReal(parsed);
	}

	 if(getline(input_stringstream,parsed,' '))
	{
		 // do some processing.
		motor4=StringConverter::parseReal(parsed);
	}
	LogManager::getSingleton().logMessage("motor1234 "+StringConverter::toString(motor1)+
		" "+StringConverter::toString(motor2)+
		" "+StringConverter::toString(motor3)+
		" "+StringConverter::toString(motor4));
}

void NamedPipeServer::upd(const FrameEvent &evt)
{
	stringstream ss;
	ss<<mPitch<<" "<<mRoll;
	WriteToPipe(ss.str());
	readMessage();
}

void NamedPipeServer::upd(float evt)
{
	stringstream ss;
	int sp,sr;
	sp=(int)(mPitch*1000);
	sr=(int)(mRoll*1000);
	ss<<sp<<" "<<sr;
	WriteToPipe(ss.str());
	readMessage();
}

void NamedPipeServer::cleanup()
{

}