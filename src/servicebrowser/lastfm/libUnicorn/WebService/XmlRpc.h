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

#ifndef XML_RPC_H
#define XML_RPC_H

#include <QVariant>
#include <QList>


class XmlRpc
{
    QList<QVariant> m_parameters;
    QString m_method;
    bool m_use_cache;

public:
    XmlRpc() : m_use_cache( false ) {}

    void addParameter( const QVariant &v ) { m_parameters += v; }

    XmlRpc &operator<< ( const QVariant &v ) { m_parameters += v; return *this; }
    
    void setMethod( QString method ) { m_method = method; }
    void setUseCache( bool b ) { m_use_cache = b; }
    
    QString toString() const;
    bool useCache() const { return m_use_cache; }

private:
    // we do this for you in toString()
    static QString escape( QString xml )
    {
        // Need to escape only &, <, > in XML RPC calls
        xml.replace( '&', "&amp;" );
        xml.replace( '<', "&lt;" );
        xml.replace( '>', "&gt;" );
    
        return xml;
    }
    
public:
    static QString unescape( QString xml )
    {
        xml.replace( "&amp;", "&" );
        xml.replace( "&lt;", "<" );
        xml.replace( "&gt;", ">" );
        return xml;
    }

    static QVariant parseValue( const class QDomElement& );
    
    static bool parse( QByteArray xmlResponse, QList<QVariant> &returnValues, QString &error );
    
private:
    enum Type { Integer, Struct, Array, Boolean, String, Unhandled };
    
    static Type typeFromString( QString s )
    {
        if (s == "i4") return Integer;
        if (s == "int") return Integer;
        if (s == "boolean") return Boolean;
        if (s == "struct") return Struct;
        if (s == "array") return Array;
        if (s == "string") return String;
        
        return Unhandled;
    }
};

#endif
