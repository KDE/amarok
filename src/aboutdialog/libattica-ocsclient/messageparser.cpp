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

#include "messageparser.h"

#include <QXmlStreamReader>

using namespace AmarokAttica;

MessageParser::MessageParser()
{
}

Message::List MessageParser::parseList( const QString &xmlString )
{
  Message::List messageList;
  
  QXmlStreamReader xml( xmlString );
  
  while ( !xml.atEnd() ) {
    xml.readNext();
    
    if ( xml.isStartElement() && xml.name() == "data" ) {
      while ( !xml.atEnd() ) {
        xml.readNext();

        if ( xml.isEndElement() && xml.name() == "data" ) break;

        if ( xml.isStartElement() && xml.name() == "message" ) {
          Message message;

          while ( !xml.atEnd() ) {
            xml.readNext();

            if ( xml.isStartElement() ) {
              if ( xml.name() == "id" ) {
                message.setId( xml.readElementText() );
              } else if ( xml.name() == "messagefrom" ) {
                message.setFrom( xml.readElementText() );
              } else if ( xml.name() == "messageto" ) {
                message.setTo( xml.readElementText() );
              } else if ( xml.name() == "senddate" ) {
                message.setSent( QDateTime::fromString( xml.readElementText(),
                  Qt::ISODate ) );
              } else if ( xml.name() == "status" ) {
                message.setStatus(
                  Message::Status( xml.readElementText().toInt() ) );
              } else if ( xml.name() == "statustext" ) {
                message.setStatusText( xml.readElementText() );
              } else if ( xml.name() == "subject" ) {
                message.setSubject( xml.readElementText() );
              } else if ( xml.name() == "body" ) {
                message.setBody( xml.readElementText() );
              }
            }

            if ( xml.isEndElement() && xml.name() == "message" ) break;
          }

          messageList.append( message );
        }
      }
    }
  }
  
  return messageList;
}
