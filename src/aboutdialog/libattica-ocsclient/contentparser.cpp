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

#include "contentparser.h"

#include <QXmlStreamReader>

using namespace Attica;

ContentParser::ContentParser()
{
}

Content::List ContentParser::parseList( const QString &xmlString )
{
  Content::List contentList;
  
  QXmlStreamReader xml( xmlString );
  
  while ( !xml.atEnd() ) {
    xml.readNext();
    
    if ( xml.isStartElement() && xml.name() == "content" ) {
      Content content = parseContent( xml );
      contentList.append( content );
    }
  }
  
  return contentList;
}

Content ContentParser::parse( const QString &xmlString )
{
  Content content;

  QXmlStreamReader xml( xmlString );
  
  while ( !xml.atEnd() ) {
    xml.readNext();
    
    if ( xml.isStartElement() && xml.name() == "content" ) {
      content = parseContent( xml );
    }
  }

  return content;
}

Content ContentParser::parseContent( QXmlStreamReader &xml )
{
  Content content;
  
  while ( !xml.atEnd() ) {
    xml.readNext();

    if ( xml.isStartElement() ) {
      if ( xml.name() == "id" ) {
        content.setId( xml.readElementText() );
      } else if ( xml.name() == "name" ) {
        content.setName( xml.readElementText() );
      } else if ( xml.name() == "score" ) {
        content.setRating( xml.readElementText().toInt() );
      } else if ( xml.name() == "downloads" ) {
        content.setDownloads( xml.readElementText().toInt() );
      } else if ( xml.name() == "created" ) {
        content.setCreated( QDateTime::fromString( xml.readElementText(),
          Qt::ISODate ) );
      } else if ( xml.name() == "updated" ) {
        content.setUpdated( QDateTime::fromString( xml.readElementText(),
          Qt::ISODate ) );
      } else {
        content.addExtendedAttribute( xml.name().toString(),
          xml.readElementText() );
      }
    }

    if ( xml.isEndElement() && xml.name() == "content" ) break;
  }

  return content;
}
