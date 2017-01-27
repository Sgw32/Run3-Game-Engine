#include "CrosshairOp.h"
#include "global.h"
#include "HUD.h"

template<> CrosshairOperator *Ogre::Singleton<CrosshairOperator>::ms_Singleton=0;

CrosshairOperator::CrosshairOperator() 
{
}

CrosshairOperator::~CrosshairOperator()
{
}

void CrosshairOperator::init()
{
}

Vector3 CrosshairOperator::getCrosshairShift(void)
{
	x = global::getSingleton().getPlayer()->camera_rotation_x*5;
	y = global::getSingleton().getPlayer()->camera_rotation_y*5;
	//HUD::getSingleton().setCrosshairShiftTopLeft(x,y);
	return Vector3::ZERO;
}

Vector3 CrosshairOperator::getCrosshairShift(Real x1,Real y1)
{
	x = x1;
	y = y1;
	//HUD::getSingleton().setCrosshairShiftTopLeft(x,y);
	return Vector3::ZERO;
}
void CrosshairOperator::upd(const FrameEvent& evt)
{
	if ((fabs(x)>0)||(fabs(y)>0))
	{
		if ((x!=0)&&(y!=0))
	HUD::getSingleton().setCrosshairShiftTopLeft((fabs(y)/y)*(fabs(y)-evt.timeSinceLastFrame),(fabs(x)/x)*(fabs(x)-evt.timeSinceLastFrame)); 
		if ((x!=0)&&(y==0))
	HUD::getSingleton().setCrosshairShiftTopLeft(0,(fabs(x)/x)*(fabs(x)-evt.timeSinceLastFrame));
		if ((x==0)&&(y!=0))
	HUD::getSingleton().setCrosshairShiftTopLeft((fabs(y)/y)*(fabs(y)-evt.timeSinceLastFrame),0);
		//(fabs(y)/y) returns a sign of float, second part returns value
		/*if (x!=0)&&
		HUD::getSingleton().setCrosshairShiftTopLeft(*(fabs(y)-evt.timeSinceLastFrame),(x/fabs(x))*(fabs(x)-evt.timeSinceLastFrame));*/
	if ((fabs(y)-evt.timeSinceLastFrame)<0)
		y=0;
	if ((fabs(x)-evt.timeSinceLastFrame)<0)
		x=0;
	}
	else
	{
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

void CrosshairOperator::cleanup()
{
}


