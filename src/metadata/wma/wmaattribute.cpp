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

#include <wmaattribute.h>
#include <wmafile.h>

using namespace TagLib;

class WMA::Attribute::AttributePrivate
{
public:
  AttributeTypes type;    
  String name;
  String value_string;
  ByteVector value_bytes;
  union {
    int value_int;
    long long value_longlong;
  };
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

WMA::Attribute::Attribute()
{
  d = new AttributePrivate;
  d->name = String::null;
  d->type = UnicodeType;
}

WMA::Attribute::Attribute(WMA::File &file)
{
  d = new AttributePrivate;
  parse(file);
}

WMA::Attribute::Attribute(const String &name, const String &value)
{
  d = new AttributePrivate;
  d->name = name;
  d->type = UnicodeType;
  d->value_string = value;
}

WMA::Attribute::Attribute(const String &name, const ByteVector &value)
{
  d = new AttributePrivate;
  d->name = name;
  d->type = BytesType;
  d->value_bytes = value;
}

WMA::Attribute::Attribute(const String &name, unsigned int value)
{
  d = new AttributePrivate;
  d->name = name;
  d->type = DWordType;
  d->value_int = value;
}

WMA::Attribute::Attribute(const String &name, unsigned long long value)
{
  d = new AttributePrivate;
  d->name = name;
  d->type = QWordType;
  d->value_longlong = value;
}

WMA::Attribute::Attribute(const String &name, unsigned short value)
{
  d = new AttributePrivate;
  d->name = name;
  d->type = WordType;
  d->value_int = value;
}

WMA::Attribute::Attribute(const String &name, bool value)
{
  d = new AttributePrivate;
  d->name = name;
  d->type = BoolType;
  d->value_int = value;
}

WMA::Attribute::~Attribute()
{
  if(d) 
    delete d;
}

String WMA::Attribute::name() const
{
  return d->name;
}

WMA::Attribute::AttributeTypes WMA::Attribute::type() const
{
  return d->type;
}

String WMA::Attribute::toString() const
{
  return d->value_string;
}

ByteVector WMA::Attribute::toByteVector() const
{
  return d->value_bytes;
}

int WMA::Attribute::toInt() const
{
  if (d->type == UnicodeType)
    return d->value_string.toInt();
  else
    return d->value_int;
}

long long WMA::Attribute::toLongLong() const
{
  return d->value_longlong;
}

bool WMA::Attribute::parse(WMA::File &f)
{
  int size = f.readWORD();
  f.readString(size, d->name);
  
  d->type = (WMA::Attribute::AttributeTypes)f.readWORD();
  size = f.readWORD();

  switch(d->type) {
  case WordType:
    d->value_int = f.readWORD();
    break;
    
  case BoolType:
  case DWordType:
    d->value_int = f.readDWORD();
    break;
    
  case QWordType:  
    d->value_longlong = f.readQWORD();
    break;
    
  case UnicodeType:  
    f.readString(size, d->value_string);
    break;
    
  case BytesType:  
    d->value_bytes = f.readBlock(size);
    break;
    
  default:
    //debug("WMA::Attribute::parse() -- Unsupported attribute type!");
    return false; 
  }
  
  return true;
}

ByteVector WMA::Attribute::render() const
{
  ByteVector data;

  ByteVector v = d->name.data(String::UTF16LE);
  data.append(ByteVector::fromShort(v.size() + 2, false));
  data.append(v + ByteVector::fromShort(0, false));
  
  data.append(ByteVector::fromShort((int)d->type, false));
    
  switch (d->type) {
  case WordType:
    data.append(ByteVector::fromShort(2, false));
    data.append(ByteVector::fromShort(d->value_int, false));
    break;
      
  case BoolType:
  case DWordType:
    data.append(ByteVector::fromShort(4, false));
    data.append(ByteVector::fromUInt(d->value_int, false));
    break;
      
  case QWordType:
    data.append(ByteVector::fromShort(8, false));
    data.append(ByteVector::fromLongLong(d->value_longlong, false));
    break;
      
  case UnicodeType:
    v = d->value_string.data(String::UTF16LE);
    data.append(ByteVector::fromShort(v.size() + 2, false));
    data.append(v + ByteVector::fromShort(0, false));
    break;
    
  case BytesType:
    data.append(ByteVector::fromShort(d->value_bytes.size(), false));
    data.append(d->value_bytes);
    break;
    
  default:  
    //debug("WMA::Attribute::render() -- Unsupported attribute type!");
    return ByteVector::null; 
  }
  
  return data;
}


