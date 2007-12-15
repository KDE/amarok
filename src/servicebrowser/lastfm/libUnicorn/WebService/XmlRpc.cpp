/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "XmlRpc.h"
#include <QDomElement>
#include <QDomNode>
#include <QStringList>


QString
XmlRpc::toString() const
{
    Q_ASSERT( !m_method.isEmpty() );

    QString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                      "<methodCall>"
                          "<methodName>" + m_method + "</methodName>" +
                          "<params>";

    foreach (QVariant const p, m_parameters)
    {
        xml += "<param><value>";

        switch (p.type())
        {
            case QVariant::String:
                xml += "<string>" + escape( p.toString() ) + "</string>";
                break;

            case QVariant::StringList:
                xml += "<array><data>";
                foreach (QString s, p.toStringList())
                    xml += "<value><string>" + escape( s ) + "</string></value>";
                xml += "</data></array>";
                break;

            default:
                // Support for this type not yet implemented
                Q_ASSERT( false );
        }

        xml += "</value></param>";
    }

    xml += "</params></methodCall>";

    return xml;
}

bool
XmlRpc::parse( QByteArray xmlResponse, QList<QVariant>& returnValues, QString& error )
{
    QDomDocument xml;
    if ( !xml.setContent( xmlResponse ) )
    {
        error = "Couldn't parse XML response: " + xmlResponse;
        return false;
    }

    QDomNodeList fault = xml.elementsByTagName( "fault" );
    if ( !fault.isEmpty() )
    {
        // TODO: Parse XML RPC fault struct here
        error = "Fault present in XML response: " + xmlResponse;
        return false;
    }

    QDomNodeList params = xml.elementsByTagName( "param" );
    if ( params.isEmpty() )
    {
        error = "No params present in XML response: " + xmlResponse;
        return false;
    }

    for ( int i = 0; i < params.count(); ++i )
    {
        QDomNode node = params.at( i );

        // Skip past the pointless "<value>" tag
        QDomElement param = node.firstChildElement().firstChildElement();
        if ( param.isNull() )
        {
            error = "Malformed XML: " + xmlResponse;
            return false;
        }
        else
            returnValues << parseValue( param );
    }

    return true;
}

QVariant
XmlRpc::parseValue( const QDomElement& e )
{
    QString const tag = e.tagName();
    
    switch (typeFromString( tag ))
    {
    case String:
        return unescape( e.text() );

    case Integer:
        return e.text().toInt();
    
    case Boolean:
        return (bool) e.text().toInt();

    case Struct: {
        QMap<QString, QVariant> map;
        QDomNodeList nodes = e.elementsByTagName( "member" );

        for (int j = 0; j < nodes.count(); ++j) 
        {
            QDomNode const n = nodes.at( j );
            QDomElement name = n.firstChildElement( "name" );
            QDomElement value = n.firstChildElement( "value" );

            QVariant v = parseValue( value.firstChildElement() );

            map.insert( name.text(), v );
        }

        return map;
    }

    case Array: {
        QList<QVariant> array;

        QDomNodeList nodes = e.firstChild().childNodes();

        for (int j = 0; j < nodes.count(); ++j) {
            QDomNode node = nodes.at( j );
            if (node.isElement() && node.toElement().tagName() == "value")
                array += parseValue( node.firstChildElement() );
        }

        return array;
    }
        
    default:
        // Support for this type not yet implemented
        Q_ASSERT( false );
        return QVariant();
    }
}
