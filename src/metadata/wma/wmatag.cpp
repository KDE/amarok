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

#include <wmatag.h>

using namespace TagLib;

class WMA::Tag::TagPrivate
{
public:
  String title;
  String artist;
  String copyright;
  String comment;
  String rating;
  AttributeMap attributeMap;
};

WMA::Tag::Tag()
: TagLib::Tag()
{
  d = new TagPrivate;
}

WMA::Tag::~Tag()
{
  if(d)
    delete d;  
}

String
WMA::Tag::title() const
{
  return d->title;  
}

String
WMA::Tag::artist() const
{
  return d->artist;  
}

String
WMA::Tag::album() const
{
  if(d->attributeMap.contains("WM/AlbumTitle"))
    return d->attributeMap["WM/AlbumTitle"]->toString();
  return String::null;
}

String
WMA::Tag::copyright() const
{
  return d->copyright;
}

String
WMA::Tag::comment() const
{
  return d->comment;
}

String
WMA::Tag::rating() const
{
  return d->rating;
}

unsigned
WMA::Tag::year() const
{
  if(d->attributeMap.contains("WM/Year"))
    return d->attributeMap["WM/Year"]->toInt();
  return 0;
}

unsigned
WMA::Tag::track() const
{
  if(d->attributeMap.contains("WM/TrackNumber"))
    return d->attributeMap["WM/TrackNumber"]->toInt();
  if(d->attributeMap.contains("WM/Track"))
    return d->attributeMap["WM/Track"]->toInt();
  return 0;
}

String
WMA::Tag::genre() const
{
  if(d->attributeMap.contains("WM/Genre"))
    return d->attributeMap["WM/Genre"]->toString();
  return String::null;
}

void 
WMA::Tag::setTitle(const String &value)
{
  d->title = value;
}

void 
WMA::Tag::setArtist(const String &value)
{
  d->artist = value;  
}

void 
WMA::Tag::setCopyright(const String &value)
{
  d->copyright = value;  
}

void 
WMA::Tag::setComment(const String &value)
{
  d->comment = value;
}

void 
WMA::Tag::setRating(const String &value)
{
  d->rating = value;
}

void 
WMA::Tag::setAlbum(const String &value)
{
  setAttribute("WM/AlbumTitle", value);
}

void 
WMA::Tag::setGenre(const String &value)
{
  setAttribute("WM/Genre", value);
}

void 
WMA::Tag::setYear(unsigned value)
{
  setAttribute("WM/Year", String::number(value));
}

void 
WMA::Tag::setTrack(unsigned value)
{
  setAttribute("WM/TrackNumber", String::number(value));
}

const WMA::AttributeMap &
WMA::Tag::attributeMap() const
{
  return d->attributeMap;
}

void
WMA::Tag::setAttribute(const ByteVector &name, const String &value)
{
  WMA::Attribute *attribute = new WMA::Attribute(name, value);
  d->attributeMap[name] = attribute;
}

void
WMA::Tag::setAttribute(const ByteVector &name, Attribute *attribute)
{
  d->attributeMap[name] = attribute;
}

bool WMA::Tag::isEmpty() const {
  return TagLib::Tag::isEmpty() &&
         copyright().isEmpty() &&
         rating().isEmpty() &&
         d->attributeMap.isEmpty();
}

void WMA::Tag::duplicate(const Tag *source, Tag *target, bool overwrite) {
  TagLib::Tag::duplicate(source, target, overwrite);
  if(overwrite) {
    target->setCopyright(source->copyright());
    target->setRating(source->rating());
  }
  else {
    if (target->copyright().isEmpty())                                            
      target->setCopyright(source->copyright());
    if (target->rating().isEmpty())                                            
      target->setRating(source->rating());
  }
}

