/*----------------------------------------------------------------------------

   libtunepimp -- The MusicBrainz tagging library.  
                  Let a thousand taggers bloom!
   
   Copyright (C) 2006 Lukas Lalinsky
   
   This file is part of libtunepimp.

   libtunepimp is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   libtunepimp is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with libtunepimp; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   $Id$

----------------------------------------------------------------------------*/

#include <string.h>
#include "speexfile.h"
#include "metadata.h"
#include "plugin.h"
#ifndef WIN32
#include "../../lib/utf8/utf8util.h"
#endif

using namespace std;

#ifndef WIN32
#define initPlugin speexInitPlugin
#endif

#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_NAME    "Speex metadata reader/writer"

static char *formats[][2] = {
  { ".spx", "Speex audio format" },
};

#define NUM_FORMATS 1

char *speexErrorString = "";

static void
speexShutdown()
{
/*  if (strlen(speexErrorString))
    free(speexErrorString);*/
}

static const char *
speexGetVersion()
{
  return PLUGIN_VERSION;
}

static const char *
speexGetName()
{
  return PLUGIN_NAME;
}

static int
speexGetNumFormats(void)
{
  return NUM_FORMATS;
}

static int
speexGetFormat(int i, char ext[TP_EXTENSION_LEN],
		char desc[TP_PLUGIN_DESC_LEN],int *functions)
{
  if (i < 0 || i >= NUM_FORMATS)
    return 0;
  
  strcpy(ext, formats[i][0]);
  strcpy(desc, formats[i][0]);
  *functions = TP_PLUGIN_FUNCTION_METADATA;

  return 1;
}

static const char *
speexGetError()
{
  return speexErrorString;
}

static int
speexReadMetadata(metadata_t *mdata, const char *fileName, int flags, const char *encoding)
{
  memset(mdata, 0, sizeof(metadata_t));

#ifndef WIN32
  TagLib::Speex::File f(utf8ToEncoding(fileName, encoding).c_str());
#else
  TagLib::Speex::File f(fileName);
#endif  

  if (f.isOpen() && f.isValid()) {
      
    TagLib::Ogg::XiphComment *tag = f.tag();
    if (!tag)
      return 1;
    
    strcpy(mdata->artist, tag->artist().to8Bit(true).c_str());
    strcpy(mdata->track, tag->title().to8Bit(true).c_str());
    strcpy(mdata->album, tag->album().to8Bit(true).c_str());
    mdata->trackNum = tag->track();
    mdata->releaseYear = tag->year();

    TagLib::AudioProperties *properties = f.audioProperties();
    if (properties)
      mdata->duration = properties->length() * 1000;
 
    strcpy(mdata->fileFormat, "speex");

    const TagLib::Ogg::FieldListMap &items = tag->fieldListMap();

    if (!items["TRACK"].isEmpty()) {
        mdata->totalInSet = 0;
        sscanf(items["TRACK"].toString().toCString(true), "%d/%d", &mdata->trackNum, &mdata->totalInSet);
    }
    
    if (!items["MUSICBRAINZ_TRACKID"].isEmpty()) 
      strcpy(mdata->trackId, items["MUSICBRAINZ_TRACKID"].toString().toCString(true));   

    if (!items["MUSICIP_PUID"].isEmpty()) 
      strcpy(mdata->filePUID, items["MUSICIP_PUID"].toString().toCString(true));   

    if (!items["MUSICBRAINZ_ARTISTID"].isEmpty()) 
      strcpy(mdata->artistId, items["MUSICBRAINZ_ARTISTID"].toString().toCString(true));   

    if (!items["MUSICBRAINZ_ALBUMID"].isEmpty()) 
      strcpy(mdata->albumId, items["MUSICBRAINZ_ALBUMID"].toString().toCString(true));   

    if (!items["MUSICBRAINZ_ALBUMARTISTID"].isEmpty()) 
      strcpy(mdata->albumArtistId, items["MUSICBRAINZ_ALBUMARTISTID"].toString().toCString(true));   

    if (!items["MUSICBRAINZ_ALBUMARTIST"].isEmpty()) 
      strcpy(mdata->albumArtist, items["MUSICBRAINZ_ALBUMARTIST"].toString().toCString(true));   

    if (!items["MUSICBRAINZ_ALBUMARTISTSORTNAME"].isEmpty()) 
      strcpy(mdata->albumArtistSortName, items["MUSICBRAINZ_ALBUMARTISTSORTNAME"].toString().toCString(true));   

    if (!items["MUSICBRAINZ_SORTNAME"].isEmpty()) 
      strcpy(mdata->sortName, items["MUSICBRAINZ_SORTNAME"].toString().toCString(true));   

    if (!items["MUSICBRAINZ_ALBUMTYPE"].isEmpty()) 
      mdata->albumType = convertToAlbumType(items["MUSICBRAINZ_ALBUMTYPE"].toString().toCString(true));
    
    if (!items["MUSICBRAINZ_ALBUMSTATUS"].isEmpty()) 
      mdata->albumStatus = convertToAlbumStatus(items["MUSICBRAINZ_ALBUMSTATUS"].toString().toCString(true));
    
    if (!items["YEAR"].isEmpty()) { 
      int year = 0, month = 0, day = 0;
      if (sscanf(items["YEAR"].toString().toCString(true), "%04d-%02d-%02d", &year, &month, &day) > 0) {
        mdata->releaseYear  = year;
        mdata->releaseMonth = month;
        mdata->releaseDay   = day;
      }
    }
    
    if (!items["MUSICBRAINZ_ALBUMRELEASECOUNTRY"].isEmpty()) 
      strcpy(mdata->releaseCountry, items["MUSICBRAINZ_ALBUMRELEASECOUNTRY"].toString().toCString(true));   
  
    if (!items["MUSICBRAINZ_NONALBUM"].isEmpty()) 
      mdata->nonAlbum = items["MUSICBRAINZ_NONALBUM"].toString().toInt();   

    if (!items["MUSICBRAINZ_VARIOUSARTISTS"].isEmpty()) 
      mdata->variousArtist = items["MUSICBRAINZ_VARIOUSARTISTS"].toString().toInt();
    
    return 1;
  }
  
  return 0;
}

