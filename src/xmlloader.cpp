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

MetaBundleXmlLoader::MetaBundleXmlLoader(): m_target( 0 )
{
    m_reader.setContentHandler( this );
    m_reader.setErrorHandler( this );
}

MetaBundleXmlLoader::~MetaBundleXmlLoader() {}

BundleList MetaBundleXmlLoader::loadBundles( QXmlInputSource *source, bool *ok ) //static
{
    return SimpleLoader( source, ok ).bundles;
}

bool MetaBundleXmlLoader::load( QXmlInputSource *source, QObject *target )
{
    m_target = target;
    const bool success = m_reader.parse( source, false );
    if( !success && target )
    {
        BundleLoadedEvent e( true );
        QApplication::sendEvent( target, &e );
    }
    return success;
}

void MetaBundleXmlLoader::loadInThread( QXmlInputSource *source, QObject *target ) //static
{
    ( new ThreadedLoader( source, target ) )->start();
}

void MetaBundleXmlLoader::newAttribute( const QString &key, const QString &value )
{
    if( key == "url" )
        m_bundle.setUrl( value );
    else
        m_attributes << QPair<QString, QString>( key, value );
}

void MetaBundleXmlLoader::newTag( const QString &name, const QString &value )
{
    static int start = 0; //most of the time, the columns should be in order
    for( int i = start; i < MetaBundle::NUM_COLUMNS; ++i )
        if( name == MetaBundle::exactColumnName( i ) )
        {
            switch( i )
            {
                case MetaBundle::Artist:
                case MetaBundle::Composer:
                case MetaBundle::Year:
                case MetaBundle::Album:
                case MetaBundle::DiscNumber:
                case MetaBundle::Track:
                case MetaBundle::Title:
                case MetaBundle::Genre:
                case MetaBundle::Comment:
                case MetaBundle::Length:
                case MetaBundle::Bitrate:
                case MetaBundle::Filesize:
                case MetaBundle::Type:
                case MetaBundle::SampleRate:
                    m_bundle.setExactText( i, value );
                    continue;

                default:
                    continue;
            }
            start = i+1;
            return;
        }
    for( int i = 0; i < start; ++i )
        if( m_currentElement == MetaBundle::exactColumnName( i ) )
        {
            switch( i )
            {
                case MetaBundle::Artist:
                case MetaBundle::Composer:
                case MetaBundle::Year:
                case MetaBundle::Album:
                case MetaBundle::DiscNumber:
                case MetaBundle::Track:
                case MetaBundle::Title:
                case MetaBundle::Genre:
                case MetaBundle::Comment:
                case MetaBundle::Length:
                case MetaBundle::Bitrate:
                case MetaBundle::Filesize:
                case MetaBundle::Type:
                case MetaBundle::SampleRate:
                    m_bundle.setExactText( i, value );
                    continue;

                default:
                    continue;
            }
            start = i+1;
            return;
        }

    return;
}

void MetaBundleXmlLoader::bundleLoaded()
{
    m_bundle.checkExists();
    emit newBundle( m_bundle, m_attributes );
    if( m_target )
    {
        BundleLoadedEvent e( false, m_bundle, m_attributes );
        QApplication::sendEvent( m_target, &e );
    }
}

bool MetaBundleXmlLoader::startElement( const QString &, const QString &localName, const QString &, const QXmlAttributes &atts )
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

bool MetaBundleXmlLoader::endElement( const QString &, const QString &localName, const QString & )
{
    if( localName == "item" )
    {
        bundleLoaded();
        m_bundle.clear();
        m_attributes.clear();
    }

    m_currentElement = QString::null;

    return true;
}

bool MetaBundleXmlLoader::characters( const QString &ch )
{
    if( m_currentElement.isNull() )
        return true;

    newTag( m_currentElement, ch );

    return true;
}

bool MetaBundleXmlLoader::endDocument()
{
    if( !m_bundle.isEmpty() )
        bundleLoaded();

    return true;
}

bool MetaBundleXmlLoader::fatalError( const QXmlParseException& )
{
    if( !m_bundle.isEmpty() )
        bundleLoaded();

    return false;
}

#include "xmlloader.moc"
