#include "SceneLoadOverlay.h"

template<> SceneLoadOverlay *Singleton<SceneLoadOverlay>::ms_Singleton=0;

SceneLoadOverlay::SceneLoadOverlay()
{
}

SceneLoadOverlay::~SceneLoadOverlay()
{
}

void SceneLoadOverlay::init(Root* mRoot)
{
	srand(time(NULL));
	cf.load("run3/game/loadingscrs/loadingscrs.cfg");
	ConfigFile::SettingsMultiMap *settings = cf.getSectionIterator().getNext();
    ConfigFile::SettingsMultiMap::iterator b;
    settings = cf.getSectionIterator().getNext();
	String curLab;
	overlay = OverlayManager::getSingleton().getByName("Run3/TD01");
	lCont = overlay->getChild("Run3/TD01Panel");
   for (b = settings->begin(); b != settings->end(); ++b)
   {
				curLab = b->first;
                Add(cf.getSetting(curLab));
   }
   root=mRoot;
}

void SceneLoadOverlay::Add(String overlay)
{
//	overlays.push_back(OverlayManager::getSingleton().getByName(overlay));
	overlays.push_back(overlay);
}

void SceneLoadOverlay::Show(String over)
{
	LogManager::getSingleton().logMessage("11");
	lCont->setMaterialName(over);
	LogManager::getSingleton().logMessage("12");
	overlay->show();
	LogManager::getSingleton().logMessage("13");
	root->renderOneFrame();
	LogManager::getSingleton().logMessage("14");
}

void SceneLoadOverlay::Show()
{
	overlay->show();
	root->renderOneFrame();
}

void SceneLoadOverlay::Show(int iter)
{
	lCont->setMaterialName(overlays.at(iter));
	overlay->show();
	root->renderOneFrame();
}

void SceneLoadOverlay::Hide(String over)
{
	overlay->hide();
}

void SceneLoadOverlay::Hide_all()
{
	overlay->hide();
}

void SceneLoadOverlay::SetRandom()
{
	random_iter = rand() % overlays.size() + 1;
	Hide_all();
	Show(random_iter);
}



