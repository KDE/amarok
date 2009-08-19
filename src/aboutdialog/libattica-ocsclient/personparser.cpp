/*
    This file is part of KDE.

    Copyright (c) 2008 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "personparser.h"

#include <QXmlStreamReader>

using namespace Attica;

PersonParser::PersonParser()
{
}

Person::List PersonParser::parseList( const QString &xmlString )
{
  Person::List personList;
  
  QXmlStreamReader xml( xmlString );
  
  while ( !xml.atEnd() ) {
    xml.readNext();
    
    if ( xml.isStartElement() &&
         ( xml.name() == "person" || xml.name() == "user" ) ) {
      Person person = parsePerson( xml );
      personList.append( person );
    }
  }
  
  return personList;
}

Person PersonParser::parse( const QString &xmlString )
{
  Person person;

  QXmlStreamReader xml( xmlString );
  
  while ( !xml.atEnd() ) {
    xml.readNext();
    
    if ( xml.isStartElement() && xml.name() == "person" ) {
      person = parsePerson( xml );
    }
  }

  return person;
}

Person PersonParser::parsePerson( QXmlStreamReader &xml )
{
  Person person;
  
  while ( !xml.atEnd() ) {
    xml.readNext();

    if ( xml.isStartElement() ) {
      if ( xml.name() == "personid" ) {
        person.setId( xml.readElementText() );
      } else if ( xml.name() == "firstname" ) {
        person.setFirstName( xml.readElementText() );
      } else if ( xml.name() == "lastname" ) {
        person.setLastName( xml.readElementText() );
      } else if ( xml.name() == "homepage" ) {
        person.setHomepage( xml.readElementText() );
      } else if ( xml.name() == "avatarpic" ) {
        person.setAvatarUrl( xml.readElementText() );
      } else if ( xml.name() == "birthday" ) {
        person.setBirthday( QDate::fromString( xml.readElementText(),
          Qt::ISODate ) );
      } else if ( xml.name() == "city" ) {
        person.setCity( xml.readElementText() );
      } else if ( xml.name() == "country" ) {
        person.setCountry( xml.readElementText() );
      } else if ( xml.name() == "latitude" ) {
        person.setLatitude( xml.readElementText().toFloat() );
      } else if ( xml.name() == "longitude" ) {
        person.setLongitude( xml.readElementText().toFloat() );
      } else {
        person.addExtendedAttribute( xml.name().toString(),
          xml.readElementText() );
      }
    }

    if ( xml.isEndElement() &&
         ( xml.name() == "person" || xml.name() == "user" ) ) break;
  }

  return person;
}
