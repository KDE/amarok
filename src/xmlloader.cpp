/*
  Copyright (c) 2006 Gábor Lehel <illissius@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/


#include <qapplication.h>

#include "xmlloader.h"
#include "xmlloader_p.h"
#include "xmlloader_p.moc"

MetaBundle::XmlLoader::XmlLoader(): m_aborted( false ), m_target( 0 )
{
    m_reader.setContentHandler( this );
    m_reader.setErrorHandler( this );
}

MetaBundle::XmlLoader::~XmlLoader() {}

bool MetaBundle::XmlLoader::load( QXmlInputSource *source, QObject *target )
{
    m_target = target;
    return m_reader.parse( source, false );
}

void MetaBundle::XmlLoader::abort()
{
    m_aborted = true;
}

QString MetaBundle::XmlLoader::lastError() const
{
    return m_lastError;
}

BundleList MetaBundle::XmlLoader::loadBundles( QXmlInputSource *source, bool *ok ) //static
{
    return SimpleLoader( source, ok ).bundles;
}

void MetaBundle::XmlLoader::loadInThread( QXmlInputSource *source, QObject *target ) //static
{
    ( new ThreadedLoader( source, target ) )->start();
}

void MetaBundle::XmlLoader::newAttribute( const QString &key, const QString &value )
{
    if( key == "url" )
        m_bundle.setUrl( value );
    else if( key == "uniqueid" )
        m_bundle.setUniqueId( value );
    else if( key == "compilation" )
        m_bundle.setCompilation( MetaBundle::CompilationYes );
    else
        m_attributes << QPair<QString, QString>( key, value );
}

void MetaBundle::XmlLoader::newTag( const QString &name, const QString &value )
{
    static int start = 0; //most of the time, the columns should be in order
    for( int i = start; i < NUM_COLUMNS; ++i )
        if( name == exactColumnName( i ) )
        {
            switch( i )
            {
                case Artist:
                case Composer:
                case AlbumArtist:
                case Year:
                case Album:
                case DiscNumber:
                case Track:
                case Bpm:
                case Title:
                case Genre:
                case Comment:
                case Length:
                case Bitrate:
                case Filesize:
                case Type:
                case SampleRate:
                    m_bundle.setExactText( i, value );
                    continue;

                default:
                    continue;
            }
            start = i+1;
            return;
        }
    for( int i = 0; i < start; ++i )
        if( m_currentElement == exactColumnName( i ) )
        {
            switch( i )
            {
                case Artist:
                case Composer:
                case AlbumArtist:
                case Year:
                case Album:
                case DiscNumber:
                case Track:
                case Bpm:
                case Title:
                case Genre:
                case Comment:
                case Length:
                case Bitrate:
                case Filesize:
                case Type:
                case SampleRate:
                    m_bundle.setExactText( i, value );
                    continue;

                default:
                    continue;
            }
            start = i+1;
            return;
        }
}

void MetaBundle::XmlLoader::bundleLoaded()
{
    m_bundle.checkExists();
    emit newBundle( m_bundle, m_attributes );
    if( m_target )
    {
        BundleLoadedEvent e( m_bundle, m_attributes );
        QApplication::sendEvent( m_target, &e );
    }
}

void MetaBundle::XmlLoader::errorEncountered( const QString &, int, int )
{
    emit error( m_lastError );
    if( m_target )
    {
        BundleLoadedEvent e( m_lastError );
        QApplication::sendEvent( m_target, &e );
    }
}

bool MetaBundle::XmlLoader::startElement( const QString &, const QString &localName, const QString &, const QXmlAttributes &atts )
{
    if( localName == "item" )
    {
        m_bundle.clear();
        m_attributes.clear();
        for( int i = 0, n = atts.count(); i < n; ++i )
            newAttribute( atts.localName( i ), atts.value( i ) );

        m_currentElement = QString::null;
    }
    else
        m_currentElement = localName;

    return true;
}

bool MetaBundle::XmlLoader::endElement( const QString &, const QString &localName, const QString & )
{
    if( localName == "item" )
    {
        bundleLoaded();
        m_bundle.clear();
        m_attributes.clear();
        if( m_aborted )
            return false;
    }

    m_currentElement = QString::null;

    return true;
}

bool MetaBundle::XmlLoader::characters( const QString &ch )
{
    if( m_currentElement.isNull() )
        return true;

    newTag( m_currentElement, ch );

    return true;
}

bool MetaBundle::XmlLoader::endDocument()
{
    if( !m_bundle.isEmpty() )
        bundleLoaded();

    return !m_aborted;
}

bool MetaBundle::XmlLoader::fatalError( const QXmlParseException &e )
{
    if( !m_bundle.isEmpty() )
        bundleLoaded();

    m_lastError = QString( "Error loading XML: \"%1\", at line %2, column %3." )
                  .arg( e.message(), QString::number( e.lineNumber() ), QString::number( e.columnNumber() ) );
    errorEncountered( e.message(), e.lineNumber(), e.columnNumber() );

    return false;
}

#include "xmlloader.moc"
