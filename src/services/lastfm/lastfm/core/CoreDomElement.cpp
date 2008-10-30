/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
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

#include "CoreDomElement.h"


CoreDomElement
CoreDomElement::operator[]( const QString& name ) const
{
    QStringList parts = name.split( ' ' );
    if (parts.size() >= 2)
    {
        QString tagName = parts[0];
        parts = parts[1].split( '=' );
        QString attributeName = parts.value( 0 );
        QString attributeValue = parts.value( 1 );

        foreach (CoreDomElement e, children( tagName ))
            if (e.e.attribute( attributeName ) == attributeValue)
                return e;
    }
    return CoreDomElement( e.firstChildElement( name ) );
}


QString
CoreDomElement::nonEmptyText() const 
{
    QString const s = e.text();
    if (s.isEmpty())
        throw Exception::emptyTextNode( e.tagName() );
    return s;
}


QList<CoreDomElement>
CoreDomElement::children( const QString& named ) const
{
    QList<CoreDomElement> elements;
    QDomNodeList nodes = e.elementsByTagName( named );
    for (int x = 0; x < nodes.count(); ++x)
        elements += CoreDomElement( nodes.at( x ).toElement() );
    return elements;
}
