#pragma once
#include <CEGUI/CEGUISchemeManager.h>
#include <CEGUI/CEGUIWindowManager.h>
#include <CEGUI/CEGUIWindow.h>
#include <CEGUI/CEGUI.h>
#include <vector>

using namespace std;

//9
#define MAX_CHAPTERS 8
#define SIMPLE_TEST
class ChapterController
{
public:
	ChapterController()
	{
		curChapter=0;
		busy=0;
		init=false;
	}
	~ChapterController()
	{
	}

	void setPrevNextWindows(CEGUI::Window* prev,CEGUI::Window* next)
	{
		nextButton=next;
		prevButton=prev;
	}
	void pushBackChapter(CEGUI::Window* win,String map)
	{
		chapterWindows.push_back(win);
		chapterMaps.push_back(map);
	}
	void step(Real dt)
	{
#ifndef SIMPLE_TEST
		busy-=dt;
		if (busy<0)
		{
			busy=0;
			if (nextImage!=curImage)
			{
				LogManager::getSingleton().logMessage("Chapter changed!!!");
				curImage->hide();
				curImage=nextImage;
				curImage->show();
			}
		}
		if ((nextImage!=curImage))
		{
			CEGUI::URect ar = curImage->getArea();
			CEGUI::UVector2 sz = ar.getSize();
			sz=sz*CEGUI::UVector2(CEGUI::UDim(1,0),CEGUI::UDim(1-dt,0));
			CEGUI::URect ne=CEGUI::URect(ar.getPosition(),ar.getPosition()+sz);
			curImage->setArea(ne);

			//curImage->setSize(CEGUI::UVector2(size.d_x,size.d_y));//*CEGUI::UDim(busy,busy)));
			//nextImage->setSize(CEGUI::UVector2(size.d_x,size.d_y));//-size.d_y*CEGUI::UDim(busy,busy)));
		}
#endif
	}
	String getMapToLoad()
	{
		return chapterMaps[curChapter];
	}

	void finalize()
	{
		vector<CEGUI::Window*>::iterator i;
		for (i=chapterWindows.begin();i!=chapterWindows.end();i++)
			(*i)->hide();
		curImage=chapterWindows[0];
		nextImage=curImage;
		curImage->show();
		init=true;
	}

	void checkNoNext()
	{
		if (curChapter==MAX_CHAPTERS)
		{
			nextButton->hide();
		}
		else
		{
			nextButton->show();
		}
	}
	void checkNoPrev()
	{
		if (!curChapter)
		{
			prevButton->hide();
		}
		else
		{
			prevButton->show();
		}
	}
	bool getBusy()
	{
		return busy!=0;
	}
	void nextChapter()
	{
		

#ifdef SIMPLE_TEST
		curImage->hide();
		curChapter++;
		if (curChapter>MAX_CHAPTERS)
			curChapter=MAX_CHAPTERS;
		checkNoNext();
		checkNoPrev();
		curImage=chapterWindows[curChapter];
		curImage->show();
#else
		busy=1.0f;
		curChapter++;
		if (curChapter>MAX_CHAPTERS)
			curChapter=MAX_CHAPTERS;
		checkNoNext();
		checkNoPrev();
		nextImage=chapterWindows[curChapter];
#endif
	}
	void prevChapter()
	{
		

#ifdef SIMPLE_TEST
		curImage->hide();
		curChapter--;
		if (curChapter<0)
			curChapter=0;
		checkNoPrev();
		checkNoNext();
		curImage=chapterWindows[curChapter];
		curImage->show();
#else
		busy=1.0f;
		curChapter--;
		if (curChapter<0)
			curChapter=0;
		checkNoPrev();
		checkNoNext();
		nextImage=chapterWindows[curChapter];
		size=curImage->getSize();
#endif

	}
private:
	bool init;
	Real busy;
	CEGUI::UVector2 size;
	int curChapter;
	CEGUI::Window* nextButton;
	CEGUI::Window* prevButton;
	CEGUI::Window* curImage;
	CEGUI::Window* nextImage;
	vector<CEGUI::Window*> chapterWindows;
	vector<String> chapterMaps;
};