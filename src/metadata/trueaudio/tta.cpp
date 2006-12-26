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
#include <id3v2tag.h>
#include <textidentificationframe.h>
#include <uniquefileidentifierframe.h>
#include "ttafile.h"
#include "metadata.h"
#include "plugin.h"
#ifndef WIN32
#include "../../lib/utf8/utf8util.h"
#endif

using namespace std;

#ifndef WIN32
#define initPlugin ttaInitPlugin
#endif

#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_NAME    "The True Audio metadata reader/writer"

static char *formats[][2] = {
  { ".tta", "The True Audio (TTA) format" },
};

#define NUM_FORMATS 1

char *ttaErrorString = "";

static void
ttaShutdown()
{
/*  if (strlen(ttaErrorString))
    free(ttaErrorString);*/
}

static const char *
ttaGetVersion()
{
  return PLUGIN_VERSION;
}

static const char *
ttaGetName()
{
  return PLUGIN_NAME;
}

static int
ttaGetNumFormats(void)
{
  return NUM_FORMATS;
}

static int
ttaGetFormat(int i, char ext[TP_EXTENSION_LEN],
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
ttaGetError()
{
  return ttaErrorString;
}

static TagLib::String
getTextFrame(TagLib::ID3v2::Tag *tag, const TagLib::ByteVector &id)
{
  if(!tag->frameListMap()[id].isEmpty())
    return tag->frameListMap()[id].front()->toString();
  return TagLib::String::null; 
}

static TagLib::String
getUserTextFrame(TagLib::ID3v2::Tag *tag, const TagLib::String &desc)
{
  TagLib::ID3v2::FrameList l = tag->frameList("TXXX");
  for (TagLib::ID3v2::FrameList::Iterator it = l.begin(); it != l.end(); ++it) {
    TagLib::ID3v2::UserTextIdentificationFrame *f =
      static_cast<TagLib::ID3v2::UserTextIdentificationFrame *>(*it);
    if (f && f->description() == desc) 
      return f->fieldList().toString();
  }
  return TagLib::String::null; 
}

static TagLib::String
getUniqueFileIdFrame(TagLib::ID3v2::Tag *tag, const TagLib::String &owner)
{
  TagLib::ID3v2::FrameList l = tag->frameList("UFID");
  for (TagLib::ID3v2::FrameList::Iterator it = l.begin(); it != l.end(); ++it) {
    TagLib::ID3v2::UniqueFileIdentifierFrame *f =
      static_cast<TagLib::ID3v2::UniqueFileIdentifierFrame *>(*it);
    if (f && f->owner() == owner)
      return f->identifier();
  }
  return TagLib::String::null; 
}

static int
ttaReadMetadata(metadata_t *mdata, const char *fileName, int flags, const char *encoding)
{
  memset(mdata, 0, sizeof(metadata_t));

#ifndef WIN32
  TagLib::TTA::File f(utf8ToEncoding(fileName, encoding).c_str());
#else
  TagLib::TTA::File f(fileName);
#endif  

  if (f.isOpen() && f.isValid()) {
      
    TagLib::Tag *t = f.tag();
    if (!t)
      return 1;
    
    strcpy(mdata->artist, t->artist().to8Bit(true).c_str());
    strcpy(mdata->track, t->title().to8Bit(true).c_str());
    strcpy(mdata->album, t->album().to8Bit(true).c_str());
    mdata->trackNum = t->track();
    mdata->releaseYear = t->year();

    TagLib::AudioProperties *properties = f.audioProperties();
    if (properties)
      mdata->duration = properties->length() * 1000;
 
    strcpy(mdata->fileFormat, "tta");

    TagLib::ID3v2::Tag *tag = f.ID3v2Tag();
    if (!tag)
      return 1;
    
    TagLib::String str = getTextFrame(tag, "TRCK"); 
    if (!str.isEmpty()) {
      mdata->totalInSet = 0;
      sscanf(str.toCString(true), "%d/%d", &mdata->trackNum, &mdata->totalInSet);
    }
    
    str = getTextFrame(tag, "TSOP"); 
    if (!str.isEmpty()) {
      strcpy(mdata->sortName, str.toCString(true));   
    }
    
    str = getUniqueFileIdFrame(tag, "http://musicbrainz.org");
    if (!str.isEmpty()) {
      strcpy(mdata->trackId, str.toCString(true));   
    }
    
    str = getUserTextFrame(tag, "MusicIP PUID");
    if (!str.isEmpty()) {
      strcpy(mdata->filePUID, str.toCString(true));   
    }
    
    str = getUserTextFrame(tag, "MusicBrainz Artist Id");
    if (!str.isEmpty()) {
      strcpy(mdata->artistId, str.toCString(true));   
    }
    
    str = getUserTextFrame(tag, "MusicBrainz Album Id");
    if (!str.isEmpty()) {
      strcpy(mdata->albumId, str.toCString(true));   
    }
    
    str = getUserTextFrame(tag, "MusicBrainz Album Artist Id");
    if (!str.isEmpty()) {
      strcpy(mdata->albumArtistId, str.toCString(true));   
    }
    
    str = getUserTextFrame(tag, "MusicBrainz Album Artist");
    if (!str.isEmpty()) {
      strcpy(mdata->albumArtist, str.toCString(true));   
    }
    
    str = getUserTextFrame(tag, "MusicBrainz Album Artist Sortname");
    if (!str.isEmpty()) {
      strcpy(mdata->albumArtistSortName, str.toCString(true));   
    }
    
    str = getUserTextFrame(tag, "MusicBrainz Album Type");
    if (!str.isEmpty()) {
      mdata->albumType = convertToAlbumType(str.toCString(true));
    }
    
    str = getUserTextFrame(tag, "MusicBrainz Album Status");
    if (!str.isEmpty()) {
      mdata->albumStatus = convertToAlbumStatus(str.toCString(true));
    }
    
    str = getUserTextFrame(tag, "MusicBrainz Album Release Country");
    if (!str.isEmpty()) {
      strcpy(mdata->releaseCountry, str.toCString(true));   
    }
    
    str = getUserTextFrame(tag, "MusicBrainz Various Artists");
    if (!str.isEmpty()) {
      mdata->variousArtist = str.toInt();
    }
    
    str = getTextFrame(tag, "TDRC");
    if (!str.isEmpty()) {
      int year = 0, month = 0, day = 0;
      if (sscanf(str.toCString(true), "%04d-%02d-%02d", &year, &month, &day) > 0) {
        mdata->releaseYear  = year;
        mdata->releaseMonth = month;
        mdata->releaseDay   = day;
      }
    }
    
    return 1;
  }
  
  return 0;
}

static void
setTextFrame(TagLib::ID3v2::Tag *tag, const TagLib::ByteVector &id,
             const TagLib::String &value,
             const TagLib::String::Type encoding = TagLib::String::UTF8)
{
  tag->removeFrames(id);
    
  TagLib::ID3v2::TextIdentificationFrame *f =
    new TagLib::ID3v2::TextIdentificationFrame(id, encoding);
  f->setText(value);
  tag->addFrame(f);
}

static void
setUserTextFrame(TagLib::ID3v2::Tag *tag, const TagLib::String &desc,
             const TagLib::String &value,
             const TagLib::String::Type encoding = TagLib::String::UTF8)
{
  TagLib::ID3v2::UserTextIdentificationFrame *f = 0;
 
  // UserTextIdentificationFrame::find in TagLib 1.4 is broken :(
  TagLib::ID3v2::FrameList l = tag->frameList("TXXX");
  for (TagLib::ID3v2::FrameList::Iterator it = l.begin(); it != l.end(); ++it) {
    f = static_cast<TagLib::ID3v2::UserTextIdentificationFrame *>(*it);
    if (f && f->description() == desc) 
      break;
    f = 0;
  }

  if (f)
    tag->removeFrame(f);

  f = new TagLib::ID3v2::UserTextIdentificationFrame(encoding);
  f->setDescription(desc);
  f->setText(value);
  tag->addFrame(f);
}

static void
setUniqueFileIdFrame(TagLib::ID3v2::Tag *tag, const TagLib::String &owner,
             const TagLib::ByteVector &value)
{
  TagLib::ID3v2::UniqueFileIdentifierFrame *f = 0;
    
  TagLib::ID3v2::FrameList l = tag->frameList("UFID");
  for (TagLib::ID3v2::FrameList::Iterator it = l.begin(); it != l.end(); ++it) {
    f = static_cast<TagLib::ID3v2::UniqueFileIdentifierFrame *>(*it);
    if (f && f->owner() == owner) 
      break;
    f = 0;
  }
    
  if (f) {
    f->setIdentifier(value);
  }
  else {
    f = new TagLib::ID3v2::UniqueFileIdentifierFrame(owner, value);
    tag->addFrame(f);
  }
}

static int
ttaWriteMetadata(const metadata_t *mdata, const char *fileName, int flags,
		   const char *encoding)
{
  string temp;  
    
#ifndef WIN32
  TagLib::TTA::File f(utf8ToEncoding(fileName, encoding).c_str());
#else
  TagLib::TTA::File f(fileName);
#endif  

  if (f.isOpen() && f.isValid()) {
      
    TagLib::ID3v2::Tag *tag = f.ID3v2Tag(true);
    
    TagLib::Tag *t = f.tag();
    if (!t)
      return 1;
    
    TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(TagLib::String::UTF8);
    
    t->setArtist(TagLib::String(mdata->artist, TagLib::String::UTF8));
    t->setAlbum(TagLib::String(mdata->album, TagLib::String::UTF8));
    t->setTitle(TagLib::String(mdata->track, TagLib::String::UTF8));
    t->setTrack(mdata->trackNum);
    t->setYear(mdata->releaseYear);

    setTextFrame(tag, "TSOP", TagLib::String(mdata->sortName, TagLib::String::UTF8));
    
    setUniqueFileIdFrame(tag, "http://musicbrainz.org", mdata->trackId);
    setUserTextFrame(tag, "MusicIP PUID", TagLib::String(mdata->filePUID, TagLib::String::UTF8));
    setUserTextFrame(tag, "MusicBrainz Artist Id", TagLib::String(mdata->artistId, TagLib::String::UTF8));
    setUserTextFrame(tag, "MusicBrainz Album Id", TagLib::String(mdata->albumId, TagLib::String::UTF8));
    
    setUserTextFrame(tag, "MusicBrainz Album Artist", TagLib::String(mdata->albumArtist, TagLib::String::UTF8));
    setUserTextFrame(tag, "MusicBrainz Album Artist Id", TagLib::String(mdata->albumArtistId, TagLib::String::UTF8));
    setUserTextFrame(tag, "MusicBrainz Album Artist Sortname", TagLib::String(mdata->albumArtistSortName, TagLib::String::UTF8));

    convertFromAlbumType(mdata->albumType, temp);
    setUserTextFrame(tag, "MusicBrainz Album Type", TagLib::String(temp, TagLib::String::UTF8));
    
    convertFromAlbumStatus(mdata->albumStatus, temp);
    setUserTextFrame(tag, "MusicBrainz Album Status", TagLib::String(temp, TagLib::String::UTF8));

    setUserTextFrame(tag, "MusicBrainz Album Release Country", TagLib::String(mdata->releaseCountry, TagLib::String::UTF8));
    
    setUserTextFrame(tag, "MusicBrainz Various Artists", TagLib::String::number(mdata->variousArtist));

    if (mdata->totalInSet > 0) {
      char temp[16];
      sprintf(temp, "%d/%d", mdata->trackNum, mdata->totalInSet);
      setTextFrame(tag, "TRCK", TagLib::String(temp, TagLib::String::UTF8));
    }
    
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
      setTextFrame(tag, "TDRC", TagLib::String(temp, TagLib::String::UTF8));
    }
    
    return f.save() ? 1 : 0;
  }
  
  return 0;
}

static unsigned long
ttaGetDuration(const char *fileName, int flags, const char *encoding)
{
  return 0;
}

static Plugin methods = {
  ttaShutdown,
  ttaGetVersion,
  ttaGetName,
  ttaGetNumFormats,
  ttaGetFormat,
  ttaGetError,
  ttaReadMetadata,
  ttaWriteMetadata,
  ttaGetDuration,
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


