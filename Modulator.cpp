#include "Modulator.h"
#include "global.h"

//PISDOSINGLETON_ACTIVATE(Modulator)
template<> Modulator *Ogre::Singleton<Modulator>::ms_Singleton=0;

/*CONSTRUCT_DESCR(Modulator)
{

}

DESTROY_DESCR(Modulator)
{

}

FUNCTION(Modulator::init)()
{
mSceneMgr=global::getSingleton().getSceneManager();
//global::getSingleton().getRoot()->addFrameListener(this);
}*/

Modulator::Modulator()
{
	start_modulate_fov=false;
	time=0;
}

Modulator::~Modulator()
{
}


void Modulator::init(){
mSceneMgr=global::getSingleton().getSceneManager();
fovend=global::getSingleton().getPlayer()->getFOV();
ffe1=fovend;
}



//FUNCTION_R(Modulator::modulate_value)(int x,int type,int step,int param)

Real Modulator::modulate_value(int x,int type,int step,int param)
{
switch (type)
{
case INCREMENT_MODULATE:
	return (Real) x+step;
	break;
case DECREMENT_MODULATE:
	return (Real) x-step;
	break;
case GEOM_PROGRESSION:
	Real res;
	res=x;
	for (unsigned int i=0;i!=step;i++)
		res*=param;
	return res;
	break;
default:
	return x;
	break;
}
}

//FUNCTION(Modulator::modulate_scenenode_pos)(SceneNode* node,Vector3 dest, Real sec)
void Modulator::modulate_scenenode_pos(SceneNode* node,Vector3 dest, Real sec)
{
logic_erase(node);
scenenodes_modulate.push_back(node);
scenenodes_dest.push_back(dest);
scenenodes_sec.push_back(sec);
scenenodes_spos.push_back(node->getPosition());
//LogManager::getSingleton().logMessage(node->getName());
}

bool Modulator::logic_erase(SceneNode* node)
{
	bool found;
	found=false;
	for (unsigned int i=0;i!=scenenodes_modulate.size();i++)
	{
		if (!found){
		if(scenenodes_modulate[i]==node)
		{
			found=true;
			node->setPosition(scenenodes_dest[i]);
			scenenodes_sec[i]=0;
			vector<SceneNode*>::iterator iter0;
			vector<Real>::iterator iter;
			vector<Vector3>::iterator iter2;
			iter0=scenenodes_modulate.begin();
			iter=scenenodes_sec.begin();
			iter2=scenenodes_dest.begin();
			advance(iter0,i);
			advance(iter,i);
			advance(iter2,i);
			//LogManager::getSingleton().logMessage(StringConverter::toString(*iter)+"ERASE_LOGIC");
			//LogManager::getSingleton().logMessage(StringConverter::toString(*iter2));
			scenenodes_modulate.erase(iter0);
			scenenodes_sec.erase(iter);
			scenenodes_dest.erase(iter2);
			iter2=scenenodes_spos.begin();
			advance(iter2,i);
		scenenodes_spos.erase(iter2);
			break;

		}
		}
	}
	if (!found)
		return false;
	return true;
}

void Modulator::modulate_FOV(Real start,Real end, Real sec)
{
modulator_complete_fov();
start_modulate_fov=true;
fovs=start;
fs1=fovs;
fovend=end;
fovvel=sec;
cvel=fovvel;
}

void Modulator::modulator_complete_fov()
{
	if (start_modulate_fov)
	{
		start_modulate_fov=false;
		fs1=0;
		LogManager::getSingleton().logMessage(StringConverter::toString(fovs)+"FS");
		LogManager::getSingleton().logMessage(StringConverter::toString(fovend)+"FE");
		global::getSingleton().getPlayer()->changeFOV(fovend);
		
		fovend=ffe1;
	}
}

void Modulator::modulate_SceneNode(SceneNode* a ,Real amp)
{
	if (a)
		scenenodes_wave[a]=amp;
}


void modulate_l_color_value(int x,int type,int step,int param)
{

}


