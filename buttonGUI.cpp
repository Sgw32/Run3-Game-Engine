#include "buttonGUI.h"

using namespace buttonGUI;
using namespace Ogre;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    **  UTILITY  CLASSES   **   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

buttonPosition::buttonPosition()
{
	usingRelative = false;
	data.abs.left = 0;
	data.abs.top = 0;
}

buttonPosition::buttonPosition(const relativePosition &relPosition, short offsetLeft, short offsetTop)
{
	usingRelative = true;
	data.rel.position = relPosition;
	data.rel.x = offsetLeft;
	data.rel.y = offsetTop;
}

buttonPosition::buttonPosition(short absoluteLeft, short absoluteTop)
{
	usingRelative = false;
	data.abs.left = absoluteLeft;
	data.abs.top = absoluteTop;
}


buttonTextColor::buttonTextColor(float r, float g, float b, float a)
{
	cTop.r = cBottom.r = r;
	cTop.g = cBottom.g = g;
	cTop.b = cBottom.b = b;
	cTop.a = cBottom.a = a;
}
buttonTextColor::~buttonTextColor()
{}

textScheme::textScheme(std::string font, short unsigned size, float r, float g, float b, float a) :
cMouseOff (buttonTextColor(r,g,b,a)),
cMouseOver (buttonTextColor(r,g,b,a)),
cOnClick (buttonTextColor(r,g,b,a)),
cOnRelease (buttonTextColor(r,g,b,a)),
mFont(font),
mFontSize(size)
{
	Ogre::FontManager::getSingletonPtr()->load( font, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
}
textScheme::~textScheme(void)
{}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    **  BUTTON  CLASS    **   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

button::button(std::string &buttonName, std::string &material, buttonPosition &position, short width, short height, buttonManager * mgr, Ogre::Overlay * o, button * parentButton, textScheme &style, bool isActiveButton) :
name(buttonName),
buttonMgr(mgr),
overlay(o),
parent(parentButton),
movable(false),  //buttons default to non-movable.
centerOnGrab(true),
isMouseOver(0),
textStyle(style),
isActive(isActiveButton),
trigger_onClick(true),			//if you would like events to be sent out or suppressed by default you can set that behavior here
trigger_onRelease(true),
trigger_mouseOver(true),
trigger_mouseOff(true),
trigger_onSubmit(true),
useLimits(false),
x1Limit(0),
x2Limit(0),
y1Limit(0),
y2Limit(0)
{
	suppressed = false;
mLua="";
	if (parent)
	{
		if (parent->suppressed)
			suppressed = true;
	}

	fadingOut = false;
	fadingOutStart = fadingOutEnd = 0;
	fadingIn = false;
	fadingInStart = fadingInEnd = 0;

	panel = static_cast<PanelOverlayElement*>(OverlayManager::getSingleton().createOverlayElement("Panel", name+"Panel")); 
	panel->setMetricsMode(GMM_PIXELS);
	panel->setWidth(width);
	panel->setHeight(height);

	setPosition(position);

	setMaterial(material);
}
button::~button(void)
{}


button * button::createChildButton(std::string name, std::string material, buttonPosition &position, short  width, short  height, bool isActiveButton)
{
	return createChildButton(name, material, position, width, height, textStyle, isActiveButton);
}
button * button::createChildButton(std::string name, std::string material, buttonPosition &position, short  width, short  height, textScheme &scheme, bool isActiveButton)
{
	button *b = new button(name, material, position, width, height, buttonMgr, NULL, this, scheme, isActiveButton);

	panel->addChildImpl(b->panel);

	childButtons.push_back(b);
	buttonMgr->buttons.push_back(b);  //list for all buttons.
	return b;
}

button *  button::addTextArea(std::string name,  Ogre::UTFString value, short posX, short posY, TextAreaOverlayElement::Alignment a)
{
	TextAreaOverlayElement* textArea = static_cast<TextAreaOverlayElement*>(OverlayManager::getSingleton().createOverlayElement("TextArea", name));
	textArea->setMetricsMode(Ogre::GMM_PIXELS);
	textArea->setPosition(posX, posY);
	textArea->setDimensions(1, 1);
	textArea->setCaption(value);
	textArea->setAlignment(a); 

	textArea->setFontName(textStyle.mFont);
	textArea->setCharHeight(textStyle.mFontSize);
	textArea->setColourTop(textStyle.cMouseOff.cTop);
	textArea->setColourBottom(textStyle.cMouseOff.cBottom);
	textArea->show();

	panel->addChild(textArea);
	textAreas.push_back(textArea);

	return this;
}

button *  button::editTextArea(std::string name,  Ogre::UTFString &value)
{
	for ( textItr = textAreas.begin(); textItr != textAreas.end(); ++textItr ) //iterate through our children textAreas 
	{
		if( name == (*textItr)->getName())
		{
			(*textItr)->setCaption(value);
			break;
		}
	}
	return this;
}

std::string button::getTextFromTextArea(std::string textAreaName)
{
	for ( textItr = textAreas.begin(); textItr != textAreas.end(); ++textItr ) //iterate through our children textAreas 
	{
		if( textAreaName == (*textItr)->getName())
		{
			return (*textItr)->getCaption();
		}
	}
	return "";
}

button * button::setTextVisibility(std::string name, bool onOff)
{
	for ( textItr = textAreas.begin(); textItr != textAreas.end(); ++textItr ) //iterate through our children textAreas 
	{
		if( name == (*textItr)->getName())
		{
			if(onOff)
				(*textItr)->show();
			else
				(*textItr)->hide();
			break;
		}
	}
	return this;
}

textInputArea * button::createTextInputArea(std::string textInputName, TextAreaOverlayElement::Alignment a, buttonPosition &position, short width, short height, short textPosX, short textPosY, bool isActiveButton)
{
	return createTextInputArea(textInputName, a, buttonMgr->defaultTextFieldMaterial, position, width, height, textPosX, textPosY,  textStyle, isActiveButton);  //use the last textStyle assigned to this button and pass it on.
}

textInputArea * button::createTextInputArea(std::string textInputName, TextAreaOverlayElement::Alignment a, std::string material, buttonPosition &position, short width, short height, short textPosX, short textPosY, bool isActiveButton)
{
	return createTextInputArea(textInputName, a, material, position, width, height, textPosX, textPosY,  textStyle, isActiveButton);  //use the last textStyle assigned to this button and pass it on.
}

textInputArea * button::createTextInputArea(std::string textInputName, TextAreaOverlayElement::Alignment a, std::string material, buttonPosition &position, short width, short height,  short textPosX, short textPosY, textScheme &style, bool isActiveButton)
{
	textInputArea *t = new textInputArea(buttonMgr, textInputName, a, material, position, this, width, height,  textPosX,  textPosY, style, isActiveButton);
	t->setMovable(false);

	panel->addChildImpl(t->panel);

	childButtons.push_back(t);
	buttonMgr->buttons.push_back(t);  //list for all buttons( and textInputAreas)
	return t;  
}

buttonMesh * button::addButtonMesh(std::string name, std::string meshFile,  short positionX, short positionY, short width,  short height)
{
	buttonMesh * bm = new buttonMesh(name, meshFile, buttonMgr, this, buttonMgr->zOrderCounter,  positionX,  positionY, width,height);
	buttonMgr->zOrderCounter++;

	buttonMeshes.push_back(bm);
	buttonMgr->buttonMeshes.push_back(bm);
	return bm;
}

buttonMesh * button::getButtonMesh(std::string name)
{
	std::string currentButtonMeshName;
	for ( meshItr = buttonMeshes.begin(); meshItr != buttonMeshes.end(); ++meshItr )
	{
		currentButtonMeshName = *(*meshItr)->getName();
		if ( currentButtonMeshName == name)
			return (*meshItr);
	}
	return NULL;  //didn't find a buttonMesh with the given name.
}


button * button::getChildButton(std::string name)
{
	std::string currentButtonName;
	for ( buttonItr = childButtons.begin(); buttonItr != childButtons.end(); ++buttonItr )
	{
		currentButtonName = *(*buttonItr)->getName();
		if ( currentButtonName == name)
			return (*buttonItr);
	}
	return NULL;  //didn't find a button with the given name.
}

button *  button::hide(bool fade, unsigned short fadeDurationMS)
{
	if(!getVisibility()) 
		return this;
	if(fadingOut)	//already busy hiding.
		return this;

	if(fadingIn)
		fadingInStart = fadingInEnd = fadingOutStart = fadingOutEnd = fadingIn = fadingOut = 0;

	setSuppressed(true);  //keep this and its children from sending events.
	if(fade)
	{
		fadingOutStart = timer.getMilliseconds();
		fadingOutEnd = timer.getMilliseconds() + fadeDurationMS + 1;
		fadingOut = true;
	}
	else
	{

		if (overlay)
			overlay->hide();
		else
			panel->hide();

		//hide child meshes.
		for ( meshItr = buttonMeshes.begin(); meshItr != buttonMeshes.end(); ++meshItr )
		{
			(*meshItr)->baseNode->setVisible(false);
		}
	}
	return this;
}

button *  button::show(bool fade, unsigned short fadeDurationMS)
{
	if(!suppressed) //already showing
		return this;

	if(fadingIn || fadingOut)
		fadingInStart = fadingInEnd = fadingOutStart = fadingOutEnd = fadingIn = fadingOut = 0;

	if(fade)
	{
		fadingInStart = timer.getMilliseconds();
		fadingInEnd = timer.getMilliseconds() + fadeDurationMS + 1;
		fadingIn = true;
		setOpacity(0);	//clear any half-transparency action from past events so it can fade from pure nothing.
	}		
	else
		setSuppressed(false); //allow sending messages again.

	if (overlay)
		overlay->show();
	else
		panel->show();

	//show child meshes.
	for ( meshItr = buttonMeshes.begin(); meshItr != buttonMeshes.end(); ++meshItr )
	{
		(*meshItr)->baseNode->setVisible(true);
	}


	return this;
}

void button::setSuppressed(bool onOff)
{
	suppressed = onOff; //allow this button to send events again
	std::vector<button*>::iterator itr;
	for ( itr = childButtons.begin(); itr != childButtons.end(); ++itr )
	{
		if ((*itr)->getVisibility())
			(*itr)->setSuppressed(onOff); //allow children to send events again.
	}
}

bool button::getVisibility(void)
{
	if (overlay)
		return overlay->isVisible();
	else
		return panel->isVisible();
}

void button::update(void)
{
	if(!getVisibility()) 
		return;

	Ogre::Real fadeMod = 1;

	if(fadingIn)
	{
		if(fadingInEnd < timer.getMilliseconds())
		{
			fadingInStart = fadingInEnd = fadingIn = 0;
			setSuppressed(false); //allow messages to be sent again now that its fully showing.

			setOpacity(1);
		}
		else
		{
			fadeMod = (float)(timer.getMilliseconds() - fadingInStart) / (float)(fadingInEnd - fadingInStart);
			setOpacity(fadeMod);
		}
	} 
	else if(fadingOut)
	{
		if(fadingOutEnd < timer.getMilliseconds())
		{
			fadingOutStart = fadingOutEnd = 0;
			fadeMod = 0.0f;
			fadingOut = false;	
			hide(false);
		}
		else
		{
			fadeMod = 1 - (float)(timer.getMilliseconds() - fadingOutStart) / (float)(fadingOutEnd - fadingOutStart);
			setOpacity(fadeMod);
		}
	}
}


button *  button::setOpacity(float opacity)
{
	//do not check for panel visibility here so that you can still set the opacity of something to prepare it for showing at half opacity, for example.

	if (!panel->isTransparent())  //if the panel is transparent it means it has no material at all.
	{
		//fade the material currently applied to this button.
		Ogre::MaterialPtr mat = panel->getMaterial(); 
		setMaterialOpacity(opacity, mat);
	}

	Ogre::ColourValue c;

	//adjust all children textAreas
	for ( textItr = textAreas.begin(); textItr != textAreas.end(); ++textItr )
	{
		c = (*textItr)->getColourTop();
		c.a = opacity;
		(*textItr)->setColourTop(c);
		c = (*textItr)->getColourBottom();
		c.a = opacity;
		(*textItr)->setColourBottom(c);
	}
	//adjust all children buttonMeshes
	for ( meshItr = buttonMeshes.begin(); meshItr != buttonMeshes.end(); ++meshItr )
	{
		(*meshItr)->setOpacity(opacity);
	}

	//adjust all child buttons to inherit this opacity
	for ( buttonItr = childButtons.begin(); buttonItr != childButtons.end(); ++buttonItr )
	{
		(*buttonItr)->setOpacity(opacity);
	}
	return this;
}

void button::setMaterialOpacity(float opacity, Ogre::MaterialPtr &mat)
{
	return;// Maybe okay at last.

	Technique *t = mat->getTechnique(0);  // we are only bothering the fade with the first technique.

	//iterate through passes and textureUnitStates, setting their opacity.
	unsigned short passNum = t->getNumPasses();
	unsigned short int p;
	unsigned short unitStateNum;
	unsigned short s;
	for ( p = 0; p < passNum; p++)
	{
		t->getPass(p)->setSceneBlending(SBT_TRANSPARENT_ALPHA);  //comment this line if you do not want to enforce material setup needed for fading.
		unitStateNum = t->getPass(p)->getNumTextureUnitStates();
		for (s = 0; s < unitStateNum; s++)
		{
			t->getPass(p)->getTextureUnitState(s)->setAlphaOperation(LBX_MODULATE, LBS_MANUAL, LBS_TEXTURE, opacity); 
		}
	}
}

button *  button::setMaterial(std::string materialName)
{
	//we need to make an instance of every material for every button so that when one fades,  it doesnt fade any other buttons that may share the material.

	onClickMaterial = "";
	onReleaseMaterial = "";
	mouseOverMaterial = "";
	mouseOffMaterial = "";

	if ((materialName == "BLANK")||(materialName == ""))
		panel->setTransparent(true);
	else
	{
		panel->setTransparent(false);
		Ogre::MaterialManager &mgr = Ogre::MaterialManager::getSingleton();
		std::string materiallnstanceName;
		MaterialPtr mat;

		if (mgr.resourceExists(materialName))  //try to default to the name itself
		{
			mat = mgr.getByName(materialName);	//the material exists,  so lets make a clone of it and use that.
			materiallnstanceName = (materialName + "_buttonGUI_"+name);
			if (!mgr.resourceExists(materiallnstanceName))  //make sure clone doesn't exist before trying to clone.
				mat->clone(materiallnstanceName);

			onClickMaterial = materiallnstanceName;
			onReleaseMaterial = materiallnstanceName;
			mouseOverMaterial = materiallnstanceName;
			mouseOffMaterial = materiallnstanceName;
			panel->setMaterialName(materiallnstanceName);
		}

		if (mgr.resourceExists(materialName+".mouseOff"))   //fall back to the mouseoff material
		{
			mat = mgr.getByName(materialName+".mouseOff");	
			materiallnstanceName = (materialName + ".mouseOff_buttonGUI_"+name); //the material exists,  so lets make a clone of it and use that.
			if (!mgr.resourceExists(materiallnstanceName))  //make sure clone doesn't exist before trying to clone.
				mat->clone(materiallnstanceName);

			onClickMaterial = materiallnstanceName;
			onReleaseMaterial = materiallnstanceName;
			mouseOverMaterial = materiallnstanceName;
			mouseOffMaterial = materiallnstanceName;
			panel->setMaterialName(materiallnstanceName);
		}

		if (mgr.resourceExists(materialName+".onClick"))
		{
			mat = mgr.getByName(materialName+".onClick");	
			materiallnstanceName = (materialName + ".onClick_buttonGUI_"+name); //the material exists,  so lets make a clone of it and use that.
			if (!mgr.resourceExists(materiallnstanceName))  //make sure clone doesn't exist before trying to clone.
				mat->clone(materiallnstanceName);
			onClickMaterial = materiallnstanceName;	
		}
		if (mgr.resourceExists(materialName+".mouseOver"))
		{
			mat = mgr.getByName(materialName+".mouseOver");	
			materiallnstanceName = (materialName + ".mouseOver_buttonGUI_"+name); //the material exists,  so lets make a clone of it and use that.
			if (!mgr.resourceExists(materiallnstanceName))  //make sure clone doesn't exist before trying to clone.
				mat->clone(materiallnstanceName);
			mouseOverMaterial = materiallnstanceName;	
			onReleaseMaterial = materiallnstanceName;	//onRelease will fall back to mouseOver unless overwritten.
		}
		if (mgr.resourceExists(materialName+".onRelease"))
		{
			mat = mgr.getByName(materialName+".onRelease");	
			materiallnstanceName = (materialName + ".onRelease_buttonGUI_"+name); //the material exists,  so lets make a clone of it and use that.
			if (!mgr.resourceExists(materiallnstanceName))  //make sure clone doesn't exist before trying to clone.
				mat->clone(materiallnstanceName);
			onReleaseMaterial = materiallnstanceName;	
		}
	}
	return this;
}


button *  button::setPosition(buttonPosition &p)
{	
	
	if (useLimits) //check if limits are even imposed on this button
	{
		if (p.usingRelative)
		{
			if (p.data.rel.x < x1Limit) 
				p.data.rel.x = x1Limit;
			if (p.data.rel.x  > x2Limit)
				p.data.rel.x = x2Limit;
			if (p.data.rel.y < y1Limit) 
				p.data.rel.y = y1Limit;
			if (p.data.rel.y  > y2Limit)
				p.data.rel.y = y2Limit;
		}
		else
		{
			if (p.data.abs.left < x1Limit) 
				p.data.abs.left= x1Limit;
			if (p.data.abs.left  > x2Limit)
				p.data.abs.left = x2Limit;
			if (p.data.abs.top < y1Limit) 
				p.data.abs.top = y1Limit;
			if (p.data.abs.top  > y2Limit)
				p.data.abs.top = y2Limit;
		}
	}	

	position = p;
	if(position.usingRelative)
	{
		int winWidth = buttonMgr->screenResX;
		int winHeight = buttonMgr->screenResY;
		float width = panel->getWidth();
		float height = panel->getHeight();

		int left = 0 + position.data.rel.x;
		int center = int((winWidth/2.0f)-(width/2.0f)) + position.data.rel.x;
		if (parent)	center = int((parent->panel->getWidth()/2.0f)-(width/2.0f))+ position.data.rel.x; //if this is a child, this should be relative to parent.
		int right = winWidth - (int)width - position.data.rel.x;
		if (parent)	right = int(parent->panel->getWidth() - width) - position.data.rel.x; //if this is a child, this should be relative to parent.

		int top = 0 + position.data.rel.y;
		int middle = int((winHeight/2.0f)-(height/2.0f)) + position.data.rel.y;
		if (parent)	middle = int((parent->panel->getHeight()/2.0f)-(height/2.0f)) + position.data.rel.y; //if this is a child, this should be relative to parent.
		int bottom = winHeight - (int)height - position.data.rel.y;
		if (parent)	bottom = int(parent->panel->getHeight() - height) - position.data.rel.y; //if this is a child, this should be relative to parent.


		switch(position.data.rel.position)
		{
		case LEFT:
			panel->setPosition((Ogre::Real)left, (Ogre::Real)middle);
			break;
		case TOP_LEFT:
			panel->setPosition((Ogre::Real)left, (Ogre::Real)top);
			break;
		case TOP_CENTER:
			panel->setPosition((Ogre::Real)center, (Ogre::Real)top);
			break;
		case TOP_RIGHT:
			panel->setPosition((Ogre::Real)right, (Ogre::Real)top);
			break;
		case RIGHT:
			panel->setPosition((Ogre::Real)right, (Ogre::Real)middle);
			break;
		case BOTTOM_RIGHT:
			panel->setPosition((Ogre::Real)right, (Ogre::Real)bottom);
			break;
		case BOTTOM_CENTER:
			panel->setPosition((Ogre::Real)center, (Ogre::Real)bottom);
			break;
		case BOTTOM_LEFT:
			panel->setPosition((Ogre::Real)left, (Ogre::Real)bottom);
			break;
		case CENTER:
			panel->setPosition((Ogre::Real)center, (Ogre::Real)middle);
			break;
		default:
			panel->setPosition((Ogre::Real)position.data.rel.x, (Ogre::Real)position.data.rel.y);
			break;
		}
	}
	else	//using absolute position
	{
		if (overlay == NULL) //are we a child?
		{	//we are a child so we need to get the offset we are inheriting from parents so we can set to our actual world position by using the local position with an offset
			int currentLocalLeft = (int)panel->getLeft();
			int currentLocalTop = (int)panel->getTop();
			buttonPosition absPos = getAbsolutePosition();

			int targetLocalLeft =  position.data.abs.left + currentLocalLeft - absPos.data.abs.left;
			int targetLocalTop =  position.data.abs.top + currentLocalTop - absPos.data.abs.top;
			panel->setPosition((Ogre::Real)targetLocalLeft, (Ogre::Real)targetLocalTop );
		}
		else
			panel->setPosition(position.data.abs.left, position.data.abs.top);
	}
	return this;
}

void button::resetPosition(void)
{
	setPosition(position);
}


const buttonPosition * button::getPosition(void)
{
	return &position;
}

bool buttonGUI::button::getPosition( short &x, short &y )
{
//	rel =  TOP_LEFT;
		x = position.data.abs.left;
		y = position.data.abs.top;
		return false;
}


bool buttonGUI::button::getPosition( relativePosition &rel, short &x, short &y )
{
	if(position.usingRelative)
	{
		rel =  position.data.rel.position;
		x = position.data.rel.x;
		y = position.data.rel.y;		
		return true;
	}
	else
	{
		rel =  TOP_LEFT;
		x = position.data.abs.left;
		y = position.data.abs.top;
		return false;
	}
}


buttonPosition button::getAbsolutePosition(void)
{
	int top = (int)panel->getTop();
	int left = (int)panel->getLeft();
	//we need to find out if he has parents,  and if so,  offset  by their top/left values
	if (overlay == NULL)  //this means this guy is a child because only the top level buttons have overlays.
	{
		Ogre::OverlayContainer * parent = panel->getParent();
		while (parent)
		{
			top = top + (int)parent->getTop();
			left = left + (int)parent->getLeft();
			parent = parent->getParent();
		}
	}

	return buttonPosition(left,top);
}

button * button::offsetPositionToScreenCoord( int posX, int posY )
{	
	buttonPosition pos = getAbsolutePosition();
	int offsetX = posX - pos.data.abs.left;
	int offsetY = posY - pos.data.abs.top;

	if (position.usingRelative)
	{
		switch(position.data.rel.position)
		{
		case LEFT:
			position.data.rel.x += offsetX ; 
			position.data.rel.y += offsetY ;
			break;
		case TOP_LEFT:
			position.data.rel.x += offsetX ; 
			position.data.rel.y += offsetY ;
			break;
		case TOP_CENTER:
			position.data.rel.x += offsetX; 
			position.data.rel.y += offsetY;
			break;
		case TOP_RIGHT:
			position.data.rel.x -= offsetX;
			position.data.rel.y += offsetY;
			break;
		case RIGHT:
			position.data.rel.x -= offsetX; 
			position.data.rel.y += offsetY;
			break;
		case BOTTOM_RIGHT:
			position.data.rel.x -= offsetX;
			position.data.rel.y -= offsetY;
			break;
		case BOTTOM_CENTER:
			position.data.rel.x += offsetX ;  
			position.data.rel.y -= offsetY ;
			break;
		case BOTTOM_LEFT:
			position.data.rel.x += offsetX ; //checked
			position.data.rel.y -= offsetY ;
			break;
		case CENTER:
			position.data.rel.x += offsetX; 
			position.data.rel.y += offsetY;
			break;
		default:
			position.data.rel.x += offsetX; 
			position.data.rel.y += offsetY;
			break;
		}
	}
	else //using absolute positioning
	{
		position.data.abs.top +=  offsetY;
		position.data.abs.left +=  offsetX;
	}

	resetPosition();
	return this;
}

button * button::setLimits(bool useLimits)
{
	this->useLimits = useLimits;
	return this;
}

button * button::setLimits(short int x1, short int y1, short int x2, short int y2, bool useLimits)
{
	this->useLimits = useLimits;

	x1Limit = x1;
	x2Limit = x2;
	y1Limit = y1;
	y2Limit = y2;

	return this;
}


button *  button::setSize(short unsigned int width, short unsigned int height)
{
	panel->setWidth(width);
	panel->setHeight(height);

	//reposition all its children which may be positioned relatively inside the button.
	for ( buttonItr = childButtons.begin(); buttonItr != childButtons.end(); ++buttonItr )
	{
		(*buttonItr)->resetPosition();
	}
	return this;
}

button *  button::setTextScheme(textScheme &scheme)
{ 

	textStyle = scheme; 
	for ( textItr = textAreas.begin(); textItr != textAreas.end(); ++textItr )
	{
		(*textItr)->setFontName(scheme.mFont);
		(*textItr)->setHeight(scheme.mFontSize);
		(*textItr)->setColourTop(scheme.cMouseOff.cTop);
		(*textItr)->setColourBottom(scheme.cMouseOff.cBottom);
	}
	return this;
}

button *  button::setTrigger(buttonAction action, bool onOff)
{
	if (action == ONCLICK)
		trigger_onClick = onOff;
	else if (action == ONRELEASE)
		trigger_onRelease = onOff;
	else if (action == MOUSEOVER)
		trigger_mouseOver = onOff;
	else if (action == MOUSEOFF)
		trigger_mouseOff = onOff;
	else if (action == ONSUBMIT)
		trigger_onSubmit = onOff;
	return this;
}

bool button::getTrigger(buttonAction action)
{
	if (action == ONCLICK)
		return trigger_onClick;
	else if (action == ONRELEASE)
		return trigger_onRelease;
	else if (action == MOUSEOVER)
		return trigger_mouseOver;
	else if (action == MOUSEOFF)
		return trigger_mouseOff;
	else if (action == ONSUBMIT)
		return trigger_onSubmit;
	return 0;
}


button *  button::setUV(Ogre::Real u1, Ogre::Real v1, Ogre::Real u2, Ogre::Real v2)
{
	panel->setUV(u1,v1,u2,v2);
	return this;
}


void button::setParent(std::string newParentName)
{
	setParent(buttonMgr->getButton(newParentName));
}

void button::setParent(button * newParentButton)
{
	if (name == "CURSOR")	//do not allow reparenting of the cursor button,  this is unsupported. (ie there is no interface to parent it to its own overlay again... .)
		return;

	if (newParentButton)
	{
		if (overlay)		//this is a top level button
		{
			overlay->remove2D(panel);
			//delete the overlay,  which is no longer needed
			OverlayManager::getSingleton().destroy(overlay->getName());	
			overlay = NULL;
		}
		else if (parent)	//this is a child of another button
		{
			parent->panel->removeChild(panel->getName());
			//remove ourselves from the list that our parent keeps of its child buttons

			for ( buttonItr = parent->childButtons.begin(); buttonItr != parent->childButtons.end(); ++buttonItr )
			{
				if((*buttonItr) == this)
				{
					parent->childButtons.erase(buttonItr);
					break;
				}
			}
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage("[buttonGUI]  ERROR!!! unknown button!  can't determine parent!"); // we have no idea what this is! 
			return;
		}

		parent = newParentButton;
		parent->panel->addChild(this->panel);
		//add ourselves to our the list of our new parent
		parent->childButtons.push_back(this);
	}
}

bool button::setZOrder(short unsigned int z)
{
	if (overlay)
	{
		overlay->setZOrder(z);
		return true;
	}
	else return false;
}

bool button::hitTest(int x, int y)
{
	if(!isActive)
		return 0;
	if(suppressed)
		return 0;
	if(fadingOut)
		return 0;
	if(!panel->isVisible())
		return 0;

	buttonPosition pos = getAbsolutePosition();

	int right = pos.data.abs.left + (int)panel->getWidth(); 
	int bottom = pos.data.abs.top + (int)panel->getHeight();

	if ((pos.data.abs.left <= x ) &&
		(x <= right) &&
		(pos.data.abs.top <= y) &&
		(y <= bottom))
	{
		return 1;
	}
	return 0;
}

bool button::onClick(bool rotateMB)
{
	if (suppressed)
		return false;

	if (onClickMaterial != "" )
	{
		panel->setMaterialName(onClickMaterial);
//		if (!panel->isTransparent())  //if the panel is transparent it means it has no material at all.
//			setOpacity(1);//clear any half transparencies from past events.
	}

	//update the colors of all the textAreas in this button.
	for ( textItr = textAreas.begin(); textItr != textAreas.end(); ++textItr )
	{
		(*textItr)->setColourTop(textStyle.cOnClick.cTop);
		(*textItr)->setColourBottom(textStyle.cOnClick.cBottom);
	}

	if (rotateMB)//send the message to children buttonMeshes
	{
		for ( meshItr = buttonMeshes.begin(); meshItr != buttonMeshes.end(); ++meshItr )
		{	
			(*meshItr)->injectMouseDown(buttonMgr->mouseX, buttonMgr->mouseY);
		}
	}

	return trigger_onClick;
}
bool button::onRelease()
{
	if (suppressed)
		return false;

	if (onReleaseMaterial != "" )
	{
		panel->setMaterialName(onReleaseMaterial);
//		if (!panel->isTransparent())  //if the panel is transparent it means it has no material at all.
//			setOpacity(1);//clear any half transparencies from past events.
	}

	//update the colors of all the textAreas in this button.
	for ( textItr = textAreas.begin(); textItr != textAreas.end(); ++textItr )
	{
		(*textItr)->setColourTop(textStyle.cOnRelease.cTop);
		(*textItr)->setColourBottom(textStyle.cOnRelease.cBottom);
	}

	return trigger_onRelease;
}

bool button::mouseOver()
{
	if (suppressed)
		return false;

	if (mouseOverMaterial != "" )
	{
		panel->setMaterialName(mouseOverMaterial);
//		if (!panel->isTransparent())  //if the panel is transparent it means it has no material at all.
//			setOpacity(1);//clear any half transparencies from past events.
	}

	//update the colors of all the textAreas in this button.
	for ( textItr = textAreas.begin(); textItr != textAreas.end(); ++textItr )
	{
		(*textItr)->setColourTop(textStyle.cMouseOver.cTop);
		(*textItr)->setColourBottom(textStyle.cMouseOver.cBottom);
	}

	isMouseOver = 1;
	return trigger_mouseOver;
}
bool button::mouseOff()
{
	if (suppressed)
		return false;

	if (mouseOffMaterial != "" )
	{
		panel->setMaterialName(mouseOffMaterial);
//		if (!panel->isTransparent())  //if the panel is transparent it means it has no material at all.
//			setOpacity(1);		//clear any half transparencies from past events.
	}

	//update the colors of all the textAreas in this button.
	for ( textItr = textAreas.begin(); textItr != textAreas.end(); ++textItr )
	{
		(*textItr)->setColourTop(textStyle.cMouseOff.cTop);
		(*textItr)->setColourBottom(textStyle.cMouseOff.cBottom);
	}

	isMouseOver = 0;
	return trigger_mouseOff;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    **  TEXT INPUT AREA CLASS    **   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


textInputArea::textInputArea(buttonManager * mgr, std::string &textInputName,  TextAreaOverlayElement::Alignment a, std::string &material, buttonPosition &position,  button * parentButton, short width, short height, short textPosX, short textPosY,  textScheme &style, bool isActiveButton) :
button(textInputName, material, position, width, height, mgr, NULL, parentButton, style, isActiveButton)
{
	maxCharacterLimit = 0; // no limit
	alphanumericOnly = false;
	isPasswordField = false;
	clearTextOnSubmit = false;  //defaults
	defocusOnSubmit = true;
	activatedMaterial = material;
	deactivatedMaterial = material;
	textValue="";
	addTextArea(textInputName + "_textInputArea", "", textPosX,  textPosY, a);
	dynamicTextElement = textAreas.back(); //get last element which is the text area we just created.

	if (Ogre::MaterialManager::getSingleton().resourceExists(material +".focused"))
		activatedMaterial = material +".focused";	//auto apply a separate activated material if it exists.
}

bool textInputArea::input(std::string s)
{

	if(suppressed)
		return false;

	if (maxCharacterLimit) 
	{
		if (textValue.size() < maxCharacterLimit)
		{
			textValue = textValue + s;
			updateText();
			return true;
		}
		else
			return false;  //rejected the input
	}
	else // if maxCharacterLimit = 0 it means unlimited
	{
		textValue = textValue + s;
		updateText();
	}
	return true;
}

bool textInputArea::insertBackspace(void)
{
	if (textValue == "")
		return false;
	if(suppressed)
		return false;

	textValue.resize(textValue.size() - 1);
	updateText();

	return true;
}

Ogre::UTFString * textInputArea::getValue(void)
{
	return &textValue;
}

void textInputArea::submit(void)
{
	if (clearTextOnSubmit)
		clearText();
	if (defocusOnSubmit)
		setFocused(false);
}

textInputArea * textInputArea::setActiveMaterial(std::string material)
{
	activatedMaterial = material;
	return this;
}

textInputArea * textInputArea::setDisplayText(std::string &s)
{
	textValue = s;
	updateText();
	return this;
}

textInputArea * textInputArea::clearText(void)
{
	textValue = "";
	dynamicTextElement->setCaption(textValue);
	return this;
}

bool  textInputArea::setFocused(bool state)
{
	if(state)
	{
		setMaterial(activatedMaterial);
		panel->setMaterialName(onClickMaterial);  //if we got focused its because mouse is on us.
	}
	else
		setMaterial(deactivatedMaterial);

	return true;
}

textInputArea * textInputArea::setMaxCharacterLimit(short unsigned int num)
{
	maxCharacterLimit = num;
	if (textValue.size() > maxCharacterLimit)
	{
		textValue.resize(maxCharacterLimit);  //truncate.
		updateText();
	}
	return this;
}

void buttonGUI::textInputArea::updateText( void )
{
	if (isPasswordField)
		dynamicTextElement->setCaption(std::string(textValue.size(), '*'));
	else
		dynamicTextElement->setCaption(textValue);
}

textInputArea * buttonGUI::textInputArea::setTextScheme( textScheme &scheme )
{
	button::setTextScheme(scheme);
	return this; // the purpose of this method is the same as the one for button except to return as textInputArea *
}

void buttonGUI::textInputArea::setSuppressed( bool onOff )
{

	button::setSuppressed(onOff);
	if (onOff)
	{
		if (buttonMgr->getActiveTextInputArea() == this)
			buttonMgr->clearActiveTextInputArea();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    **   BUTTONMESH CLASS    **   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
buttonMesh::buttonMesh( const Ogre::String name, const Ogre::String meshFile, buttonManager * buttonMgr, button* parentButton, short unsigned int zOrder,  short positionX, short positionY,  short width, short height) :
name(name),
buttonMgr(buttonMgr),
parent(parentButton),
relX(positionX),
relY(positionY),
zoom(1),	
objectAngle(0.9273f),
rotatableX(true),
rotatableY(true),
mouseOn(false),
momentumX(0),
momentumY(0),
friction(.035f),
turnSpeedModifier(.025f) //less makes it turn slower
{
	overlay = OverlayManager::getSingleton().create(name+"_buttonMeshOverlay");
	overlay->setZOrder(zOrder);  
	overlay->show();

	baseNode = new Ogre::SceneNode(buttonMgr->sceneMgr, name +"_buttonGUIBaseNode");  //because this node is going into an overlay, should not use sceneManager to make this instance.  See comments in ogreoverlay.h for more info.
	e = buttonMgr->sceneMgr->createEntity(name + "_buttonGUIEntity", meshFile);
	baseNode->attachObject(e);
	overlay->add3D(baseNode);

	//replace all the materials on the mesh with instances,  so that we can fade buttonMeshes independently of each other
	Ogre::MaterialManager &mgr = Ogre::MaterialManager::getSingleton();
	for (short unsigned int x = 0; x < e->getNumSubEntities(); x++)
	{
		Ogre::MaterialPtr mat = e->getSubEntity(x)->getMaterial();
		std::string matName = mat->getName();
		std::string materiallnstanceName = (matName + "_buttonGUI_"+name);
		if (!mgr.resourceExists(materiallnstanceName))  //make sure clone doesn't exist before trying to clone.
			mat->clone(materiallnstanceName);
		e->getSubEntity(x)->setMaterialName(materiallnstanceName);
	}

	if (!parent->getVisibility())
		baseNode->setVisible(false);  //inherit the parent's visibility

	//size the node to fit inside a 1 unit box so we can then use that as a modifier to get our target height.
	Ogre::Vector3 bbox = e->getBoundingBox().getSize();
	Ogre::Real largest = bbox.x;
	if (largest  < bbox.y)
		largest = bbox.y;
	if (largest  < bbox.z)
		largest = bbox.z;
	scaleModifier = 1/largest;

	setDimensions(width, height);
	update();
}

buttonMesh::~buttonMesh()
{
	overlay->remove3D(baseNode);
	OverlayManager::getSingleton().destroy(overlay);
	if (baseNode)
	{
		if (e)
		{
			baseNode->detachObject(e);
			buttonMgr->sceneMgr->destroyEntity(e);
		}
		delete baseNode;
		baseNode = NULL;
	}
}
void buttonMesh::resetDimensions(void)
{
	setDimensions(width, height);
}
buttonMesh * buttonMesh::setDimensions(short w, short h)
{
	width = w;  
	height = h;
	Ogre::Radian fov = buttonMgr->sceneMgr->getCamera(buttonMgr->camera)->getFOVy();
	const Ogre::Real percentageModifier = fov.valueRadians()/objectAngle;  //this is our basepoint modifier for the object scale.
	viewPlaneLengthAtOne = tan(fov.valueRadians()/2) *2;  //this is our basepoint modifier for the object position.. how long is the viewplane in y at a distance of 1, used later for object placement.

	aspectRatio = float(buttonMgr->screenResX)/float(buttonMgr->screenResY);
	screenSpaceModifier = (float(1)/float(buttonMgr->screenResY)) ;

	Ogre::Real scale  = screenSpaceModifier * width * percentageModifier * zoom * scaleModifier; 

	baseNode->setScale(scale, scale, scale);
	return this;
}

buttonMesh * buttonMesh::setZoom(Ogre::Real z)
{
	zoom = abs(z);  //negative values will put the object behind the camera... ie not visible!
	resetDimensions();
	setPosition(relX,relY); //reset position
	return this;
}

buttonMesh * buttonMesh::setRotation(Ogre::Real x, Ogre::Real y, Ogre::Real z)
{
	Quaternion quat = Quaternion(Degree(x), Ogre::Vector3::UNIT_X);
	Quaternion quat1 = Quaternion(Degree(y), Ogre::Vector3::UNIT_Y);
	Quaternion quat2 = Quaternion(Degree(z), Ogre::Vector3::UNIT_Z);
	quat = quat * quat1 * quat2;

	baseNode->rotate(quat);
	return this;
}

buttonMesh * buttonMesh::setRotation(Quaternion rot)
{
	baseNode->rotate(rot);
	return this;
}


buttonMesh * buttonMesh::setPosition(short x, short y)
{
	x = x + width/2; //models use their pivot as a reference point instead of upper left corner like buttons...so we adjust here.
	y = y + height/2;

	//include offset  (distance from center of the screen to top at z = 1  and distance from center of screen to left at z = 1)
	Ogre::Real xPos = (x * screenSpaceModifier * viewPlaneLengthAtOne ) - (viewPlaneLengthAtOne/2 * aspectRatio);	//the offset at the end puts the pivot at the exact upper left of the screen so we can make proper screen calculations.
	Ogre::Real yPos = y * screenSpaceModifier * viewPlaneLengthAtOne - viewPlaneLengthAtOne/2;

	baseNode->setPosition(xPos * zoom, -yPos * zoom , -zoom);
	return this;
}

buttonMesh * buttonMesh::setOpacity(float opacity)
{

	for (short unsigned int x = 0; x < e->getNumSubEntities(); x++)
	{
		Ogre::MaterialPtr mat = e->getSubEntity(x)->getMaterial();
		parent->setMaterialOpacity(opacity, mat);
	}
	return this;
}

void buttonMesh::update(void)  
{
	buttonPosition p = parent->getAbsolutePosition();
	setPosition(p.data.abs.left + relX, p.data.abs.top + relY); //we can't truly parent buttonMeshes to buttons.  so I update their positions each frame here

	if((rotatableX)||(rotatableY))
	{
		if(rotatableX)
		{
			if (momentumX)
			{
				baseNode->yaw(Ogre::Radian(-momentumX * turnSpeedModifier), Ogre::Node::TS_WORLD);

				momentumX = momentumX  - friction * (float)buttonMgr->timeDiff;
				if (momentumX <= 0)
					momentumX = 0;
			}
		}
		if(rotatableY)
		{
			if (momentumY)
			{
				baseNode->pitch(Ogre::Radian(-momentumY * turnSpeedModifier), Ogre::Node::TS_WORLD);	

				momentumY = momentumY - friction * (float)buttonMgr->timeDiff;
				if (momentumY <= 0)
					momentumY = 0;
			}
		}
	}
}

void buttonMesh::injectMouseUp(short &x, short &y)
{
	mouseOn = false;
}
void buttonMesh::injectMouseDown(short &x, short &y)
{
	mouseOn = true;

	if(mouseOn)
	{
		if((rotatableX)||(rotatableY))
		{
			lastPos.x = x;
			lastPos.y = y;
		}
	}
}
void buttonMesh::injectMouseMove(short &x, short &y)
{
	if(mouseOn)
	{
		if((rotatableX)||(rotatableY))
		{
			momentumX = lastPos.x - (float)x;
			momentumY = lastPos.y - (float)y;
			lastPos.x = (float)x;
			lastPos.y = (float)y;
		}
	}
}

unsigned short friction;
short momentumX;
short momentumY;
Ogre::Vector2 lastPos;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    **  BUTTON  MANAGER CLASS    **   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

buttonManager::buttonManager(std::string defaultTextFieldM,  textScheme defaultTextScheme, Ogre::SceneManager *mgr,  std::string cameraName) :
sceneMgr(mgr),
camera(cameraName),
defaultTextFieldMaterial(defaultTextFieldM),
defaultTextStyle(defaultTextScheme),
grabbedButton(NULL),
grabbingMouseButton(MB_Left),  //defnes the mouse button that is used to drag buttons around
turningMouseButton(MB_Right), // defines the mouse button that can rotate buttonMeshes
zOrderCounter(5),
activeTextInputArea(NULL),
eventContainer(ONSUBMIT, OIS::MouseButtonID(-1), NULL, NULL),  //junk event container just for the initialization
capslock(false),
lshift(false),
rshift(false),
backSpace(false),
backSpaceHeld(false),
backSpaceHeldTime(0)
{
	resetScreenResolution();  //set our resolution so our stuff will be sized right

	Ogre::Overlay * mouseOverlay = OverlayManager::getSingleton().create("buttonGUI_CURSOR");
	mouseOverlay->setZOrder(650);  //closest possible to camera
	mouseOverlay->show();
	//create a button that represents  the cursor. 
	cursorButton = new button(std::string("CURSOR"),std::string("BLANK"),buttonPosition(0,0),1,1,this,mouseOverlay, NULL, defaultTextScheme, false);	
	mouseOverlay->add2D(cursorButton->panel);
}
buttonManager::~buttonManager(void)
{
	shutdown();
}

void buttonManager::resetScreenResolution(void)
{
	RenderWindow *rw= Ogre::Root::getSingleton().getAutoCreatedWindow();
	screenResX= rw->getWidth();
	screenResY = rw->getHeight();

	//repostion all elements
	for ( buttonItr = buttons.begin(); buttonItr != buttons.end(); ++buttonItr )
	{
		(*buttonItr)->resetPosition();
	}

	//resize the 3D stuff which gets squashed and streched.
	for ( meshItr = buttonMeshes.begin(); meshItr != buttonMeshes.end(); ++meshItr )
	{
		(*meshItr)->resetDimensions();
	}
}

textInputArea * buttonManager::cycleTextInputArea( void )
{
	std::vector<button*> tias;
	for ( buttonItr = buttons.begin(); buttonItr != buttons.end(); ++buttonItr )
	{
		if ((*buttonItr)->isTextInputArea())
		{
			if ((*buttonItr)->getVisibility()) //log only the usable TIAs
			{
				if (activeTextInputArea)
					tias.push_back((*buttonItr));
				else
				{
					forceClickButton((*buttonItr));
					return activeTextInputArea;
				}
			}
		}
	}
	
	for ( buttonItr = tias.begin(); buttonItr != tias.end(); ++buttonItr )
	{
		if ((*buttonItr) == activeTextInputArea)
		{
			++buttonItr;
			if (buttonItr == tias.end()) //cycle to the first one if we are at the end of the array
				forceClickButton((*tias.begin()));
			else
				forceClickButton((*buttonItr));
			return activeTextInputArea;
		}
	}
	return activeTextInputArea;
}

button * buttonManager::setCursor(std::string material,  unsigned short width,  unsigned short height,  unsigned short hotspotX,  unsigned short hotspotY, bool visibility)
{
	ShowCursor(false);  //hide windows cursor
	mouseOffsetX = hotspotX;
	mouseOffsetY = hotspotY;

	cursorButton->setMaterial(material);
	cursorButton->setSize(width,height);

	cursorButton->setPosition(buttonPosition(mouseX - mouseOffsetX, mouseY- mouseOffsetY));

	if (visibility)
		cursorButton->show(false);
	else
		cursorButton->hide(false);

	return cursorButton;
}

button * buttonManager::showCursor(void)
{
	cursorButton->show(false);
	return cursorButton;
}
button * buttonManager::hideCursor(void)
{
	cursorButton->hide(false);
	return cursorButton;
}

bool buttonManager::injectKeyPressed( const OIS::KeyEvent & arg)
{	
	if (arg.key == KC_TAB)  //cycle through tia's
	{
		cycleTextInputArea();
	}

	if (arg.key == KC_CAPITAL)  //caps lock
	{
		if (capslock)
			capslock = false;
		else
			capslock = true;
	}

	if (activeTextInputArea)
	{ //inject the key into the active textArea 
		if(arg.key == KC_RETURN)
		{
			if (activeTextInputArea->getTrigger(ONSUBMIT))
			{
				eventLog.push_back(buttonEvent(ONSUBMIT, OIS::MouseButtonID(-1), activeTextInputArea, NULL, *activeTextInputArea->getValue())); //log the event
				activeTextInputArea->submit();
				if (activeTextInputArea->getDefocusOnSubmit())
					activeTextInputArea = NULL;
			}
		}
		else if(arg.key == KC_BACK)
		{
			if (activeTextInputArea->insertBackspace())
				eventLog.push_back(buttonEvent(KEYACCEPTED, OIS::MouseButtonID(-1), activeTextInputArea, NULL, "BACKSPACE")); //log the event, accepted
			else
				eventLog.push_back(buttonEvent(KEYREJECTED, OIS::MouseButtonID(-1), activeTextInputArea, NULL, "BACKSPACE")); //log the event as rejected
			backSpace = true;
			backSpaceHeldTime = timer.getMilliseconds();
		}
		else if(arg.key == KC_LSHIFT ) 
			lshift = true;
		else if(arg.key == KC_RSHIFT )
			rshift = true;
		else
		{
			bool shift = false;
			if ((lshift)||(rshift))
				shift = true;

			eventStringContainer = keyCodeToString(arg.key, shift, activeTextInputArea->getAlphanumericOnly());//in theory,  one could support unicode here...
			bool accepted;
			if (eventStringContainer == "")
			{
				eventStringContainer = "noTranslation";
				accepted = false;
			}
			else
				accepted = activeTextInputArea->input(eventStringContainer);  

			if (accepted)
				eventLog.push_back(buttonEvent(KEYACCEPTED, OIS::MouseButtonID(-1), activeTextInputArea, NULL, eventStringContainer)); //log the event
			else
				eventLog.push_back(buttonEvent(KEYREJECTED, OIS::MouseButtonID(-1), activeTextInputArea, NULL, eventStringContainer)); //log the event
		}
	}
	return true;
}

bool buttonManager::injectKeyReleased( const OIS::KeyEvent & arg)
{
	if (activeTextInputArea)
	{ //inject the key into the active textArea 
		if(arg.key == KC_BACK)
		{
			backSpace = false;
			backSpaceHeld = false;
			backSpaceHeldTime = 0;
		}
		else if(arg.key == KC_LSHIFT )
			lshift = false;
		else if(arg.key == KC_RSHIFT )
			rshift = false;
	}
	return true;
}

button * buttonManager::getTopButton(void)
{
	std::vector<button*>hits;  //determine who is on top and run the command only on him.
	for ( buttonItr = buttons.begin(); buttonItr != buttons.end(); ++buttonItr )
	{
		if ((*buttonItr)->hitTest(mouseX,mouseY))
		{
			hits.push_back((*buttonItr));  //keep track of positives
		}
	}

	//find out who has the highest zOrder ie who gets the onClick event 
	button* topButton = NULL;
	unsigned short topValue = 0;
	for ( buttonItr = hits.begin(); buttonItr != hits.end(); ++buttonItr )
	{		
		if ((*buttonItr)->panel->getZOrder() >= topValue)
		{
			if((*buttonItr) != grabbedButton)  //skip the dragged button,  we already know what it is and we want to click and drop things on buttons other than him. 
			{
				topButton = (*buttonItr);  //closest so far
				topValue = (*buttonItr)->panel->getZOrder(); //move up the topValue
			}
		}
	}

	return topButton;
}

bool buttonManager::injectMouseDown(OIS::MouseButtonID &id)
{
	//de-focus the active textInputArea, if any
	if (activeTextInputArea)
		activeTextInputArea->setFocused(false);
	activeTextInputArea = NULL;

	button* topButton = getTopButton();

	if (topButton)
	{
		bool isRotateMB = false;
		if (turningMouseButton == id)
			isRotateMB = true;

		//if (id==OIS::MB_Left)
		//{
			if(topButton->onClick(isRotateMB)) //register the event in the button and check the returned trigger
				eventLog.push_back(buttonEvent(ONCLICK, id, topButton)); //log the event
		//}

		if(topButton->setFocused(true)) //only textInputAreas can be focused, so only they return true with setFocused()
		{
			activeTextInputArea = static_cast<textInputArea*>(topButton); //store this textInputArea as the focus so we can channel the inputs there
		}

		if (id == grabbingMouseButton)	//should we check for movable buttons?
		{
			if (topButton->getMovable())
			{
				grabbedButton = topButton;		//grab this button
				injectMouseMove(mouseX, mouseY); // force a move to the same location so that the button will snap to the mouse
			}
		}			
	}

	return true;
}

bool buttonManager::injectMouseUp(OIS::MouseButtonID &id)
{
	button* topButton = getTopButton();

	if (topButton)
	{
		if (topButton->onRelease())
		{
			std::string message;
			//if a button is currently grabbed,  pass its name as the 'additional message' because that means that that button has been drag/dropped onto this one.
			if (grabbedButton)
				eventLog.push_back(buttonEvent(ONRELEASE,id, topButton, grabbedButton));  //log the event
			else 
				eventLog.push_back(buttonEvent(ONRELEASE,id, topButton));  //log the event
		}
	}

	if (id == grabbingMouseButton)
	{
		if (grabbedButton)
		{
			grabbedButton->onRelease();
			if (!topButton)  //send an event that the dragged button was released on nothing.
				eventLog.push_back(buttonEvent(ONRELEASE,id, NULL, grabbedButton)); 
		}
		grabbedButton = NULL;		//stop carrying any grabbed button
	}

	//send the message to all buttonMeshes
	for ( meshItr = buttonMeshes.begin(); meshItr != buttonMeshes.end(); ++meshItr )
	{	
		(*meshItr)->injectMouseUp(mouseX, mouseY);
	}

	return true;
}



bool buttonManager::injectMouseMove(int xPos, int yPos)
{
	//drag the grabbed button (if any)
	if (grabbedButton)
	{	
		if (grabbedButton->centerOnGrab )//this centers the grabbed button to the mouse
		{
			int width = int(grabbedButton->panel->getWidth()/2.0f);  
			int height = int(grabbedButton->panel->getHeight()/2.0f);
			grabbedButton->offsetPositionToScreenCoord(xPos - width, yPos - height);
		}
		else	//do a relative drag.
		{
			buttonPosition p = grabbedButton->getAbsolutePosition();
			grabbedButton->offsetPositionToScreenCoord(p.data.abs.left +  xPos - mouseX, p.data.abs.top + yPos - mouseY);  //current pos + old mousePos - new mousePos
		}
	}

	mouseX = xPos;  
	mouseY = yPos;
	cursorButton->setPosition(buttonPosition(xPos - mouseOffsetX, yPos- mouseOffsetY));  //update our cursor, line it up with trigger point.

	//  trigger all the mouseOvers and mouseOffs
	for ( buttonItr = buttons.begin(); buttonItr != buttons.end(); ++buttonItr )
	{
		if  ((*buttonItr)->hitTest(mouseX,mouseY)) //are we over x button?
		{
			if (!(*buttonItr)->isMouseOver)  //if the mouse was previously off,  trigger the mouse over.
			{
				if((*buttonItr)->mouseOver()) //register the event and check the returned trigger
					eventLog.push_back(buttonEvent(MOUSEOVER, OIS::MouseButtonID(-1), (*buttonItr)));

			}
		}
		else
		{
			if ((*buttonItr)->isMouseOver)  //if the mouse was previously over,  trigger the mouse off.
			{
				if((*buttonItr)->mouseOff()) //register the event and check the returned trigger
					eventLog.push_back(buttonEvent(MOUSEOFF,OIS::MouseButtonID(-1), (*buttonItr)));
			}
		}
	}

	//send the message to all buttonMeshes,  this is to allow rotation even when the mouse has moved off the actual button
	for ( meshItr = buttonMeshes.begin(); meshItr != buttonMeshes.end(); ++meshItr )
	{	
		(*meshItr)->injectMouseMove(mouseX, mouseY);
	}
	return true;
}

bool buttonManager::injectMouseWheel(short int z)
{
	if (z != 0)
	{
		button* topButton = getTopButton();
		if (topButton)
		{
			buttonAction a;
			if (z > 0)
				a = MOUSEWHEELUP;
			else
				a = MOUSEWHEELDOWN;

			eventLog.push_back(buttonEvent(a, OIS::MouseButtonID(-1), topButton));
		}
	}
	return true;
}



void buttonManager::forceClick(OIS::MouseButtonID mouseButton)
{
	injectMouseDown(mouseButton); //insert a single click
	injectMouseUp(mouseButton);
}

void buttonGUI::buttonManager::forceClickButton(button *b,  OIS::MouseButtonID mouseButton /*= MB_Left*/)
{
	if (b)
	{
		short origX = mouseX;
		short origY = mouseY;
		buttonPosition pos = b->getAbsolutePosition();
		injectMouseMove(pos.data.abs.left + 1, pos.data.abs.top + 1); //position mouse
		forceClick(mouseButton);
		injectMouseMove(origX,origY);
	}
}

void buttonGUI::buttonManager::forceClickButton( std::string buttonName, OIS::MouseButtonID mouseButton /*= MB_Left*/ )
{
	forceClickButton(getButton(buttonName), mouseButton);
}

button* buttonManager::createButton(std::string buttonName,  std::string material,  buttonPosition &position, unsigned short width, unsigned short height, unsigned short zOrder,  bool visible, bool isActive)
{
	return createButton(buttonName, material, position, width, height, defaultTextStyle, zOrder, visible, isActive);
}

button* buttonManager::createButton(std::string buttonName,  std::string material,  buttonPosition &position, unsigned short width, unsigned short height, unsigned short zOrder,  bool visible, bool isActive,String lua)
{

	button *b = createButton(buttonName, material, position, width, height, defaultTextStyle, zOrder, visible, isActive);
	b->mLua=lua;
	return b;  //return our new button which is at the end of the vector
}

button* buttonManager::createButton(std::string buttonName,  std::string material,  buttonPosition &position, unsigned short width, unsigned short height, textScheme &scheme, unsigned short zOrder,  bool visible, bool isActive)
{
	if(!zOrder)
		zOrder = zOrderCounter++;

	Ogre::Overlay * o = OverlayManager::getSingleton().create(buttonName+"buttonOverlay");
	o->setZOrder(zOrder);  
	o->show();

	button *b = new button(buttonName, material, position, width, height,  this, o, NULL, scheme, isActive);

	o->add2D(b->panel); //add the panel that the button created to the overlay.  Overlays are only created for top level buttons,  the panels of child buttons are parented to panel of the button that created it.

	if (!visible)
		b->hide(false);

	buttons.push_back(b);
	return b;  //return our new button which is at the end of the vector
}


void buttonManager::setDefaults(std::string textFieldMaterial, textScheme defaultTextScheme)
{
	defaultTextFieldMaterial = textFieldMaterial;
	defaultTextStyle = defaultTextScheme;
}

buttonEvent * buttonManager::getEvent(void)
{
	//basically,  this is a hopper system that you can get the events from one by one
	if (eventLog.empty())
		return NULL;
	else
	{
		eventContainer = eventLog.back();
		eventLog.pop_back();
	}
	return &eventContainer;
}

button * buttonManager::getButton(std::string buttonName)
{
	std::string currentButtonName;
	for ( buttonItr = buttons.begin(); buttonItr != buttons.end(); ++buttonItr )
	{
		currentButtonName = *(*buttonItr)->getName();
		if ( currentButtonName == buttonName)
		{
			if  (!(*buttonItr)->isTextInputArea()) // only return buttons
				return (*buttonItr);
		}
	}

	return NULL;  //didn't find a button with the given name.
}


void buttonManager::deleteButton(std::string buttonName)
{
	deleteButton(getButton(buttonName));
}

void buttonManager::deleteButton(button * b)
{
	//this function is a little dangerous... lets make sure none of the buttons being deleted are being pointed to by these...
	if (activeTextInputArea)
	{
		activeTextInputArea->setFocused(false);
		activeTextInputArea = NULL;
	}

	if (grabbedButton)
		grabbedButton = NULL;

	if(b)	//this is a recursive algorithm,  so i just want to be safe and make sure b hasnt been deleted already if someone decides to call this inside some kind of loop
	{
		//delete children first.
		std::vector<button *>children = b->childButtons;  // we need to make a copy of the vector so that when we delete children so they dont muck with the list we are iterating on while we are deleting them.
		std::vector<button*>::iterator itr; //don't use the buttonManager button vector iterator here because this function is recursive.
		for ( itr = children.begin(); itr != children.end(); ++itr )
		{
			deleteButton((*itr));
		}

		//delete its buttonMeshes
		std::vector<buttonMesh *>bmChildren = b->buttonMeshes;  // we need to make a copy of the vector so that when we delete children so they dont muck with the list we are iterating on while we are deleting them.
		std::vector<buttonMesh*>::iterator childBMitr; //don't use the buttonManager buttonMesh vector iterator here because this function is recursive.
		for ( childBMitr= bmChildren.begin(); childBMitr != bmChildren.end(); ++childBMitr )
		{
			//remove buttonMesh pointer from the master list.
			for ( meshItr = buttonMeshes.begin(); meshItr != buttonMeshes.end(); ++meshItr )
			{
				if((*meshItr) == (*childBMitr))
				{
					buttonMeshes.erase(meshItr);
					break;
				}
			}
			//then delete the actual pointer... no need to bother removing it from the buttonMesh's parent's list,  because the parent button is about to get deleted.
			delete (*childBMitr);
			(*childBMitr) = NULL;
		}

		Ogre::Overlay * overlay = b->overlay;
		button * parent = b->parent;
		if (overlay)		//this is a top level button
		{
			//delete the overlay,  which is no longer needed
			OverlayManager::getSingleton().destroyOverlayElement(b->panel);   //SUGGESTED BY DarkScythe
			OverlayManager::getSingleton().destroy(overlay->getName());	
			overlay = NULL;
		}
		else if (parent)	//this is a child of another button
		{
			parent->panel->removeChild(b->panel->getName());

			//remove the button from the list that its parent keeps of its child buttons			
			for ( itr = parent->childButtons.begin(); itr != parent->childButtons.end(); ++itr )
			{
				if((*itr) == b)
				{
					parent->childButtons.erase(itr);
					break;
				}
			}
			//delete the panel
			OverlayManager::getSingleton().destroyOverlayElement(b->panel);
		}

		//remove button from the master list.
		for ( itr = buttons.begin(); itr != buttons.end(); ++itr )
		{
			if((*itr) == b)
			{
				buttons.erase(itr);
				break;
			}
		}
		//delete the button instance
		delete b;
		b = NULL;
	}
}

void buttonManager::deleteAllButtons(void)
{
	std::vector<button *>topButtons;
	for ( buttonItr = buttons.begin(); buttonItr != buttons.end(); ++buttonItr )
	{
		//gather the top level buttons.
		if ((*buttonItr)->overlay)	
			topButtons.push_back((*buttonItr));
	}
	//cycle through the top buttons only, that way the rest will be deleted in a non-conflictive order.
	for ( buttonItr = topButtons.begin(); buttonItr != topButtons.end(); ++buttonItr )
	{
		deleteButton((*buttonItr));
	}
}

void buttonManager::shutdown(void)
{
	deleteAllButtons();
	deleteButton(cursorButton);
}

textInputArea * buttonManager::getTextInputArea(std::string name)
{
	std::string currentButtonName;
	for ( buttonItr = buttons.begin(); buttonItr != buttons.end(); ++buttonItr )
	{
		currentButtonName = *(*buttonItr)->getName();
		if ( currentButtonName == name)
		{
			if  ((*buttonItr)->isTextInputArea()) // only return textInputAreas
				return static_cast<textInputArea*>(*buttonItr);
		}
	}

	return NULL;  //didn't find a button with the given name.
}


void buttonManager::update(void)
{
	unsigned long currentTime = timer.getMilliseconds();
	timeDiff = currentTime - lastFrameTime;
	lastFrameTime = currentTime;

	//this handles the action of holding down backspace while a textArea is active.
	if(activeTextInputArea)
	{
		if (backSpace)
		{
			if (backSpaceHeld)
			{
				unsigned long timePassed = currentTime - backSpaceHeldTime;
				if (timePassed != 0)
				{
					unsigned int numCharsToDelete = unsigned int(timePassed / backSpaceFlowTime);		

					if (numCharsToDelete != 0)
						backSpaceHeldTime = currentTime; //reset the timer 
					// delete x num characters
					for (unsigned short x = 0; x < numCharsToDelete; x++)
					{
						if (activeTextInputArea->insertBackspace())
							eventLog.push_back(buttonEvent(KEYACCEPTED, OIS::MouseButtonID(-1), activeTextInputArea, NULL, "BACKSPACE")); //log the event
						else
							eventLog.push_back(buttonEvent(KEYREJECTED, OIS::MouseButtonID(-1), activeTextInputArea, NULL, "BACKSPACE")); //log the event
					}
				}
			}
			else if (  backSpaceHeldWait < (currentTime - backSpaceHeldTime) ) //calculate if we need to hold it.
			{
				backSpaceHeldTime =  currentTime;
				backSpaceHeld = true;
			}
		}
	}
	//update all buttons.
	for ( buttonItr = buttons.begin(); buttonItr != buttons.end(); ++buttonItr )
	{
		(*buttonItr)->update();
	}
	//and mesh buttons
	for ( meshItr = buttonMeshes.begin(); meshItr != buttonMeshes.end(); ++meshItr )
	{	
		(*meshItr)->update(); 
	}
}


std::string  buttonManager::keyCodeToString(const OIS::KeyCode &key, bool shift, bool alphanumericOnly) 
{ 
	//I apologize for my barbaric keyCode translation algorithm.
	std::string s = "";

	if (key == KC_1 )
		s = "1";
	else if (key == KC_2 )
		s = "2";
	else if (key == KC_3 )
		s = "3";
	else if (key == KC_4 )
		s = "4";
	else if (key == KC_5 )
		s = "5";
	else if (key == KC_6 )
		s = "6";
	else if (key == KC_7 )
		s = "7";
	else if (key == KC_8 )
		s = "8";
	else if (key == KC_9 )
		s = "9";
	else if (key == KC_0 )
		s = "0";
	else if (key == KC_MINUS ) {  // - on main keyboard
		if (shift)	s = "_";
		else	s = "-";}
	else if (key == KC_Q ){
		if (shift||capslock)	s = "Q";
		else	s = "q";}
	else if (key == KC_W ){
		if (shift||capslock)	s = "W";
		else	s = "w";}
	else if (key == KC_E ){
		if (shift||capslock)	s = "E";
		else	s = "e";}
	else if (key == KC_R ){
		if (shift||capslock)	s = "R";
		else	s = "r";}
	else if (key == KC_T ){
		if (shift||capslock)	s = "T";
		else	s = "t";}
	else if (key == KC_Y ){
		if (shift||capslock)	s = "Y";
		else	s = "y";}
	else if (key == KC_U ){
		if (shift||capslock)	s = "U";
		else	s = "u";}
	else if (key == KC_I ){
		if (shift||capslock)	s = "I";
		else	s = "i";}
	else if (key == KC_O ){
		if (shift||capslock)	s = "O";
		else	s = "o";}
	else if (key == KC_P ){
		if (shift||capslock)	s = "P";
		else	s = "p";}
	else if (key == KC_A){
		if (shift||capslock)	s = "A";
		else	s = "a";}
	else if (key == KC_S){
		if (shift||capslock)	s = "S";
		else	s = "s";}
	else if (key == KC_D){
		if (shift||capslock)	s = "D";
		else	s = "d";}
	else if (key == KC_F){
		if (shift||capslock)	s = "F";
		else	s = "f";}
	else if (key == KC_G){
		if (shift||capslock)	s = "G";
		else	s = "g";}
	else if (key == KC_H){
		if (shift||capslock)	s = "H";
		else	s = "h";}
	else if (key == KC_J){
		if (shift||capslock)	s = "J";
		else	s = "j";}
	else if (key == KC_K){
		if (shift||capslock)	s = "K";
		else	s = "k";}
	else if (key == KC_L){
		if (shift||capslock)	s = "L";
		else	s = "l";}
	else if (key == KC_Z){
		if (shift||capslock)	s = "Z";
		else	s = "z";}
	else if (key == KC_X ){
		if (shift||capslock)	s = "X";
		else	s = "x";}
	else if (key == KC_C ){
		if (shift||capslock)	s = "C";
		else	s = "c";}
	else if (key == KC_V ){
		if (shift||capslock)	s = "V";
		else	s = "v";}
	else if (key == KC_B ){
		if (shift||capslock)	s = "B";
		else	s = "b";}
	else if (key == KC_N ){
		if (shift||capslock)	s = "N";
		else	s = "n";}
	else if (key == KC_M){
		if (shift||capslock)	s = "M";
		else	s = "m";}
	else if (key == KC_NUMPAD7)
		s = "7";
	else if (key == KC_NUMPAD8)
		s = "8";
	else if (key == KC_NUMPAD9)
		s = "9";
	else if (key == KC_SUBTRACT)// - on numeric keypad
		s = "-";
	else if (key == KC_NUMPAD4)
		s = "4";
	else if (key == KC_NUMPAD5)
		s = "5";
	else if (key == KC_NUMPAD6)
		s = "6";
	else if (key == KC_NUMPAD1)
		s = "1";
	else if (key == KC_NUMPAD2 )
		s = "2";
	else if (key == KC_NUMPAD3)
		s = "3";
	else if (key == KC_NUMPAD0)
		s = "0";

	if(!alphanumericOnly)
	{
		if (key == KC_SPACE )
			s = " ";
		else if (key == KC_LBRACKET){
			if (shift)	s = "{";
			else	s = "[";}
		else if (key == KC_RBRACKET){
			if (shift)	s = "}";
			else	s = "]";}
		else if (key == KC_PERIOD){
			if (shift)	s = ">";
			else	s = ".";}
		else if (key == KC_COMMA){
			if (shift)	s = "<";
			else	s = ",";}
		else if (key == KC_SEMICOLON){
			if (shift)	s = ":";
			else	s = ";";}
		else if (key == KC_SLASH){
			if (shift)	s = "?";
			else	s = "/";}
		else if (key == KC_SLASH){
			if (shift)	s = "?";
			else	s = "/";}
		else if (key == KC_BACKSLASH){
			if (shift)	s = "|";
			else	s = "/";}  //lets keep it forward slash,  just in case someone can use a backslash to exploit some parsing function.
		else if (key == KC_APOSTROPHE){
			if (shift)	s = "\"";
			else	s = "'";}
		else if (key == KC_EQUALS){
			if (shift)	s = "+";
			else	s = "=";}
		else if (key == KC_ADD) // + on keypad
			s = "+";
		else if (key == KC_NUMPAD0)
			s = "0";
		else if (key == KC_MULTIPLY)// * on numeric keypad
			s = "*";
		else if(shift)
		{
			if (key == KC_1 )
				s = "!";
			else if (key == KC_2 )
				s = "@";
			else if (key == KC_3 )
				s = "#";
			else if (key == KC_4 )
				s = "$";
			else if (key == KC_5 )
				s = "%";
			else if (key == KC_6 )
				s = "^";
			else if (key == KC_7 )
				s = "&";
			else if (key == KC_8 )
				s = "*";
			else if (key == KC_9 )
				s = "(";
			else if (key == KC_0 )
				s = ")";
		}
		//else if (key == KC_RETURN = )   // Enter on main keyboard
		//	else if (key == KC_GRAVE       = 0x29,    // accent
		//else if (key == KC_TAB )
	}

	return s;
}


