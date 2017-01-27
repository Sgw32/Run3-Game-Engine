//improved Ogre::ConfigFile thomas {AT} thomasfischer {DOT} biz 3/2008
#ifndef IMPROVEDCONFIGFILE_H_
#define IMPROVEDCONFIGFILE_H_

#include <OgrePrerequisites.h>

namespace Ogre
{

class ImprovedConfigFile : public ConfigFile
{
public:
   ImprovedConfigFile() : separators(), filename()
   {
      ConfigFile::ConfigFile();
   }
   
   ~ImprovedConfigFile()
   {
   }
    
   // note: saving is only supported for direct loaded files atm!
   void ImprovedConfigFile::load(const String& filename, const String& separators, bool trimWhitespace)
    {
      this->separators = separators;
      this->filename = filename;
      ConfigFile::load(filename, separators, trimWhitespace);
    }
   
	void ImprovedConfigFile::load(const String& filename)
    {
		this->filename = filename;
      ConfigFile::load(filename);
    }

   bool ImprovedConfigFile::save()
   {
      if(!filename.length())
      {
         OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Saving of the configuration File is only allowed when the configuration was not loaded using the resource system!", "ImprovedConfigFile::save");
         return false;
      }
      FILE *f = fopen(filename.c_str(), "w");
      if(!f)
      {
         OGRE_EXCEPT(Exception::ERR_FILE_NOT_FOUND, "Cannot open File '"+filename+"' for writing.", "ImprovedConfigFile::save");
         return false;
      }

      SettingsBySection::iterator secIt;
      for(secIt = mSettings.begin(); secIt!=mSettings.end(); secIt++)
      {
         if(secIt->first.size() > 0)
            fprintf(f, "[%s]\n", secIt->first.c_str());
         SettingsMultiMap::iterator setIt;
         for(setIt = secIt->second->begin(); setIt!=secIt->second->end(); setIt++)
         {
            fprintf(f, "%s%c%s\n", setIt->first.c_str(), separators[0], setIt->second.c_str());
         }
         
      }
      fclose(f);
      return true;
   }

   void setSetting(String &key, String &value, String section = StringUtil::BLANK)
   {
      SettingsMultiMap *set = mSettings[section];
      if(!set)
      {
         // new section
         set = new SettingsMultiMap();
         mSettings[section] = set;
      }
      if(set->count(key))
         // known key, delete old first
         set->erase(key);
        // add key
      set->insert(std::multimap<String, String>::value_type(key, value));
   }

   String ImprovedConfigFile::getSetting(const String &key, const String &section = StringUtil::BLANK, const String &defaultval = StringUtil::BLANK)
   {
	   String data = ConfigFile::getSetting(key,section,defaultval);
	   setSetting(String(key.c_str()),String(data.c_str()),"");
	   return data;
   }

   // type specific implementations
   Radian getSettingRadian(String key, String section = StringUtil::BLANK) { return StringConverter::parseAngle(ConfigFile::getSetting(key, section));   }
   void setSetting(String key, Radian value, String section = StringUtil::BLANK) { setSetting(key, StringConverter::toString(value), section);   }

   bool getSettingBool(String key, String section = StringUtil::BLANK) { return StringConverter::parseBool(ConfigFile::getSetting(key, section)); }
   void setSetting(String key, bool value, String section = StringUtil::BLANK) { setSetting(key, StringConverter::toString(value), section); }

   Real getSettingReal(String key, String section = StringUtil::BLANK) { return StringConverter::parseReal(ConfigFile::getSetting(key, section)); }
   void setSetting(String key, Real value, String section = StringUtil::BLANK) { setSetting(key, StringConverter::toString(value), section); }

   int getSettingInt(String key, String section = StringUtil::BLANK) { return StringConverter::parseInt(ConfigFile::getSetting(key, section)); }
   void setSetting(String key, int value, String section = StringUtil::BLANK) { setSetting(key, StringConverter::toString(value), section); }

   unsigned int getSettingUnsignedInt(String key, String section = StringUtil::BLANK) { return StringConverter::parseUnsignedInt(ConfigFile::getSetting(key, section)); }
   void setSetting(String key, unsigned int value, String section = StringUtil::BLANK) { setSetting(key, StringConverter::toString(value), section); }

   long getSettingLong(String key, String section = StringUtil::BLANK) { return StringConverter::parseLong(ConfigFile::getSetting(key, section)); }
   void setSetting(String key, long value, String section = StringUtil::BLANK) { setSetting(key, StringConverter::toString(value), section); }

   unsigned long getSettingUnsignedLong(String key, String section = StringUtil::BLANK) { return StringConverter::parseUnsignedLong(ConfigFile::getSetting(key, section)); }
   void setSetting(String key, unsigned long value, String section = StringUtil::BLANK) { setSetting(key, StringConverter::toString(value), section); }

   Vector3 getSettingVector3(String key, String section = StringUtil::BLANK) { return StringConverter::parseVector3(ConfigFile::getSetting(key, section)); }
   void setSetting(String key, Vector3 value, String section = StringUtil::BLANK) { setSetting(key, StringConverter::toString(value), section); }

   Matrix3 getSettingMatrix3(String key, String section = StringUtil::BLANK) { return StringConverter::parseMatrix3(ConfigFile::getSetting(key, section)); }
   void setSetting(String key, Matrix3 value, String section = StringUtil::BLANK) { setSetting(key, StringConverter::toString(value), section); }

   Matrix4 getSettingMatrix4(String key, String section = StringUtil::BLANK) { return StringConverter::parseMatrix4(ConfigFile::getSetting(key, section)); }
   void setSetting(String key, Matrix4 value, String section = StringUtil::BLANK) { setSetting(key, StringConverter::toString(value), section); }

   Quaternion getSettingQuaternion(String key, String section = StringUtil::BLANK) { return StringConverter::parseQuaternion(ConfigFile::getSetting(key, section)); }
   void setSetting(String key, Quaternion value, String section = StringUtil::BLANK) { setSetting(key, StringConverter::toString(value), section); }

   ColourValue getSettingColorValue(String key, String section = StringUtil::BLANK) { return StringConverter::parseColourValue(ConfigFile::getSetting(key, section)); }
   void setSetting(String key, ColourValue value, String section = StringUtil::BLANK) { setSetting(key, StringConverter::toString(value), section); }

   StringVector getSettingStringVector(String key, String section = StringUtil::BLANK) { return StringConverter::parseStringVector(ConfigFile::getSetting(key, section)); }
   void setSetting(String key, StringVector value, String section = StringUtil::BLANK) { setSetting(key, StringConverter::toString(value), section); }

protected:
   String separators;
   String filename;
};

};
#endif