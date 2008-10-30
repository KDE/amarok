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

#ifndef CORE_DOM_ELEMENT_H
#define CORE_DOM_ELEMENT_H

#include <lastfm/DllExportMacro.h>
#include <lastfm/core/CoreException.h>
#include <QDebug>
#include <QDomElement>
#include <QList>
#include <QStringList>


/** @author <max@last.fm>
  * @brief facade pattern for QDomElement, throwing exceptions in situations that we must handle
  *
  * QDomElement dome;
  * CoreDomElement( dome )["album"]["image size=small"].text();
  * foreach (CoreDomElement e, CoreDomElement( dome )["album"].children( "image" ))
  *     qDebug() << e.text();
  */
class LASTFM_CORE_DLLEXPORT CoreDomElement
{
    QDomElement e;

	friend QDebug operator<<( QDebug, const CoreDomElement& );
    
    CoreDomElement()
    {}
    
public:
    class Exception : public CoreException
    {
        friend class CoreDomElement;

        Exception( QString s ) : CoreException( s )
        {}

    public:
        static Exception nullNode() { return Exception( "Expected node absent." ); }
        static Exception emptyTextNode( QString name ) { return Exception( "Unexpected empty text node: " + name ); }
    };


    CoreDomElement( const QDomElement& x ) : e( x )
    {
        if (e.isNull()) throw Exception::nullNode();
    }

    /** returns a null element unless the node @p name exists */
    CoreDomElement optional( const QString& name ) const
    {
        try
        {
            return this->operator[]( name );
        }
        catch (Exception&)
        {
            return CoreDomElement();
        }
    }
    
    /** Selects a child element, you can specify attributes like so:
      *
      * e["element"]["element attribute=value"].text();
      */
    CoreDomElement operator[]( const QString& name ) const;
    
    /** use in all cases where empty would be an error, it throws if empty,
      * ignores optional() since you are explicitly asking for a throw! */
    QString nonEmptyText() const;

    QString text() const { return e.text(); }
    QList<CoreDomElement> children( const QString& named ) const;
};


inline QDebug operator<<( QDebug debug, const CoreDomElement& e )
{
	QString s;
	QTextStream t( &s, QIODevice::WriteOnly );
	e.e.save( t, 2 );
	return debug << s;
}

#endif