static int
speexWriteMetadata(const metadata_t *mdata, const char *fileName, int flags,
		   const char *encoding)
{
  string temp;  
    
#ifndef WIN32
  TagLib::Speex::File f(utf8ToEncoding(fileName, encoding).c_str());
#else
  TagLib::Speex::File f(fileName);
#endif  

  if (f.isOpen() && f.isValid()) {
      
    TagLib::Ogg::XiphComment *tag = f.tag();
    if (!tag)
      return 1;
    
    tag->setArtist(TagLib::String(mdata->artist, TagLib::String::UTF8));
    tag->setAlbum(TagLib::String(mdata->album, TagLib::String::UTF8));
    tag->setTitle(TagLib::String(mdata->track, TagLib::String::UTF8));
    tag->setTrack(mdata->trackNum);
    tag->setYear(mdata->releaseYear);
    
    tag->addField("MUSICBRAINZ_TRACKID", TagLib::String(mdata->trackId, TagLib::String::UTF8));
    tag->addField("MUSICIP_PUID", TagLib::String(mdata->filePUID, TagLib::String::UTF8));
    tag->addField("MUSICBRAINZ_ARTISTID", TagLib::String(mdata->artistId, TagLib::String::UTF8));
    tag->addField("MUSICBRAINZ_ALBUMID", TagLib::String(mdata->albumId, TagLib::String::UTF8));
    tag->addField("MUSICBRAINZ_SORTNAME", TagLib::String(mdata->sortName, TagLib::String::UTF8));
    
    tag->addField("MUSICBRAINZ_ALBUMARTISTID", TagLib::String(mdata->albumArtistId, TagLib::String::UTF8));
    tag->addField("MUSICBRAINZ_ALBUMARTIST", TagLib::String(mdata->albumArtist, TagLib::String::UTF8));
    tag->addField("MUSICBRAINZ_ALBUMARTISTSORTNAME", TagLib::String(mdata->albumArtistSortName, TagLib::String::UTF8));

    convertFromAlbumType(mdata->albumType, temp);
    tag->addField("MUSICBRAINZ_ALBUMTYPE", TagLib::String(temp, TagLib::String::UTF8));
    
    convertFromAlbumStatus(mdata->albumStatus, temp);
    tag->addField("MUSICBRAINZ_ALBUMSTATUS", TagLib::String(temp, TagLib::String::UTF8));

    if (mdata->releaseYear > 0) {
      char temp[16];
      if (mdata->releaseMonth > 0) {
        if (mdata->releaseDay > 0) {
          sprintf(temp, "%04d-%02d-%02d", mdata->releaseYear, mdata->releaseMonth, mdata->releaseDay);
        }
        else {
          sprintf(temp, "%04d-%02d", mdata->releaseYear, mdata->releaseMonth);
        }
      }
      else {
          sprintf(temp, "%04d", mdata->releaseYear);
      }
      tag->addField("YEAR", TagLib::String(temp, TagLib::String::UTF8));
    }
    
    if (mdata->totalInSet > 0) {
      char temp[16];
      sprintf(temp, "%d/%d", mdata->trackNum, mdata->totalInSet);
      tag->addField("TRACK", TagLib::String(temp, TagLib::String::UTF8));
    }
    
    tag->addField("MUSICBRAINZ_ALBUMRELEASECOUNTRY", TagLib::String(mdata->releaseCountry, TagLib::String::UTF8));

    tag->addField("MUSICBRAINZ_NONALBUM", TagLib::String::number(mdata->nonAlbum));
    tag->addField("MUSICBRAINZ_VARIOUSARTISTS", TagLib::String::number(mdata->variousArtist));
    
    return f.save() ? 1 : 0;
  }
  
  return 0;
}

static unsigned long
speexGetDuration(const char *fileName, int flags, const char *encoding)
{
  return 0;
}

static Plugin methods = {
  speexShutdown,
  speexGetVersion,
  speexGetName,
  speexGetNumFormats,
  speexGetFormat,
  speexGetError,
  speexReadMetadata,
  speexWriteMetadata,
  speexGetDuration,
  NULL,
  NULL,
  NULL,
  NULL
};

extern "C" Plugin *
initPlugin()
{
  return &methods;
}


