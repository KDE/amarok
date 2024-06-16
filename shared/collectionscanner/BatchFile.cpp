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

#include "Version.h"  // for AMAROK_VERSION

#include <QFile>
#include <QDateTime>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <QDebug>
CollectionScanner::BatchFile::BatchFile()
{
}

CollectionScanner::BatchFile::BatchFile( const QString &batchPath )
{
    QFile batchFile( batchPath );

    if( !batchFile.exists() ||
        !batchFile.open( QIODevice::ReadOnly ) )
        return;

    QString path;
    uint mtime = 0;
    bool haveMtime = false;
    QXmlStreamReader reader( &batchFile );

    // very simple parser
    while (!reader.atEnd()) {
        reader.readNext();

        if( reader.isStartElement() )
        {
            QStringRef name = reader.name();

            if( name == QLatin1String("scanner") )
            {
                ; // just recurse into the element
            }
            else if( name == QLatin1String("directory") )
            {
                path.clear();
                mtime = 0;
                haveMtime = false;
            }
            else if( name == QLatin1String("path") )
                path = reader.readElementText(QXmlStreamReader::SkipChildElements);
            else if( name == QLatin1String("mtime") )
            {
                mtime = reader.readElementText(QXmlStreamReader::SkipChildElements).toUInt();
                haveMtime = true;
            }
            else
            {
                reader.skipCurrentElement();
            }
        }
        else if( reader.isEndElement() )
        {
            QStringRef name = reader.name();
            if( name == QLatin1String("directory") )
            {
                if( !path.isEmpty() )
                {
                    if( haveMtime )
                        m_timeDefinitions.append( TimeDefinition( path, mtime ) );
                    else
                        m_directories.append( path );
                }
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
CollectionScanner::BatchFile::write( const QString &batchPath )
{
    QFile batchFile( batchPath );
    if( !batchFile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
        return false;

    QXmlStreamWriter writer( &batchFile );
    writer.setAutoFormatting( true );

    writer.writeStartDocument();
    writer.writeStartElement( QStringLiteral("scanner") );
    writer.writeComment(QStringLiteral("Batch file for amarokcollectionscanner ") + QLatin1String(AMAROK_VERSION) + QStringLiteral(" created on ") + QDateTime::currentDateTime().toString());

    for( const auto &dir : m_directories )
    {
        writer.writeStartElement( QStringLiteral("directory") );
        writer.writeTextElement( QStringLiteral("path"), dir );
        writer.writeEndElement();
    }

    for( const auto &pair : m_timeDefinitions )
    {
        QString path( pair.first );
        uint mtime = pair.second;

        writer.writeStartElement( QStringLiteral("directory") );
        writer.writeTextElement( QStringLiteral("path"), path );
        // note: some file systems return an mtime of 0
        writer.writeTextElement( QStringLiteral("mtime"), QString::number( mtime ) );
        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeEndDocument();

    return true;
}
