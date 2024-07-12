/****************************************************************************************
 * Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
 * Copyright (c) 2024 Tuomas Nurmi <tuomas@norsumanageri.org>                           *
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

#include "SimilarArtistModel.h"
#include "SimilarArtistsEngine.h"

#include "core/support/Debug.h"

#include <QXmlStreamReader>

SimilarArtistModel::SimilarArtistModel( QObject *parent )
    : QStandardItemModel( parent )
{

}

QString
SimilarArtistModel::SimilarArtistItem::name() const
{
    return m_name;
}

int
SimilarArtistModel::SimilarArtistItem::match() const
{
    return m_match;
}

QUrl
SimilarArtistModel::SimilarArtistItem::url() const
{
    return m_url;
}

QUrl
SimilarArtistModel::SimilarArtistItem::urlImage() const
{
    return m_urlImage;
}

QString
SimilarArtistModel::SimilarArtistItem::bioText() const
{
    return m_bioText;
}

QString
SimilarArtistModel::SimilarArtistItem::listenerCount() const
{
    return m_listenerCount;
}

QString
SimilarArtistModel::SimilarArtistItem::playCount() const
{
    return m_playCount;
}

QString
SimilarArtistModel::SimilarArtistItem::ownPlayCount() const
{
    return m_ownPlayCount;
}

QUrl
SimilarArtistModel::SimilarArtistItem::albumCover() const
{
    return m_albumCover;
}

QVariant
SimilarArtistModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    const QStandardItem *item = itemFromIndex( index );

    if( const auto artist = dynamic_cast<const SimilarArtistItem *>( item ) )
    {
        if (role == NameRole)
            return artist->name();
        if (role == MatchRole)
            return artist->match();
        if (role == LinkRole)
            return artist->url();
        if (role == ImageRole)
            return artist->urlImage();
        if (role == AlbumCoverRole)
            return artist->albumCover();
        if (role == BioRole || role == ListenerCountRole || role == PlayCountRole || role == OwnPlayCountRole)
        {
            if(artist->bioText() == QString())
            {
                if( const auto engine = dynamic_cast<SimilarArtistsEngine *>( parent() ) )
                {
                    engine->artistInfoRequest( artist->name() );
                }
                return QString();
            }
            switch( role )
            {
                case BioRole:
                    return artist->bioText();
                case ListenerCountRole:
                    return artist->listenerCount();
                case PlayCountRole:
                    return artist->playCount();
                case OwnPlayCountRole:
                    return artist->ownPlayCount();
            }
        }
    }
    return itemFromIndex( index )->data( role );
}

void
SimilarArtistModel::clearAll()
{
    m_similarTo = QString();
    clear();
}

void
SimilarArtistModel::setCurrentTarget( const QString &target )
{
    if( target != m_similarTo )
    {
        clearAll();
        m_similarTo = target;
    }
}

void
SimilarArtistModel::setCover( const QString &target, const QUrl &cover )
{
    for( auto i : findItems( target ) )
    {
        if( auto artist = dynamic_cast< SimilarArtistItem *>( i ) )
        {
            artist->m_albumCover = cover;
            artist->emitDataChanged();
        }
    }
}

QHash<int, QByteArray>
SimilarArtistModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[MatchRole] = "match";
    roles[LinkRole] = "link";
    roles[ImageRole] = "image";
    roles[BioRole] = "bio";
    roles[ListenerCountRole] = "listeners";
    roles[PlayCountRole] = "plays";
    roles[OwnPlayCountRole] = "ownplays";
    roles[AlbumCoverRole] = "albumcover";
    return roles;
}

void
SimilarArtistModel::fillArtistInfoFromXml( QXmlStreamReader &xml )
{
    xml.readNextStartElement(); // lfm
    if( xml.attributes().value(QLatin1String("status")) != QLatin1String("ok") )
        return;

    QString name;
    QString listeners;
    QString plays;
    QString ownPlays;
    QString tags;
    QString bio=" ";

    while( xml.name() != QLatin1String("name") )
        xml.readNextStartElement();
    name = xml.readElementText();

    if( name == QString() )
        return; // something's not right, stop

    if( const auto engine = dynamic_cast<SimilarArtistsEngine *>( parent() ) )
    { // we know a similar artist's name, also check if we've got some in our local collection
        engine->searchLocalCollection( name );
    }

    while( xml.readNextStartElement() )
    {
        const QStringView &n = xml.name();
        const QXmlStreamAttributes &a = xml.attributes();
        if( n == QLatin1String("stats") )
        {
            while (xml.readNextStartElement())
            {
                if( xml.name() == QLatin1String("listeners") )
                    listeners = xml.readElementText();
                else if( xml.name() == QLatin1String("playcount") )
                    plays = xml.readElementText();
                else if( xml.name() == QLatin1String("userplaycount") )
                    ownPlays = xml.readElementText();
                else
                    xml.skipCurrentElement();
            }
        }
        else if( n == QLatin1String("tags") )
        {
            while (xml.readNextStartElement())
            {
                //<tag><name>glam rock</name>
                //<url>https://www.last.fm/tag/glam+rock</url>
                //</tag>
                if( xml.name() == QLatin1String("tag") )
                {
                    while (xml.readNextStartElement())
                    {
                        xml.skipCurrentElement();
                    }
                }
            }
        }
        else if( n == QLatin1String("bio") )
        {
            while (xml.readNextStartElement())
            {
                //if( xml.name() == QLatin1String("summary") )
                if( xml.name() == QLatin1String("content") )
                {
                    bio = xml.readElementText().replace( QStringLiteral("\n"), QStringLiteral("<br>") );
                    if(bio == QString())
                        bio = " ";
                }
                else
                    xml.skipCurrentElement();
            }
        }
        else
            xml.skipCurrentElement();
    }

    for( auto i : findItems( name ) )
    {
        if( auto artist = dynamic_cast< SimilarArtistItem *>( i ) )
        {
            artist->m_bioText = bio.isNull() ? " " : bio;
            artist->m_listenerCount = listeners;
            artist->m_playCount = plays;
            artist->m_ownPlayCount = ownPlays;
            artist->emitDataChanged();
        }
    }
}

void
SimilarArtistModel::fillFromXml( QXmlStreamReader &xml )
{
    clear();
    xml.readNextStartElement(); // lfm
    if( xml.attributes().value(QLatin1String("status")) != QLatin1String("ok") )
        return;

    xml.readNextStartElement(); // similarartists
    if( xml.attributes().hasAttribute(QLatin1String("artist")) )
        m_similarTo = xml.attributes().value(QLatin1String("artist")).toString();

    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String("artist") )
        {
            SimilarArtistItem *similarArtistItem = new SimilarArtistItem();
            while( xml.readNextStartElement() )
            {
                const QStringView &n = xml.name();
                const QXmlStreamAttributes &a = xml.attributes();
                if( n == QLatin1String("name") )
                    similarArtistItem->m_name = xml.readElementText();
                else if( n == QLatin1String("match") )
                    similarArtistItem->m_match = xml.readElementText().toFloat() * 100.0;
                else if( n == QLatin1String("url") )
                    similarArtistItem->m_url = QUrl( xml.readElementText() );
                // unfortunately disabled in last.fm api since 2019 or so, now returns only generic placeholder image
                else if( n == QLatin1String("image")
                         && a.hasAttribute(QLatin1String("size"))
                         && a.value(QLatin1String("size")) == QLatin1String("large") )
                    similarArtistItem->m_urlImage = QUrl( xml.readElementText() );
                else
                    xml.skipCurrentElement();
            }
            similarArtistItem->setText( similarArtistItem->m_name );
            appendRow( similarArtistItem );
        }
        else
            xml.skipCurrentElement();
    }
    debug() << "Found" << rowCount() << "similar artists to" << m_similarTo;
}
