/****************************************************************************************
 * Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "SimilarArtist.h"

#include <QXmlStreamReader>

SimilarArtist::SimilarArtist() {}

SimilarArtist::SimilarArtist( const QString &name, const int match, const QUrl &url,
                              const QUrl &urlImage, const QString &similarTo )
    : m_name( name )
    , m_match( match )
    , m_url( url )
    , m_urlImage( urlImage )
    , m_similarTo( similarTo )
{

    static bool metaTypeRegistered = false;
    if ( !metaTypeRegistered )
    {
        qRegisterMetaType<SimilarArtist>( "SimilarArtists" );
        metaTypeRegistered = true;
    }
}

SimilarArtist::SimilarArtist( const SimilarArtist &other )
    : QSharedData( other )
    , m_name( other.m_name )
    , m_match( other.m_match )
    , m_url( other.m_url )
    , m_urlImage( other.m_urlImage )
    , m_similarTo( other.m_similarTo )
{
}

QString
SimilarArtist::name() const
{
    return m_name;
}

int
SimilarArtist::match() const
{
    return m_match;
}

QUrl
SimilarArtist::url() const
{
    return m_url;
}

QUrl
SimilarArtist::urlImage() const
{
    return m_urlImage;
}

QString
SimilarArtist::similarTo() const
{
    return m_similarTo;
}

void
SimilarArtist::setSimilarTo( const QString &artist )
{
    m_similarTo = artist;
}

SimilarArtist::List
SimilarArtist::listFromXml( QXmlStreamReader &xml )
{
    SimilarArtist::List saList;
    xml.readNextStartElement(); // lfm
    if( xml.attributes().value(QLatin1String("status")) != QLatin1String("ok") )
        return saList;

    QString similarTo;
    xml.readNextStartElement(); // similarartists
    if( xml.attributes().hasAttribute(QLatin1String("artist")) )
        similarTo = xml.attributes().value(QLatin1String("artist")).toString();

    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String("artist") )
        {
            QString name;
            QUrl artistUrl;
            QUrl imageUrl;
            float match( 0.0 );
            while( xml.readNextStartElement() )
            {
                const QStringView &n = xml.name();
                const QXmlStreamAttributes &a = xml.attributes();
                if( n == QLatin1String("name") )
                    name = xml.readElementText();
                else if( n == QLatin1String("match") )
                    match = xml.readElementText().toFloat() * 100.0;
                else if( n == QLatin1String("url") )
                    artistUrl = QUrl( xml.readElementText() );
                else if( n == QLatin1String("image")
                         && a.hasAttribute(QLatin1String("size"))
                         && a.value(QLatin1String("size")) == QLatin1String("large") )
                    imageUrl = QUrl( xml.readElementText() );
                else
                    xml.skipCurrentElement();
            }
            SimilarArtistPtr artist( new SimilarArtist( name, match, artistUrl, imageUrl, similarTo ) );
            saList.append( artist );
        }
        else
            xml.skipCurrentElement();
    }
    return saList;
}
