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

#include "BatchFile.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

CollectionScanner::BatchFile::BatchFile()
{
}

CollectionScanner::BatchFile::BatchFile( const QString &path )
{
    QFile batchFile( path );

    if( !batchFile.exists() ||
        !batchFile.open( QIODevice::ReadOnly ) )
        return;

    QXmlStreamReader reader( &batchFile );

    // very simple parser
    // improve scanner with skipCurrentElement as soon as Amarok requires Qt 4.6
    while (!reader.atEnd()) {
        reader.readNext();

        if( reader.isStartElement() )
        {
            QStringRef name = reader.name();

            if( name == "directory" )
            {
                QString dirPath = reader.readElementText();
                m_directories.append( dirPath );
            }
            else if( name == "mtime" )
            {
                uint mTime = 0;
                if( reader.attributes().hasAttribute( "time" ) )
                    mTime = reader.attributes().value( "time" ).toString().toUInt();

                QString dirPath = reader.readElementText();

                m_timeDefinitions.append( TimeDefinition( dirPath, mTime ) );
            }
        }
    }

}

const QStringList&
CollectionScanner::BatchFile::directories() const
{
    return m_directories;
}

void
CollectionScanner::BatchFile::setDirectories( const QStringList &value )
{
    m_directories = value;
}

const QList<CollectionScanner::BatchFile::TimeDefinition>&
CollectionScanner::BatchFile::timeDefinitions() const
{
    return m_timeDefinitions;
}

void
CollectionScanner::BatchFile::setTimeDefinitions( const QList<TimeDefinition> &value )
{
    m_timeDefinitions = value;
}

bool
CollectionScanner::BatchFile::write( const QString &path )
{
    QFile batchFile( path );
    if( !batchFile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
        return false;

    QXmlStreamWriter writer( &batchFile );
    writer.setAutoFormatting( true );

    writer.writeStartDocument();
    writer.writeStartElement( "batch" );

    foreach( const QString &dir, m_directories )
    {
        writer.writeStartElement( "directory" );
        writer.writeCharacters( dir );
        writer.writeEndElement();
    }
    foreach( const TimeDefinition &pair, m_timeDefinitions )
    {
        writer.writeStartElement( "mtime" );
        // note: some file systems return an mtime of 0
        writer.writeAttribute( "time", QString::number( pair.second ) );

        writer.writeCharacters( pair.first );
        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeEndDocument();

    return true;
}

