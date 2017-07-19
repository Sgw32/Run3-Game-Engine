#include "Run3Benchmark.h"
#include "global.h"

template<> Run3Benchmark *Ogre::Singleton<Run3Benchmark>::ms_Singleton=0;

Run3Benchmark::Run3Benchmark()
{
	iteration=0;
	gameIsLoading=false;
	mapAtBenchMark="";
	currentComment="";
}

Run3Benchmark::~Run3Benchmark()
{
}

void Run3Benchmark::init()
{

}

void Run3Benchmark::benchMarkMap(String map)
{
	LogManager::getSingleton().logMessage("Benchmarking:"+map);
	mapAtBenchMark=map;
	waitAverageFPS=5.0f;
	iteration=0;
}

int Run3Benchmark::benchMarkGetIteration()
{
	return iteration;
}

bool Run3Benchmark::benchMarkIteration(int it)
{
	if (benchMarkEnabled())
	{
		return (it<=iteration);
	}
	return true;
}

void Run3Benchmark::benchMarkStop()
{
	mapAtBenchMark="";
}

void Run3Benchmark::gameStarted()
{
	gameIsLoading=false;
	waitAverageFPS=5.0f;
	iteration++;
}

void Run3Benchmark::setIterationCommentary(String comment)
{
	currentComment=comment;
}

bool Run3Benchmark::benchMarkEnabled()
{
	return !(mapAtBenchMark.empty());
}

void Run3Benchmark::frameStarted(const Ogre::FrameEvent &evt)
{
	if (benchMarkEnabled())
	{
		waitAverageFPS-=evt.timeSinceLastFrame;
		if ((waitAverageFPS<0)&&(!gameIsLoading))
		{
			const RenderTarget::FrameStats& stats = global::getSingleton().getWindow()->getStatistics();
			if (stats.lastFPS>1.0f)
			{
				String fpsString = StringConverter::toString(stats.lastFPS);
				std::stringstream ss;
				ss<<"BMARK,"<<fpsString<<","<<iteration<<","<<currentComment;
				LogManager::getSingleton().logMessage(ss.str());
				//Время загружать новую итерацию.
				gameIsLoading=true;
				global::getSingleton().changemap_now = true;
				global::getSingleton().mMap = mapAtBenchMark;
			}
		}
	}
}
