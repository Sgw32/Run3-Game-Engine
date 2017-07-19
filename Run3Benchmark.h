/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include <OIS/OIS.h>
#include <stdlib.h>
#include <time.h>

using namespace Ogre;

class Run3Benchmark: public Singleton<Run3Benchmark>
{
public:
   Run3Benchmark();
   ~Run3Benchmark();
   void init();
   void benchMarkMap(String map);
   void gameStarted();
   void benchMarkStop();
   bool benchMarkEnabled();
   int benchMarkGetIteration();
   bool benchMarkIteration(int it);
   void setIterationCommentary(String comment);
   void frameStarted(const Ogre::FrameEvent &evt);
private:
	Real waitAverageFPS;
	bool gameIsLoading;
	int iteration;	
	String mapAtBenchMark;
	String currentComment;
};