/***************************************************************************
    copyright            : (C) 2005 by Lukas Lalinsky
    email                : lalinsky@gmail.com
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 ***************************************************************************/

#include <tstring.h>
#include <wmafile.h>
#include <wmatag.h>
#include <wmaproperties.h>

using namespace TagLib;

class WMA::File::FilePrivate
{
public:
  FilePrivate(): size(0), offset1(0), offset2(0), size1(0), size2(0),
    numObjects(0), tag(0), properties(0) {}
  unsigned long long size;
  unsigned long offset1, offset2, size1, size2, numObjects;
  WMA::Tag *tag; 
  WMA::Properties *properties; 
};

// GUIDs

struct WMA::GUID 
{
  WMA::DWORD v1;
  WMA::WORD v2;
  WMA::WORD v3;
  WMA::BYTE v4[8];
  bool operator==(const GUID &g) const { return memcmp(this, &g, sizeof(WMA::GUID)) == 0; }
  bool operator!=(const GUID &g) const { return memcmp(this, &g, sizeof(WMA::GUID)) != 0; }
  static GUID header;
  static GUID fileProperties;
  static GUID streamProperties;
  static GUID contentDescription;
  static GUID extendedContentDescription;
  static GUID audioMedia;
}; 

WMA::GUID WMA::GUID::header = {
  0x75B22630, 0x668E, 0x11CF, { 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C }
}; 

WMA::GUID WMA::GUID::fileProperties = {
  0x8CABDCA1, 0xA947, 0x11CF, { 0x8E, 0xE4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65 },
}; 

WMA::GUID WMA::GUID::streamProperties = {
  0xB7DC0791, 0xA9B7, 0x11CF, { 0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65 },
};

WMA::GUID WMA::GUID::contentDescription = {
  0x75b22633, 0x668e, 0x11cf, { 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c },
}; 

WMA::GUID WMA::GUID::extendedContentDescription = {
  0xD2D0A440, 0xE307, 0x11D2, { 0x97, 0xF0, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50 },
}; 

WMA::GUID WMA::GUID::audioMedia = {
  0xF8699E40, 0x5B4D, 0x11CF, { 0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B },
}; 

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

WMA::File::File(const char *file, bool readProperties, Properties::ReadStyle propertiesStyle) 
  : TagLib::File(file)
{
  d = new FilePrivate;
  read(readProperties, propertiesStyle);
}

WMA::File::~File()
{
  if(d) {
    if (d->tag) 
      delete d->tag;
    if (d->properties) 
      delete d->properties;
    delete d;   
  }
}

TagLib::Tag *WMA::File::tag() const
{
  return d->tag;
} 

WMA::Tag *WMA::File::WMATag() const
{
  return d->tag;
} 

WMA::Properties *WMA::File::audioProperties() const
{
  return d->properties;
} 

void WMA::File::read(bool readProperties, Properties::ReadStyle /*propertiesStyle*/)
{
  WMA::GUID guid;

  readGUID(guid);
  if(guid != GUID::header) {
    //debug("WMA::File::read() -- Invalid ASF file!");
    return;
  }

  int length = 0;
  int bitrate = 0;
  int sampleRate = 0;
  int channels = 0;
  
  d->tag = new WMA::Tag();
  if(!d->tag)
    return;
  
  d->size = readQWORD();
  d->numObjects = readDWORD(); 
  seek(2, Current);
    
  for(int i = 0; i < (int)d->numObjects; i++) {

    readGUID(guid);
    long objectSize = (long)readQWORD();
      
    if(readProperties && guid == GUID::fileProperties) {

      seek(16+8+8+8, Current);
      length = (int)(readQWORD() / 10000000L);
      seek(8+8+4+4+4+4, Current);
      
    }

    else if(readProperties && guid == GUID::streamProperties) {
      
      long pos = tell();
        
      readGUID(guid);
      if(guid != GUID::audioMedia) {
        //debug("WMA::File::read() -- File contains non-audio streams!");
        return; 
      }
        
      seek(16+8+4+4+2+4+2, Current);
      channels = readWORD();
      sampleRate = readDWORD();
      bitrate = readDWORD() * 8 / 1000;
        
      seek(pos + (long)objectSize - 24);
    }
      
    else if(guid == GUID::extendedContentDescription) {

      d->offset2 = tell() - 16 - 8;
      d->size2 = (long)objectSize;

      int numDescriptors = readWORD();
        
      for(int j = 0; j < numDescriptors; j++) {
        WMA::Attribute *attr = new WMA::Attribute(*this);
        d->tag->setAttribute(attr->name().toCString(false), attr);
      }
      
    }
      
    else if(guid == GUID::contentDescription) {

      d->offset1 = tell() - 16 - 8;
      d->size1 = (long)objectSize;

      int titleLength = readWORD();
      int artistLength = readWORD();
      int copyrightLength = readWORD();
      int commentLength = readWORD();
      int ratingLength = readWORD();

      String value;
      
      readString(titleLength, value);
      d->tag->setTitle(value);
      
      readString(artistLength, value);
      d->tag->setArtist(value);
      
      readString(copyrightLength, value);
      d->tag->setCopyright(value);
      
      readString(commentLength, value);
      d->tag->setComment(value);
      
      readString(ratingLength, value);
      d->tag->setRating(value);
    }
      
    else {
      seek((long)objectSize - 24, Current);
    }
    
  }
  
  if(readProperties) {
    d->properties = new WMA::Properties();
    if(d->properties) 
      d->properties->set(length, bitrate, sampleRate, channels);
  }
  
}

