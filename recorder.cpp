//=========================================================================================
// KeyBoard Recorder
// @author: Gonzales Cenelia
// homepage: www.ai-search.4t.com
//
// The purpose of these program is to simulate a human typist,the way that these
// program proceed to do this is to first record the time delays between each keyboard
// hit,those time delays are saved into and array and later,they are saved into a file.
// Once the the time delays are saved the program can use them for writing any given text.
// this code is copyrighted and has limited warranty. //****************
//=========================================================================================
#include "recorder.h"

//#define WAIT(x) sleep((double)(x));
#define WAIT(x)
#define SPEED 2.2f
// buffer for saving the string entered by the user
std::string sBuffer = "";
unsigned int numOfLines = 0;
//===========================
// generates a random number
//===========================
int Random( const int val ) {
	if( val == 0 ) return 0;
	else { 
		return rand() % val; 
	}
}

//====================
// chronometing time
//====================
double countTime(void)
{
	/*LARGE_INTEGER liFreq;
    LARGE_INTEGER liStart;
    LARGE_INTEGER liStop;

	//QueryPerformanceCounter(&liStart);
	QueryPerformanceFrequency(&liFreq);

	while(!kbhit())
	{
	}

	QueryPerformanceCounter(&liStop);
	return ((double)(liStop.QuadPart - liStart.QuadPart)/liFreq.QuadPart);*/
	return 0;
}

//========================================
// pause for a specific number of seconds
//========================================
void sleep( double goal ) 
{
	
}

//========================================
// records and saves time delays to file
//========================================
void save_delay(void)
{
	
}

//=============================
// reads time delays from file
//=============================
void read_time_delay(std::vector<double> &time_delay)
{
	
}

//==============================================================
// printing the given string by using prerecorded time delays
//==============================================================
void print_string(const char *string, std::vector<double> time_delay, size_t linePos) {
	
}
