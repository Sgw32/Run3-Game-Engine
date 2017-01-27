#include "CrosshairOp.h"
#include "global.h"
#include "HUD.h"

template<> CrosshairOperator *Ogre::Singleton<CrosshairOperator>::ms_Singleton=0;

CrosshairOperator::CrosshairOperator() 
{
	startDecreasing=false;
}

CrosshairOperator::~CrosshairOperator()
{
}

void CrosshairOperator::init()
{
}

Vector3 CrosshairOperator::getCrosshairShift(void)
{
	
	Real cx = global::getSingleton().getPlayer()->camera_rotation_x*5;
	Real cy = global::getSingleton().getPlayer()->camera_rotation_y*5;
	if ((x<0)&&(cx<x))
		x=cx;
	if ((y<0)&&(cy<y))
		y=cy;
	if ((x>0)&&(cx>x))
		x=cx;
	if ((y>0)&&(cy>y))
		y=cy;

	pauseBeforeDecrease=1.0f;
	//HUD::getSingleton().setCrosshairShiftTopLeft(x,y);
	return Vector3::ZERO;
}

Vector3 CrosshairOperator::getCrosshairShift(Real x1,Real y1)
{
	Real cx = x1;
	Real cy = y1;
	bool changed = false;
	if ((x<=0)&&(cx<x))
	{
		x=cx;
		changed=true;
	}
	if ((y<=0)&&(cy<y))
	{
		y=cy;
		changed=true;
	}
	if ((x>=0)&&(cx>x))
	{
		x=cx;
		changed=true;
	}
	if ((y>=0)&&(cy>y))
	{
		y=cy;
	changed=true;
	}
	if (changed)
	HUD::getSingleton().setCrosshairShiftTopLeft(x,y);
	pauseBeforeDecrease=1.0f;
	//HUD::getSingleton().setCrosshairShiftTopLeft(x,y);
	return Vector3::ZERO;
}
void CrosshairOperator::upd(const FrameEvent& evt)
{
	if (pauseBeforeDecrease>0)
	{
		pauseBeforeDecrease-=evt.timeSinceLastFrame;
	}
	else
	{
		startDecreasing=true;
	}
	if (startDecreasing)
	{
	if ((x>0)&&(y>0))
	{
	HUD::getSingleton().setCrosshairShiftTopLeft(y-evt.timeSinceLastFrame,x-evt.timeSinceLastFrame);
	}
	else
	{
		startDecreasing=false;
		/*if (x=<0)
		{
			x=0;
			HUD::getSingleton().setCrosshairShiftTopLeft(y-evt.timeSinceLastFrame,0);
		}
		if (y=<0)
		{
			y=0;
			HUD::getSingleton().setCrosshairShiftTopLeft(0,x-evt.timeSinceLastFrame);
		}
		if ((x=<0)&&(y=<0))*/
			HUD::getSingleton().setCrosshairShiftTopLeft(0,0);
	}
	}
}

void CrosshairOperator::cleanup()
{
}