bool WMA::File::save()
{
  if(readOnly()) {
    //debug("WMA::File::save() -- File is read only.");
    return false;
  }

  if(d->offset1 == 0) {
    d->offset1 = 16 + 8 + 4 + 2;
    d->numObjects++;
  }

  if(d->offset2 == 0) {
    d->offset2 = 16 + 8 + 4 + 2;
    d->numObjects++;
  }

  ByteVector chunk1 = renderContentDescription();
  ByteVector chunk2 = renderExtendedContentDescription();

  if(d->offset1 > d->offset2) {
    insert(chunk1, d->offset1, d->size1);
    insert(chunk2, d->offset2, d->size2);
  }
  else {
    insert(chunk2, d->offset2, d->size2);
    insert(chunk1, d->offset1, d->size1);
  }

  insert(ByteVector::fromLongLong(d->size + 
                                  (int)(chunk1.size() - d->size1) + 
                                  (int)(chunk2.size() - d->size2), false) +
         ByteVector::fromUInt(d->numObjects, false), 16, 8 + 4);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

int WMA::File::readBYTE()
{
  ByteVector v = readBlock(1);
  return v[0];
}

int WMA::File::readWORD()
{
  ByteVector v = readBlock(2);
  return v.toShort(false);
}

unsigned int WMA::File::readDWORD()
{
  ByteVector v = readBlock(4);
  return v.toUInt(false);
}

long long WMA::File::readQWORD()
{
  ByteVector v = readBlock(8);
  return v.toLongLong(false);
}

void WMA::File::readGUID(GUID &g)
{
  g.v1 = readDWORD();
  g.v2 = readWORD();
  g.v3 = readWORD();
  for(int i = 0; i < 8; i++)
    g.v4[i] = readBYTE(); 
}

void WMA::File::readString(int len, String &s)
{
  ByteVector v = readBlock(len);
  if(len < 2 || v[len-1] != 0 || v[len-2] != 0)
    v.append(ByteVector::fromShort(0));
  s = String(v, String::UTF16LE);
}

ByteVector WMA::File::renderContentDescription()
{
  String s;    
    
  s = d->tag->title();  
  ByteVector v1 = s.data(String::UTF16LE);
  if(s.size()) {
    v1.append((char)0);
    v1.append((char)0);
  }
  
  s = d->tag->artist();  
  ByteVector v2 = s.data(String::UTF16LE);
  if(s.size()) {
    v2.append((char)0);
    v2.append((char)0);
  }
  
  s = d->tag->copyright();  
  ByteVector v3 = s.data(String::UTF16LE);
  if(s.size()) {
    v3.append((char)0);
    v3.append((char)0);
  } 
  
  s = d->tag->comment();  
  ByteVector v4 = s.data(String::UTF16LE);
  if(s.size()) {
    v4.append((char)0);
    v4.append((char)0);
  }
  
  s = d->tag->rating();  
  ByteVector v5 = s.data(String::UTF16LE);
  if(s.size()) {
    v5.append((char)0);
    v5.append((char)0);
  }

  ByteVector data;

  data.append(ByteVector::fromShort(v1.size(), false));
  data.append(ByteVector::fromShort(v2.size(), false));
  data.append(ByteVector::fromShort(v3.size(), false));
  data.append(ByteVector::fromShort(v4.size(), false));
  data.append(ByteVector::fromShort(v5.size(), false));

  data.append(v1);
  data.append(v2);
  data.append(v3);
  data.append(v4);
  data.append(v5);

  data = ByteVector(reinterpret_cast<const char *>(&GUID::contentDescription), sizeof(GUID))
    + ByteVector::fromLongLong(data.size() + 16 + 8, false)
    + data;

  return data;
}

ByteVector WMA::File::renderExtendedContentDescription()
{
  ByteVector data;

  data.append(ByteVector::fromShort(d->tag->attributeMap().size(), false));
  
  WMA::AttributeMap::ConstIterator it = d->tag->attributeMap().begin();
  for(; it != d->tag->attributeMap().end(); it++) 
    data.append(it->second->render());
  
  data = ByteVector(reinterpret_cast<const char *>(&GUID::extendedContentDescription), sizeof(GUID))
    + ByteVector::fromLongLong(data.size() + 16 + 8, false)
    + data;

  return data;
}


