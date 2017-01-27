#pragma once
#include <Ogre.h>

using namespace Ogre;

class DisplayLuaCallback:public Singleton<DisplayLuaCallback>
{
public:
	DisplayLuaCallback();
	~DisplayLuaCallback();
	void setWriteLnText(String text)
	{
		wrtLn=text;
	}

	String getWriteLnText(void)
	{
	String twrt = wrtLn;
	wrtLn="";
	return twrt;
	}
	
	void setFirstChar(String text)
	{
		fChar=text;
	}

	String getFirstChar(void)
	{
		String twrt = fChar;
		fChar="";
		return twrt;
	}

	void setDTex(String tex)
	{
		dtex=tex;
	}

	void setDimensions(Real x1,Real y1,Real x2,Real y2);

	void resetDimensions();

	void setFont(String font)
	{
		dfont=font;
	}

	void setLines(String tex)
	{
		lines=StringConverter::parseInt(tex);
	}
	
	void shutdown();

	int getLines(void)
	{
		Real lines2=lines;
		lines=-1;
		return lines2;
	}

	String getFont(void)
	{
		String dfont2 = dfont;
		dfont="";
		return dfont2;
	}

	void setNewLine(void)
	{
		newLine=true;
	}

	bool getNewLine(void)
	{
		bool newLine2 = newLine;
		newLine=false;
		return newLine2;
	}

	void setFontSize(String font)
	{
		fontSize=font;
	}

	String getFontSize(void)
	{
		String dfont2 = fontSize;
		fontSize="";
		return dfont2;
	}

	void setClear()
	{
		doClear=true;
	}

	String getDTex(void)
	{
	String twrt = dtex;
	dtex="";
	return twrt;
	}

	bool getClear(void)
	{
	bool dClear=doClear;
	doClear=false;
	return dClear;
	}
private:
	bool doClear;
	bool newLine;
	Real lines;
	String wrtLn;
	String fontSize;
	String dtex;
	String fChar;
	String dfont;
};