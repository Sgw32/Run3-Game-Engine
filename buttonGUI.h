/*
	buttonGUI  v1.23
	This file is part of buttonGUI, a class that allows developers to easily and
	simply make interactive graphical user interface within an Ogre3D application.

	The intention of this library is to provide the simplicity of betaGUI combined with the ease of use and intuitiveness of Navi Library

	This class contains code from and has heavy influence from:
	Navi Library By Adam J. Simmons 
	BetaGUI class by Robin "Betajaen" Southern

	Brought to you by The Lo-Fi Apocalypse    http://www.lfa.com/
	Copyright (C) 2008 Jorge D. Hernandez (metaldev)

	If you use this class,  I love to see your project!  Post screenshots on the LFA forums or the Ogre forums!

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef LFA_BUTTONGUI_H
#define LFA_BUTTONGUI_H


#include "Ogre.h"
#include "OIS/OIS.h"
#include "OgrePanelOverlayElement.h"
#include "OgreTextAreaOverlayElement.h"
#include "OgreFontManager.h"

using namespace OIS;
using namespace Ogre;


namespace buttonGUI {

	enum relativePosition
	{
		LEFT,
		TOP_LEFT,
		TOP_CENTER,
		TOP_RIGHT,
		RIGHT,
		BOTTOM_RIGHT,
		BOTTOM_CENTER,
		BOTTOM_LEFT,
		CENTER
	};

	/**
	* An object that holds position-data for a button. Used by buttonManager::createButton and buttonManager::setButtonPosition.
	*/
	class buttonPosition
	{
		bool usingRelative;
		union {
			struct { relativePosition position; short x; short y; } rel;
			struct { short left; short top; } abs;
		} data;

		friend class button;
		friend class buttonManager;
		friend class buttonMesh;
		buttonPosition();
	public:
		/**
		* Creates a relatively-positioned buttonPosition object.
		*
		* @param	relPosition		The position of the button in relation to the Render Window or parent button (if this button is a child).
		*
		* @param	offsetLeft	How many pixels from the left to offset the button from the relative position.
		*
		* @param	offsetTop	How many pixels from the top to offset the button from the relative position.
		*/
		buttonPosition(const relativePosition &relPosition, short offsetLeft = 0, short offsetTop = 0);

		/**
		* Creates an absolutely-positioned buttonPosition object.
		*
		* @param	absoluteLeft	The number of pixels from the left of the Render Window.
		*
		* @param	absoluteTop	The number of pixels from the top of the Render Window.
		*/
		buttonPosition(short absoluteLeft, short absoluteTop);
	};



	enum buttonAction
	{
			ONCLICK = 0,
			ONRELEASE,
			MOUSEOVER,
			MOUSEOFF,
			MOUSEWHEELUP,
			MOUSEWHEELDOWN,
			KEYACCEPTED,
			KEYREJECTED,
			ONSUBMIT		//only when the enter key is pressed with a text field active
	};

	class button;
	class buttonEvent  
	{
	public:
		buttonEvent(buttonAction a, OIS::MouseButtonID id, button * mainButton, button * dragDroppedButton = NULL, Ogre::UTFString data = "") :
			action(a),
			mouseButton(id),
			actionButton(mainButton),
			droppedButton(dragDroppedButton),
			additionalData(data)
			{}
		buttonAction action;
		OIS::MouseButtonID mouseButton;
		button * actionButton;
		button * droppedButton;
		Ogre::UTFString additionalData;
	};

	class buttonTextColor
	{
	public:
		buttonTextColor(float r, float g, float b, float a);
		~buttonTextColor();

		Ogre::ColourValue cTop;    //these colors are applied to text accordingly so you can have gradients in your text.
		Ogre::ColourValue cBottom;
	};


	class textScheme
	{
	public:
		textScheme(std::string font, short unsigned size, float r, float g, float b, float a);
		~textScheme();

		std::string mFont;
		short unsigned mFontSize;
		buttonTextColor cOnClick;
		buttonTextColor cOnRelease;
		buttonTextColor cMouseOver;
		buttonTextColor cMouseOff;
	};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    **  BUTTON  CLASS    **   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class textInputArea;

	class button 
	{
		friend class buttonManager;
		friend class buttonMesh;

	public:

		button(std::string &buttonName, std::string &material, buttonPosition &position, short width, short height, buttonManager * mgr, Ogre::Overlay * o, button * parentButton, textScheme &style, bool isActiveButton);
			~button();

			/**
			*		Gets the name of this button
			*/
			std::string * getName(void) {return &name;}

			/**
			*		Allows you to choose a new name for this button.
			*/
			button * setName(std::string newName) {name = newName; return this;}

			/**
			*  Adds a sub-button to your button.
			*
			* @param	name		name of the button for later access
			*
			* @param	material		This material name will be used for all states unless:
			*		materialName.onClick  materialName.onRelease   materialName.mouseOver   materialName.mouseOff    
			*		are defined.  You can also use "" or "BLANK"  to have the button be transparent.
			*
			* @param	position		The buttonPosition to set the button to, will define where this button exists on the screen.
			*		
			* @param	width			width of the button,  in pixels.	
			*
			* @param	height		height of the button,  in pixels.	
			*		
			* @param	scheme		the textScheme that you want all text areas associated with this button to have,  if you don't care about it,  use the other override function
			*								and it will inherit the textScheme from the parent button	
			*		
			* @param	isActiveButton	sets whether this button is a static image.  non-Active buttons can't be triggered and can't be moved unless the parent is moved or unless you 
			*										change it later using setActive();
			*.returns a pointer to the new child button.
			*/
			button * createChildButton(std::string name, std::string material, buttonPosition &position, short  width, short  height, bool isActiveButton = true);
			button * createChildButton(std::string name, std::string material, buttonPosition &position, short  width, short  height, textScheme &scheme, bool isActiveButton = true);
			

			/**
			*  Adds a textArea (text element) to your button.
			*
			* @param	name	name of the textArea,  use this as an identifier to access it again with editTextArea().
			*
			* @param	value		The display text
			*
			* @param	posX		The horizontal position you want to put this textArea in relation to its parent button, in pixels.
			*
			* @param	posY		The vertical position you want to put this textArea in relation to its parent button, in pixels.
			*
			* @param	a  The alignment of the text,  Left/Center/Right
			*		
			*  NOTE:  you can't specify a particular textScheme for a text area, textSchemes are per-button.
			*		If you need a text area with a separate scheme you can make an invisible child button with a text area.
			* returns a pointer to self.
			*/
			button *  addTextArea(std::string name, Ogre::UTFString value, short posX, short posY, TextAreaOverlayElement::Alignment a = TextAreaOverlayElement::Center);

			/**
			*	Modify the text value of a particular textArea of this button.
			*
			* @param	name	name of the textArea,  use this as an identifier to access it again with editTextArea().
			*
			* @param	value		The display text
			*
			* returns a pointer to self.
			*/
			button *  editTextArea(std::string name,  Ogre::UTFString &value);

			/**
			*  Hide or show a particular child textArea of this button.
			*
			*	returns a pointer to self.
			*/
			button * setTextVisibility(std::string name, bool onOff);

			/**
			*  Creates a specialized button called a textInputArea which allows text input and submission
			*
			* @param	textInputName	name of the textInputArea,  use this as an identifier to access it again with buttonManager->getTextInputArea().
			*
			* @param	a	the alignment of the input text.
			*
			* @param	position		how you want the textInputArea aligned to either the screen or the parent.
			*
			* @param	width		The width of the textInputArea, in pixels.
			*
			* @param	height	The height of the textInputArea,  in pixels.
			*
			* @param	textPosX	The horizontal position you want the text to appear, relative to the left side of the textInputArea
			*
			* @param	textPosY	The vertical position you want the text to appear, relative to the top of the textInputArea
			*
			* @param isActiveButton	whether the textInputArea should respond to user input yet.
			*
			* returns a pointer to the newly created textInputArea.
			*/
			textInputArea * createTextInputArea(std::string textInputName, TextAreaOverlayElement::Alignment a, buttonPosition &position, short width, short height, short textPosX, short textPosY,  bool isActiveButton = true) ;
			textInputArea * createTextInputArea(std::string textInputName, TextAreaOverlayElement::Alignment a, std::string material, buttonPosition &position, short width, short height, short textPosX, short textPosY,  bool isActiveButton = true) ;
			textInputArea * createTextInputArea(std::string textInputName, TextAreaOverlayElement::Alignment a, std::string material, buttonPosition &position, short width, short height, short textPosX, short textPosY,  textScheme &style, bool isActiveButton = true) ;
			
			/**
			*  Adds a mesh to your button.
			*
			* @param	name	name of the buttonMesh,  use this as an identifier to access it again with getButtonMesh().
			*
			* @param	meshFile	is the .mesh file you want to load ie  model.mesh
			*
			* @param	positionX	The horizontal position you want to put this buttonMesh in relation to its parent button, in pixels.
			*
			* @param	positionY	The vertical position you want to put this buttonMesh in relation to its parent button, in pixels.
			*
			* @param	width		The width of the buttonMesh, in pixels.
			*
			* @param	height	The height of the buttonMesh in pixels.
			*
			* returns a pointer to the newly created buttonMesh.
			*/
			buttonMesh * addButtonMesh(std::string name, std::string meshFile,   short positionX, short positionY, short width,  short height);
	
			/**
			* Returns a pointer to the named child buttonMesh 
			*/
			buttonMesh * getButtonMesh(std::string name);

			/**
			* Returns a pointer to the named child button 
			*/
			button * getChildButton(std::string name);

			/**
			* hides the button if it was showing.
			* @param fade   if you want the button to hide smoothly or immediately
			*
			* @param fadeDurationMS   milliseconds for the fade to occur.
			*
			*returns a pointer to self
			*/
			button *  hide(bool fade, unsigned short fadeDurationMS = 300);
						/**
			* shows the button if it was hidden
			* @param fade   if you want the button to show smoothly or immediately
			*
			* @param fadeDurationMS   milliseconds for the fade to occur.
			*
			*returns a pointer to self
			*
			*/
			button * show(bool fade, unsigned short fadeDurationMS = 300);

			/**
			*returns if the button is visible or not.
			*/
			bool getVisibility(void); 

			/**
			* @param	material		This material name will be used for all states unless:
			*		materialName.onClick  materialName.onRelease   materialName.mouseOver   materialName.mouseOff    exist.
			*		
			*	returns a pointer to self.
			*/
			button *  setMaterial (std::string materialName);

			/**
			* Changes the overall opacity  to a certain percentage.
			* @param	opacity		The opacity percentage as a float. 
			*						Fully Opaque = 1.0, Fully Transparent = 0.0.

			* returns a pointer to self
			*/
			button *  setOpacity(float opacity);

			/** 
			* Sets the default position of this button to a new position and then moves
			* the button to that position.
			*
			* @param	position		The new buttonPosition to set the button to.

			* returns a pointer to self.
			*/
			button *  setPosition(buttonPosition &position);

			/** 
			* this will offset a button from its current position, without disturbing its relative positioning 
			* to match a particular coords relative to the screen.
			* returns a pointer to self.
			*/
			button * offsetPositionToScreenCoord(int posX, int posY); 

			/**
			*	Sets a new size for the button.
			*	returns a pointer to self
			*/
			button *  setSize(short unsigned int width, short unsigned int height);  

			/**
			* Use this to change the UV mapping on this button  (to only use a part of the texture applied to it) 
			*
			* @params u1, v1   represent the upper left coordinates of the area you want to cover
			*
			* @params u2, v2	represent the lower right coordinates of the area you want to cover
			* returns a pointer to self.
			*/
			button *  setUV(Ogre::Real u1, Ogre::Real v1, Ogre::Real u2, Ogre::Real v2);

			/**
			* Returns a copy of this button's textScheme
			*/
			textScheme getTextScheme(void) {return textStyle;}  //returning a reference would not be safe because changing it would not trickle down to the button's textAreas. Trickling down is only done through setTextScheme(), so returning a copy is better.

			button *  setTextScheme(textScheme &scheme);

			/**
			* Moves the button to the last stored position
			*/
			void  resetPosition(void);

			/**
			* Gets whatever position was last given to the button 
			*/
			const buttonPosition * getPosition(void);
			/**
			* Position data is private to classes outside of buttonGUI
			* this override allows access to this data.
			* returns true if using relative,  false if using absolute positioning.
			*/
			bool getPosition(relativePosition &rel,  short &x, short &y);
			
			bool getPosition(short &x, short &y);

			/**
			* Gets the absolute position of the button ( relative to the screen.)
			*/
			buttonPosition getAbsolutePosition(void);

			/**
			* Toggles whether or not this button is movable. 
			*
			* @param	m	Whether or not this button should be movable.
			*
			* @param centerButtonOnCursor  Whether the button centers to the mouse when it is dragged,  or whether it is dragged relative to where the onClick occured (recommended for large buttons).
			*
			* returns a pointer to self.
			*/
			button *  setMovable(bool m , bool centerButtonOnCursor = true)		{movable = m; centerOnGrab = centerButtonOnCursor; return this;}

			/**
			* Returns if the button is allowed to be moved (grabbed) by the user
			*/
			bool getMovable(void)			{return movable;}

			/**
			* Returns if the button is allowed to receive inputs/be moved etc
			*/
			bool getIsActive(void) {return isActive;}
			/**
			* Sets this button to be triggerable in any way.  (set to false if you just want to use it as a background image for example)
			*
			* returns a poitner to self.
			*/
			button *  setActive(bool s) {isActive = s; return this;}

			/**
			* Allows you to set the trigger for a particular action on this button so that it will be reported (or not)
			* inside buttonManager->getEvent()    The only reason to turn of triggers is for efficiency or to reduce feedback from the buttonMgr
			*  
			*
			* @param	action  The buttonAction that you want to set or unset the trigger for.
			*
			* @param onOff		Whether you want to turn on or turn off the trigger
			*
			* returns a pointer to self.
			*/
			button * setTrigger(buttonAction action, bool onOff);

			/**
			* Get the current trigger setting for a particular action on this button.
			*  The trigger determines if this particular buttonAction should be sent into the buttonManager event log.
			*
			* @param	action  The buttonAction that you want to get the trigger for on this button.
			*
			*/
			bool getTrigger(buttonAction action);

			/**
			*  Sets a new parent for this button.  
			*
			* @param  parentName	the name of an existing button to use as a parent.
			* 
			*/
			void setParent(std::string newParentName);
			void setParent(button * newParentButton);

			/**
			* Returns the parent button of this button,  returns NULL if it is a top level button.
			*/
			button * getParent (void) {return parent;}

			/**
			*  Sets the zOrder for this button.
			*
			* NOTE:  this will only have an effect on top-level buttons. returns true if successful,  false otherwise.
			*/
			bool setZOrder(short unsigned int z);

			/**
			*  Sets limits for the position of this button 
			*  Limits are relative to whatever position type you used
			*  For example, if you used BOTTOM_LEFT in the last positioning of the button,  
			*  these limits will be relative to BOTTOM_LEFT
			*
			* NOTE:  this is useful if you want to make a slider or something that can be grabbed but is 'contained'
			*/
			button * setLimits(bool useLimits);
			button * setLimits(short int x1, short int y1, short int x2, short int y2, bool useLimits = true);
			

			/**
			* Returns the raw vector containing pointers to all the textAreas (TextAreaOverlayElements) of this button.
			*/
			std::vector<Ogre::TextAreaOverlayElement*> getAllTextAreas(void) {return textAreas;}

			/**
			* Returns the text contained in the given text area,  will return "" if the text area does not exist.
			*/
			std::string getTextFromTextArea(std::string textAreaName);

			Ogre::Real getWidth(void)	{ return panel->getWidth();}
			Ogre::Real getHeight(void)	{ return panel->getHeight();}
			String mLua;
		protected:
				buttonPosition position;  
				bool movable;
				bool isActive;
				std::string name;
				button * parent;	//this will be NULL if the button is a top level button,  otherwise will contain a pointer to the parent button
				Ogre::Overlay * overlay;  //this should be NULL if this button is the child of another button,  otherwise this is one of the topButtons and will contain its own overlay and allow zOrder modifications
				Ogre::PanelOverlayElement * panel; 
				buttonManager * const buttonMgr;
				bool hitTest(int x, int y);
				bool isMouseOver;
				std::vector<button*>::iterator buttonItr;
				std::vector<button*> childButtons; 
				std::vector<buttonMesh*>::iterator meshItr;
				std::vector<buttonMesh*> buttonMeshes; 
				std::vector<Ogre::TextAreaOverlayElement*>::iterator textItr;
				std::vector<Ogre::TextAreaOverlayElement*> textAreas;
				textScheme textStyle;

				void update(void); //this is protected because you should update all buttons by only calling update() on the manager.
				void setMaterialOpacity(float opacity, Ogre::MaterialPtr &mat);
				virtual void setSuppressed(bool onOff);
				bool suppressed;	//suppressed refers to if the button or one if its parents is faded and should not return events,  visibility refers to its actual panel or overlay (hence bool visibility does not exist in the button class)
				bool fadingOut;
				unsigned long fadingOutStart;
				unsigned long fadingOutEnd;
				bool fadingIn;
				unsigned long fadingInStart;
				unsigned long fadingInEnd;
				Ogre::Timer timer;

				//these hold the names of available materials
				std::string onClickMaterial;
				std::string onReleaseMaterial;
				std::string mouseOverMaterial;
				std::string mouseOffMaterial;

				bool trigger_onClick;
				bool trigger_onRelease;
				bool trigger_mouseOver;
				bool trigger_mouseOff;
				bool trigger_onSubmit;

				bool onClick(bool rotateMB);  //argument states if the onclick event should be passed down to the buttonMeshes
				bool onRelease(void);
				bool mouseOver(void);
				bool mouseOff(void);

				bool centerOnGrab;	//whether button centers to mouse when grabbed.
				virtual bool setFocused(bool state) {return false;} 
				virtual bool isTextInputArea(void) {return false;}

				bool useLimits;
				short int x1Limit; // these are for using this button as a slider or something that you want to limit how far it can be moved from the parent.  0 = no limit
				short int x2Limit;
				short int y1Limit;
				short int y2Limit;
	};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    ** TEXTINPUTAREA CLASS    **   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class textInputArea : public button
	{
		friend class buttonManager;

		public:
			textInputArea(buttonManager * mgr, std::string &textInputName, TextAreaOverlayElement::Alignment a, std::string &material, buttonPosition &position, button * parentButton, short width, short height, short textPosX, short textPosY,  textScheme &style, bool isActiveButton) ;
			~textInputArea();

			bool input(std::string s);
			bool insertBackspace(void);
			Ogre::UTFString * getValue(void);
			/**
			* textInput areas can have a separate material for when they are  the active textInputArea,  you can set it with this function.
			*  NOTE: .onClick  .onRelease  .mouseOver  .mouseOff  variants are also valid for this active state.
			* returns pointer to self.
			*/
			textInputArea * setActiveMaterial(std::string material);	

			textInputArea *  setTextScheme(textScheme &scheme);

			/**
			* forces the given text to replace the text inside the textInputArea
			* returns pointer to self.
			*/
			textInputArea * setDisplayText(std::string &s);

			/**
			* Returns the text contained in the input field
			* This will return the correct text even in password mode.
			*/
			std::string getDisplayText(void) {return textValue;}

			/**
			* Removes the text currently in the textInputArea without submitting.
			* returns pointer to self.
			*/
			textInputArea * clearText(void);

			/**
			* Send a submit message from buttonMgr with a pointer to this button and the text it contains in additionalData
			*  will also clear the textInputArea if  clearTextOnSubmit is set to on.
			*/
			void submit(void);

			/**
			* Set the maximum number of input characters that this textInputArea is allowed to have.
			* NOTE:  this will truncate an existing string if it already has input.
			* returns pointer to self.
			*/
			textInputArea * setMaxCharacterLimit(short unsigned int num);

			/**
			* Returns whether only numbers and letters are allowed (true) in the textInputArea or any translatable characters available (false).
			*/
			bool getAlphanumericOnly(void) {return alphanumericOnly;}
			/**
			* Sets whether this textInputArea should accept any available character or only numbers and letters.
			* returns pointer to self.
			*/
			textInputArea * setAlphanumericOnly(bool onOff)		{ alphanumericOnly = onOff; return this;}

			/**
			* Returns whether this textInputArea will clear its text automatically when the enter key is pressed while
			* it is focused (when it sends a submit message with the text to the buttonEvent log).  
			*/
			bool getClearTextOnSubmit(void){return clearTextOnSubmit;}
			/**
			* Sets whether this textInputArea should clear the text whenever the user presses enter when focused and it submits an event with the text 
			* to the event log.
			* returns pointer to self.
			*/
			textInputArea * setClearTextOnSubmit(bool onOff)			{ clearTextOnSubmit = onOff; return this;}
			/**
			* Sets whether this textInputArea should defocus when its contents are submitted by the user.
			* returns pointer to self.
			*/
			textInputArea * setDefocusOnSubmit(bool onOff)			{ defocusOnSubmit = onOff; return this;}
			/**
			* Returns whether this textInputArea will defocus when its contents are submitted.
			*/
			bool getDefocusOnSubmit(void){return defocusOnSubmit;}

			textInputArea * setInputIsPassword(bool onOff) {isPasswordField = onOff; updateText(); return this;}
			bool getInputIsPassword(void) {return isPasswordField;}

	protected:
			std::string activatedMaterial;
			std::string deactivatedMaterial;
			bool setFocused(bool state);  
			bool alphanumericOnly;
			bool clearTextOnSubmit;
			bool defocusOnSubmit;
			bool isPasswordField;
			Ogre::TextAreaOverlayElement* dynamicTextElement;
			Ogre::UTFString textValue;
			short unsigned int maxCharacterLimit;
			bool isTextInputArea(void) {return true;}
			void updateText(void);

			void setSuppressed(bool onOff); //need extra actions on textInputArea in case we are active when we get suppressed
	};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    **  BUTTONMESH CLASS    **   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class buttonMesh  //internal class to help manage 3D stuff in the buttons
	{
		friend class button;
		friend class buttonManager;

		public:
			buttonMesh(const Ogre::String name, const Ogre::String meshFile, buttonManager * buttonMgr, button* parentButton, short unsigned int zOrder, short positionX, short positionY,  short width, short height) ;
			~buttonMesh();


			/**
			* Set the size of this buttonMesh relative to the screen.  (The scale of the node will be adjusted accordingly.)
			* @param  w   the width,  in pixels
			*	
			* @param  h   the height,  in pixels
			*/
			buttonMesh * setDimensions(short w, short h);

			//set the relative position using the upper left corner of the buttonMesh as a pivot in absolute screen coordinates.
			buttonMesh * setPosition(short x, short y);

			/**
			* Set the actual distance of the object from the camera,  the object will look exactly the same on the screen.
			*	One might use this to help cheat problems with sorting,  default is 1.
			* 
			*/
			buttonMesh * setZoom(Ogre::Real z);

			/**
			* Set the rotation you would like the buttonMesh to appear.
			* 
			*/
			buttonMesh * setRotation(Ogre::Real x, Ogre::Real y, Ogre::Real z);
			
			buttonMesh * setRotation(Quaternion rot);
			/**
			* Sets the drawing order of this buttonMesh
			*/
			buttonMesh * setZOrder(short unsigned int z) {overlay->setZOrder(z); return this;}

			/**
			* Sets the opacity of the buttonMesh,  beware with non-convex shapes may cause sorting problems with itself.
			*/
			buttonMesh * setOpacity(float opacity);

			/**
			* Sets if the buttonMesh should be interactively rotatable by the user in X and/or y
			*/
			buttonMesh * setRotatable(bool yesNoX, bool yesNoY)  {rotatableX = yesNoX;  rotatableY = yesNoY; return this;}

			/**
			* Returns if the buttonMesh is interactively rotatable by the user
			*/
			bool isRotatable(void)	 {if (rotatableX) return true; if (rotatableY) return true; return false;}

			/**
			* Returns the buttonMesh's name
			*/
			const std::string * getName(void) {return &name;}
			

		protected:
			const std::string name;
			void resetDimensions(void); //run this if the screen resolution changes.
			void update(void);
			short width;
			short height;
			short relX;	// the relative position to the parent
			short relY;
			Ogre::Real viewPlaneLengthAtOne;  //length of the viewplane at the distance of 1 calculated from the the fov
			Ogre::Real aspectRatio;
			Ogre::Real xModifier;
			Ogre::Real screenSpaceModifier;
			Ogre::Real zoom;
			Ogre::SceneNode * baseNode;
			Ogre::Entity * e;
			button * parent;
			Ogre::Overlay * overlay;
			buttonManager * buttonMgr;
			Ogre::Real scaleModifier;
			const Ogre::Real objectAngle;   //this is the radian angle that an object of height 1 @ distance 1 would obscure from a point
			//the following are related to dynamic mesh rotation.
			bool mouseOn;
			bool rotatableX;
			bool rotatableY;
			void injectMouseUp(short &x, short &y);
			void injectMouseDown(short &x, short &y);
			void injectMouseMove(short &x, short &y);
			Ogre::Real friction;
			Ogre::Real turnSpeedModifier;
			Ogre::Real momentumX;
			Ogre::Real momentumY;
			Ogre::Vector2 lastPos;
	};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    **  BUTTON  MANAGER CLASS    **   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class buttonManager
	{
		friend class button;
		friend class buttonMesh;
	public:
		/**
		* to start the manager you need to define a default material for  textInputAreas and a scheme for textAreas
		* these will allow you to use the simplified functions later for creating them.  You will still
		* have the overriden functions with all the options if you need exceptions.
		* If you want to change the defaults you can always use the setDefaults() function and
		* any new buttons/windows/textfields created will use the new default.
		*
		* @param	defaultTextFieldM	The default material to be used for textInputAreas
		*
		* @param	defaultTextScheme 	The default text scheme to be used for any text created, unless specified otherwise.
		*
		* @param	mgr	The current Ogre::SceneManager,  this is needed only if you are using buttonMeshes
		*
		* @param	cameraName 	The name of the camera that is used to display buttonMeshes
		*
		*	NOTE:  for a given material the material name itself will be used unless:
		*		materialName.onClick  materialName.onRelease   materialName.mouseOver   materialName.mouseOff    are defined
		*/
		buttonManager(std::string defaultTextFieldM,  textScheme defaultTextScheme, Ogre::SceneManager *mgr = NULL,  std::string cameraName = "");
		~buttonManager(void);


		/**
		* You can customize the mouse cursor by using this function.
		*  NOTE:  a  button instance is created to represent the cursor graphic.  You can use any of the normal functionality of a button
		*  on the cursor with the exception of setParent() which will have no effect.
		*  returns a pointer to the special button assigned to the cursor, see  getCursorButton()  for more info.
		*
		* @param	material		The Ogre material you want to assign to the cursor.
		*
		* @param	width	/height	The dimensions of the cursor.
		*
		* @param	hotspotX/hotspotY	An offset to use as a trigger point for the cursor.  If your graphic does not lend itself well to pointing from its upper left corner, for example.
		*
		* @param	visibility		Should the cursor be hidden?
		*
		*/
		button * setCursor(std::string material, unsigned short width,  unsigned short height,  unsigned short hotspotX, unsigned short hotspotY, bool visibility = true);

		/**
		* Displays the cursor.
		*	NOTE: won't do anything if you have not called setCursor()
		*	returns a pointer to the cursorButton;
		*/
		button * showCursor(void);
		/**
		* Hides the cursor.
		*	NOTE: won't do anything if you have not called setCursor()
		*	returns a pointer to the cursorButton;
		*/
		button * hideCursor(void);

		/**
		* Injects the mouse's current coordinates in absolute coords when it moves
		* NOTE:  unlike click events, buttons do not block each other from mouseOver and mouseOff commands 
		*/
		bool injectMouseMove(int xPos, int yPos);

		/**
		* Inject the absolute position of the mouse here along with its pressed button.
		*  NOTE:  only the topmost button will be affected
		*/
		bool injectMouseDown(OIS::MouseButtonID &id);

		/**
		* Inject the absolute position of the mouse here along with its raised button.
		*/
		bool injectMouseUp(OIS::MouseButtonID &id);

		/**
		* Inject rotative clicks of the mousewheel as a relative int.   Ex.   1 = one click up,  -2 = two clicks down 
		*/
		bool injectMouseWheel(short int z);

		/**
		* inject your keyEvents here to be able to type into text fields
		* If anybody is knowledgable about unicode i could use help here. (i tried,  but couldn't get it to work without bugs.)
		*/
		bool injectKeyPressed( const OIS::KeyEvent & arg);

		/**
		*  let buttonGUI know when a key has been released .
		*/
		bool injectKeyReleased( const OIS::KeyEvent & arg);
		

		/**
		* Creates a button.
		*
		* @param	name	The name of the button, used to refer to a specific button in subsequent calls.
		*
		* @param material  The name of the material you want to be applied to the button.  You can use "BLANK" to leave it without a material.
		*							A given material name will recognize variants such as  materialName.onClick  materialName.onRelease   materialName.mouseOver   materialName.mouseOff
		*							and use them as appropriate
		*
		* @param	buttonPosition	The unified position (either relative or absolute) of a button.
		*
		* @param	width		The width of the button.
		*
		* @param	height	The height of the button.
		*
		* @param	zOrder		Sets the starting Z-Order for this button; buttons with higher Z-Orders will be on top of other
		*						buttons. To auto-increment this value for every successive button, leave this parameter as '0'.
		*
		*/
		button* createButton (std::string name, std::string material, buttonPosition &position, unsigned short width, unsigned short height, unsigned short zOrder = 0, bool visible = true, bool isActive = true);
		button* createButton (std::string name, std::string material, buttonPosition &position, unsigned short width, unsigned short height, unsigned short zOrder = 0, bool visible = true, bool isActive = true,String lua = "");
		button* createButton (std::string name, std::string material, buttonPosition &position, unsigned short width, unsigned short height, textScheme &scheme, unsigned short zOrder = 0, bool visible = true, bool isActive = true);

		/*
		* set new defaults for future buttons.
		*/
		void setDefaults( std::string defaultTextFieldM,  textScheme defaultTextScheme);

		/*
		* Call this when your resolution changes so that buttonMeshes can be adjusted to the new settings.
		*/
		void resetScreenResolution();  

		/**
		* Returns a pointer to the button with the given name,  NULL if it doesnt exist.
		*
		*  @param		buttonName	The name of the button you created earlier that you now want to get a pointer for.
		*/
		button * getButton(std::string buttonName);

		/**
		* Deletes a particular button.
		*
		*  @param		name	The name of the button or textInputArea you want to destroy.
		*  NOTE:  this function is a little dangerous,  make sure you aren't holding any pointers to deleted buttons or children of deleted buttons.
		*	also make sure you are checking for NULL in places you are asking for getButton("name") in case "name"  has been deleted now.
		*	If in doubt,  just hide buttons until your program is done and call shutdown()
		*/
		void deleteButton(std::string buttonName);
		void deleteButton(button * b);

		/**
		*  clears the buttonMgr of all buttons
		*/
		void deleteAllButtons(void);

		/**
		* Returns a pointer to the textInputArea with the given name,  NULL if it doesnt exist.
		*
		*  @param		name	The name of the textInputArea you created earlier that you now want to get a pointer for.
		*/
		textInputArea * getTextInputArea(std::string name);


		/**
		* Forces a mouse down and a mouse up wherever the mouse is.
		*
		*  @param		mouseButton	The mouse button you want to force a click with.
		*/
		void forceClick(OIS::MouseButtonID mouseButton = MB_Left);

		/**
		* Moves cursor to a button,  then forces a click and release.
		*
		*  @param		mouseButton	The mouse button you want to force a click with.
		*  @param		b the button you want to force click.
		* NOTE: button must be active for it to work
		*/
		void forceClickButton(button *b, OIS::MouseButtonID mouseButton = MB_Left);
		void forceClickButton(std::string buttonName, OIS::MouseButtonID mouseButton = MB_Left);


		/**
		* Returns the last event that occurred since the last time this was called.
		* You should continue calling this function until it returns NULL to clear out all events.
		*/
		buttonEvent * getEvent(void);

		/**
		* Returns a pointer to the cursor button.
		* NOTE: the cursor button is not a regular button,  it does not report nor receive hit events.  You also cannot retreive it using getButton("name").
		* You may however wish to use this pointer for the setParent() of another button if you wish to parent buttons to the cursor.  If you choose to do this,
		* beware of your button's relative position as you may block the hotspot and keep the cursor from triggering onClick events of other, unparented buttons.
		*/
		button * getCursorButton(void) {return cursorButton;}

		/**
		* Returns a pointer to the grabbed button
		* if no button is being grabbed, it returns null;
		*/
		button * getGrabbedButton(void) {return grabbedButton;}

		/**
		* Returns the current hotspot position of the cursor 
		*/
		buttonPosition getCursorPosition(void) {return buttonPosition(mouseX, mouseY);}

		/**
		* This will highlight the next TIA in order of creation. It is connected to TAB to toggle between TIAs at the moment but can be forced here as well.
		* returns the possibly newly active TIA...  will return NULL if there are none or if they are all hidden 
		*/
		textInputArea * cycleTextInputArea(void);

		textInputArea * getActiveTextInputArea(void) {return activeTextInputArea;}
		void clearActiveTextInputArea(void) {activeTextInputArea = 0;}

		/**
		* Call this every frame to have your buttons behave properly.
		*/
		void update(void);

		/**
		* Calls delete on all buttons and destroys all overlays to prepare for the instance of the manager to be deleted and set to NULL
		*/
		void shutdown(void);
	
		/**
		* Returns the raw vector containing pointers to all buttons (except cursor button.)
		*/
		std::vector<button*> getAllButtons(void) {return buttons;}
		
	private:
		button * getTopButton(void);
		
		std::string defaultTextFieldMaterial;
		textScheme defaultTextStyle;	

		short int mouseX;
		short int mouseY;
		short int mouseOffsetX;
		short int mouseOffsetY;

		std::vector<button*>::iterator buttonItr;
		std::vector<button*> buttons; //contains pointers to all the buttons created through the manager (also contains textInputAreas)
		std::vector<buttonMesh*>::iterator meshItr;
		std::vector<buttonMesh*> buttonMeshes;  //contains pointers to every buttonMesh
		unsigned long lastFrameTime;
		unsigned long timeDiff;
		textInputArea* activeTextInputArea; 
		button * grabbedButton;
		button * cursorButton;  //can be used as an attach point for other buttons, will never report events.

		Ogre::Timer timer;
		Ogre::SceneManager * sceneMgr;  //this is only needed for 3D elements in the overlays, you can send NULL if you arent using models in the buttons.
		std::string camera;							//this is only needed for 3D elements in the overlays,  you can throw junk in here if you arent using them,  otherwise,  give it the name of the camera you are using.

		static const short triggerOpacity = 50;   //any windows,  text or buttons  will not react when their opacity is less than this percentage

		unsigned int screenResX;
		unsigned int screenResY;

		unsigned short zOrderCounter;
		MouseButtonID grabbingMouseButton;
		MouseButtonID turningMouseButton; // this is the mouse button that can rotate buttonMeshes.

		std::string eventStringContainer;  //temporary string for transferring event data.
		buttonEvent eventContainer;
		std::vector<buttonEvent> eventLog;

		//Ogre::UTFString keycodeToUTF32( unsigned int scanCode);  //maybe someone knows enough to implement this one day...  i tried but couldnt get it bug free.
		std::string keyCodeToString(const OIS::KeyCode &key, bool shift, bool alphanumericOnly = false) ;
		bool lshift;
		bool rshift;
		bool capslock;
		bool backSpace;
		unsigned long backSpaceHeldTime;
		static const unsigned long backSpaceHeldWait = 300;	//the amount of MS to wait before the characters start to get deleted in a row
		bool backSpaceHeld;														//used to know when to quickly eliminate characters
		static const unsigned long backSpaceFlowTime = 30;		//when held, how long to wait to remove the next character  
	};
}
#endif //LFA_BUTTONGUI_H