bool Modulator::frameStarted(const Ogre::FrameEvent &evt)
{
	time+=evt.timeSinceLastFrame;
	map<SceneNode*,Real>::iterator it;
	for (it=scenenodes_wave.begin();it!=scenenodes_wave.end();it++)
	{
		if (it->first)
		{
			it->first->roll(Degree(sin(time)* (it->second)));
		}
		else
		{
			scenenodes_wave.erase(it);
			it--;
		}
	}
	if (time>2)
		time=0;
	if (scenenodes_modulate.size()>0)
	{
//LogManager::getSingleton().logMessage(StringConverter::toString(scenenodes_modulate.size())+"SUCC!1");
for (unsigned int i=0;i!=scenenodes_modulate.size();i++)
{
	//LogManager::getSingleton().logMessage(StringConverter::toString(i));
	if (((scenenodes_dest[i]-scenenodes_modulate[i]->getPosition()).length()<=0)||(scenenodes_sec[i]==0))
	{
		scenenodes_modulate[i]->setPosition(scenenodes_dest[i]);
		return true;
	}
	//Vector3 trans = (scenenodes_dest[i]-scenenodes_modulate[i]->getPosition())*evt.timeSinceLastFrame*20/scenenodes_sec[i];
	Vector3 trans = (scenenodes_dest[i]-scenenodes_spos[i])*scenenodes_sec[i];
	//LogManager::getSingleton().logMessage(StringConverter::toString(trans));
	scenenodes_modulate[i]->setPosition(scenenodes_dest[i]-trans);
	scenenodes_sec[i]-=evt.timeSinceLastFrame;//*TIME_SHIFT;
	//LogManager::getSingleton().logMessage(StringConverter::toString(scenenodes_sec[i])+"SEC");
	//LogManager::getSingleton().logMessage(StringConverter::toString(scenenodes_modulate[i]->getPosition()));
}

unsigned int s;
////LogManager::getSingleton().logMessage("SUCC!1");
//vector<SceneNode*>::iterator i;
vector<Real>::iterator iter;
vector<Vector3>::iterator iter2;
//s=0;
bool stop=true;
while (stop)
{
stop=false;
s=0;
for (vector<SceneNode*>::iterator i=scenenodes_modulate.begin();i!=scenenodes_modulate.end();i++)
{
	if (!stop)
	{
	if (scenenodes_sec[s]<0)
	{
		iter=scenenodes_sec.begin();
		iter2=scenenodes_dest.begin();
		advance(iter,s);
		advance(iter2,s);
		(*i)->setPosition(scenenodes_dest[s]);
		//LogManager::getSingleton().logMessage((*i)->getName()+"ERASE");
		//LogManager::getSingleton().logMessage(StringConverter::toString(scenenodes_modulate.size()));
		scenenodes_modulate.erase(i);
		scenenodes_sec.erase(iter);
		scenenodes_dest.erase(iter2);
		iter2=scenenodes_spos.begin();
		advance(iter2,s);
		scenenodes_spos.erase(iter2);
		stop=true;
		//LogManager::getSingleton().logMessage("SUCCERASE");
		//LogManager::getSingleton().logMessage(StringConverter::toString(scenenodes_modulate.size()));
		
	}	
	s++;
	if (stop)
		break;
	//LogManager::getSingleton().logMessage(StringConverter::toString(s));
	}
}
}
	}
////LogManager::getSingleton().logMessage("SUCC!2");
if (start_modulate_fov)
{
global::getSingleton().getPlayer()->changeFOV(fovs+(fovend-fs1)/fovvel*evt.timeSinceLastFrame);//*TIME_SHIFT);
fovs=global::getSingleton().getPlayer()->getFOV();
LogManager::getSingleton().logMessage(StringConverter::toString(fovs+(fovend-fs1)/fovvel*evt.timeSinceLastFrame*TIME_SHIFT)+"FOV");
cvel-=evt.timeSinceLastFrame;//*TIME_SHIFT;
LogManager::getSingleton().logMessage(StringConverter::toString(cvel)+"SEC");
if (cvel<0)
{
	LogManager::getSingleton().logMessage("COMFOV");
	modulator_complete_fov();
}
}
return true;
}