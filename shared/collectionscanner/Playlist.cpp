/***************************************************************************
 *   Copyright (C) 2010 Ralf Engels <ralf-engels@gmx.de>                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "Playlist.h"
#include "utils.h"

#include <QDir>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

CollectionScanner::Playlist::Playlist( const QString &path )
{
    m_path = path;
    m_rpath = QDir::current().relativeFilePath( path );
}

CollectionScanner::Playlist::Playlist( QXmlStreamReader *reader )
{
   while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            if( reader->name() == QLatin1String("path") )
                m_path = reader->readElementText();
            else if( reader->name() == QLatin1String("rpath") )
                m_rpath = reader->readElementText();
            else
                reader->readElementText(); // just read over the element
        }

        else if( reader->isEndElement() )
        {
            break;
        }
    }
}

QString
CollectionScanner::Playlist::path() const
{
    return m_path;
}

QString
CollectionScanner::Playlist::rpath() const
{
    return m_rpath;
}

void
CollectionScanner::Playlist::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( QStringLiteral("path"), escapeXml10(m_path) );
    writer->writeTextElement( QStringLiteral("rpath"), escapeXml10(m_rpath) );
}
