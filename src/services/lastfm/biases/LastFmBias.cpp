/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#define DEBUG_PREFIX "LastFmBias"

#include "LastFmBias.h"

#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <KLocalizedString>
#include <QStandardPaths>

#include <QDomDocument>
#include <QDomNode>
#include <QFile>
#include <QLabel>
#include <QPixmap>
#include <QRadioButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <Artist.h>
#include <ws.h>

QString
Dynamic::LastFmBiasFactory::i18nName() const
{ return i18nc("Name of the \"Last.fm\" similar bias", "Last.fm similar"); }

QString
Dynamic::LastFmBiasFactory::name() const
{ return Dynamic::LastFmBias::sName(); }

QString
Dynamic::LastFmBiasFactory::i18nDescription() const
{ return i18nc("Description of the \"Last.fm\" bias",
                   "The \"Last.fm\" similar bias looks up tracks on Last.fm and only adds similar tracks."); }

Dynamic::BiasPtr
Dynamic::LastFmBiasFactory::createBias()
{ return Dynamic::BiasPtr( new Dynamic::LastFmBias() ); }



// ----- LastFmBias --------

Dynamic::LastFmBias::LastFmBias()
    : SimpleMatchBias()
    , m_match( SimilarArtist )
{
    loadDataFromFile();
}

Dynamic::LastFmBias::~LastFmBias()
{
    // TODO: kill all running queries
}

void
Dynamic::LastFmBias::fromXml( QXmlStreamReader *reader )
{
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringView name = reader->name();
            if( name == QStringLiteral("match") )
                m_match = matchForName( reader->readElementText(QXmlStreamReader::SkipChildElements) );
            else
            {
                debug()<<"Unexpected xml start element"<<reader->name()<<"in input";
                reader->skipCurrentElement();
            }
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }
}

void
Dynamic::LastFmBias::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( "match", nameForMatch( m_match ) );
}

QString
Dynamic::LastFmBias::sName()
{
    return "lastfm_similarartists";
}

QString
Dynamic::LastFmBias::name() const
{
    return Dynamic::LastFmBias::sName();
}

QString
Dynamic::LastFmBias::toString() const
{
    switch( m_match )
    {
    case SimilarTrack:
        return i18nc("Last.fm bias representation",
                     "Similar to the previous track (as reported by Last.fm)");
    case SimilarArtist:
        return i18nc("Last.fm bias representation",
                     "Similar to the previous artist (as reported by Last.fm)");
    }
    return QString();
}


QWidget*
Dynamic::LastFmBias::widget( QWidget* parent )
{
    QWidget *widget = new QWidget( parent );
    QVBoxLayout *layout = new QVBoxLayout( widget );

    QLabel *imageLabel = new QLabel();
    imageLabel->setPixmap( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/lastfm.png" ) ) );
    QLabel *label = new QLabel( i18n( "<a href=\"http://www.last.fm/\">Last.fm</a> thinks the track is similar to" ) );

    QRadioButton *rb1 = new QRadioButton( i18n( "the previous track's artist" ) );
    QRadioButton *rb2 = new QRadioButton( i18n( "the previous track" ) );

    rb1->setChecked( m_match == SimilarArtist );
    rb2->setChecked( m_match == SimilarTrack );

    connect( rb1, &QRadioButton::toggled,
             this, &LastFmBias::setMatchTypeArtist );

    layout->addWidget( imageLabel );
    layout->addWidget( label );
    layout->addWidget( rb1 );
    layout->addWidget( rb2 );

    return widget;
}

Dynamic::TrackSet
Dynamic::LastFmBias::matchingTracks( const Meta::TrackList& playlist,
                                     int contextCount, int finalCount,
                                     const Dynamic::TrackCollectionPtr& universe ) const
{
    Q_UNUSED( contextCount );
    Q_UNUSED( finalCount );

    if( playlist.isEmpty() )
        return Dynamic::TrackSet( universe, true );

    // determine the last track and artist
    Meta::TrackPtr lastTrack = playlist.last();
    Meta::ArtistPtr lastArtist = lastTrack->artist();

    m_currentTrack = lastTrack->name();
    m_currentArtist = lastArtist ? lastArtist->name() : QString();

    {
        QMutexLocker locker( &m_mutex );

        if( m_match == SimilarArtist )
        {
            if( m_currentArtist.isEmpty() )
                return Dynamic::TrackSet( universe, true );
            if( m_tracksMap.contains( m_currentArtist ) )
                return m_tracksMap.value( m_currentArtist );
        }
        else if( m_match == SimilarTrack )
        {
            if( m_currentTrack.isEmpty() )
                return Dynamic::TrackSet( universe, true );
            QString key = m_currentTrack + '|' + m_currentArtist;
            if( m_tracksMap.contains( key ) )
                return m_tracksMap.value( key );
        }
    }

    m_tracks = Dynamic::TrackSet( universe, false );
    QTimer::singleShot(0,
                       const_cast<LastFmBias*>(this),
                       &LastFmBias::newQuery); // create the new query from my parent thread

    return Dynamic::TrackSet();
}


bool
Dynamic::LastFmBias::trackMatches( int position,
                                   const Meta::TrackList& playlist,
                                   int contextCount ) const
{
    Q_UNUSED( contextCount );

    if( position <= 0 || position >= playlist.count())
        return false;

    // determine the last track and artist
    Meta::TrackPtr lastTrack = playlist[position-1];
    Meta::ArtistPtr lastArtist = lastTrack->artist();
    QString lastTrackName = lastTrack->name();
    QString lastArtistName = lastArtist ? lastArtist->name() : QString();

    Meta::TrackPtr currentTrack = playlist[position];
    Meta::ArtistPtr currentArtist = currentTrack->artist();
    QString currentTrackName = currentTrack->name();
    QString currentArtistName = currentArtist ? currentArtist->name() : QString();

    {
        QMutexLocker locker( &m_mutex );

        if( m_match == SimilarArtist )
        {
            if( lastArtistName.isEmpty() )
                return true;
            if( currentArtistName.isEmpty() )
                return false;
            if( lastArtistName == currentArtistName )
                return true;
            if( m_similarArtistMap.contains( lastArtistName ) )
                return m_similarArtistMap.value( lastArtistName ).contains( currentArtistName );
        }
        else if( m_match == SimilarTrack )
        {
            if( lastTrackName.isEmpty() )
                return true;
            if( currentTrackName.isEmpty() )
                return false;
            if( lastTrackName == currentTrackName )
                return true;
            TitleArtistPair lastKey( lastTrackName, lastArtistName );
            TitleArtistPair currentKey( currentTrackName, currentArtistName );
            if( m_similarTrackMap.contains( lastKey ) )
                return m_similarTrackMap.value( lastKey ).contains( currentKey );
        }
    }

    debug() << "didn't have a cached suggestions for track:" << lastTrackName;
    return false;
}


void
Dynamic::LastFmBias::invalidate()
{
    SimpleMatchBias::invalidate();
    m_tracksMap.clear();
}

void
Dynamic::LastFmBias::newQuery()
{
    DEBUG_BLOCK;

    debug() << "similarArtists:"<<m_similarArtistMap.count() << "similarTracks:"<<m_similarTrackMap.count();
    // - get the similar things
    QStringList similarArtists;
    QList<TitleArtistPair> similarTracks;
    {
        QMutexLocker locker( &m_mutex );
        if( m_match == SimilarArtist )
        {
            if( m_similarArtistMap.contains( m_currentArtist ) )
            {
                similarArtists = m_similarArtistMap.value( m_currentArtist );
                debug() << "for"<<m_currentArtist<<"got similar artists:" << similarArtists.join(", ");
            }
            else
            {
                locker.unlock();
                newSimilarQuery();
                return; // not yet ready to do construct a query maker
            }
        }
        else if( m_match == SimilarTrack )
        {
            TitleArtistPair key( m_currentTrack, m_currentArtist );
            if( m_similarTrackMap.contains( key ) )
            {
                similarTracks = m_similarTrackMap.value( key );
                debug() << "for"<<key<<"got similar tracks:" << similarTracks.count();
            }
            else
            {
                locker.unlock();
                newSimilarQuery();
                return; // not yet ready to do construct a query maker
            }
        }
    }

    // ok, I need a new query maker
    m_qm.reset( CollectionManager::instance()->queryMaker() );

    // - construct the query
    m_qm->beginOr();
    if( m_match == SimilarArtist )
    {
        for( const QString &name : similarArtists )
        {
            m_qm->addFilter( Meta::valArtist, name, true, true );
        }
    }
    else if( m_match == SimilarTrack )
    {
        for( const TitleArtistPair &name : similarTracks )
        {
            m_qm->beginAnd();
            m_qm->addFilter( Meta::valTitle, name.first, true, true );
            m_qm->addFilter( Meta::valArtist, name.second, true, true );
            m_qm->endAndOr();
        }
    }
    m_qm->endAndOr();

    m_qm->setQueryType( Collections::QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );

    connect( m_qm.data(), &Collections::QueryMaker::newResultReady,
             this, &LastFmBias::updateReady );
    connect( m_qm.data(), &Collections::QueryMaker::queryDone,
             this, &LastFmBias::updateFinished );

    // - run the query
    m_qm->run();
}


void Dynamic::LastFmBias::newSimilarQuery()
{
    DEBUG_BLOCK

    QMap< QString, QString > params;
    //   params[ "limit" ] = "70";
    if( m_match == SimilarArtist )
    {
        params[ "method" ] = "artist.getSimilar";
        params[ "artist" ] = m_currentArtist;
        QNetworkReply* request = lastfm::ws::get( params );
        connect( request, &QNetworkReply::finished,
                 this, &LastFmBias::similarArtistQueryDone );
    }
    else if( m_match == SimilarTrack )
    {
        // if( track->mb
        // TODO add mbid if the track has one
        params[ "method" ] = "track.getSimilar";
        params[ "artist" ] = m_currentArtist;
        params[ "track" ] = m_currentTrack;
        QNetworkReply* request = lastfm::ws::get( params );
        connect( request, &QNetworkReply::finished,
                 this, &LastFmBias::similarTrackQueryDone );
    }
}


void
Dynamic::LastFmBias::similarArtistQueryDone() // slot
{
    DEBUG_BLOCK

    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if( !reply )
    {
        queryFailed( "job was deleted from under us...wtf! blame the gerbils." );
        return;
    }
    reply->deleteLater();

    QByteArray data = reply->readAll();
//     debug() << "artistQuery has data:" << data;
    QDomDocument d;
    if( !d.setContent( data ) )
    {
        queryFailed( "Got invalid XML data from last.fm!" );
        return;
    }

    QDomNodeList nodes = d.elementsByTagName( "artist" );
    QStringList similarArtists;
    for( int i =0; i < nodes.size(); ++i )
    {
        QDomElement n = nodes.at( i ).toElement();
        //  n.firstChildElement( "match" ).text().toFloat() * 100,
        similarArtists.append( n.firstChildElement( "name" ).text() );
    }

    QMutexLocker locker( &m_mutex );

    m_similarArtistMap.insert( m_currentArtist, similarArtists );

    saveDataToFile();

    // -- try again to do the query
    newQuery();
}

void Dynamic::LastFmBias::similarTrackQueryDone()
{
    DEBUG_BLOCK

    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if( !reply )
    {
        queryFailed( "who send this...wtf! blame the gerbils." );
        return;
    }
    reply->deleteLater();

    // double match value, qpair title - artist
    QMap< int, QPair<QString,QString> > similar;
    QByteArray data = reply->readAll();
//     debug() << "trackQuery has data:" << data;
    QDomDocument d;
    if( !d.setContent( data ) )
    {
        queryFailed( "Got invalid XML data from last.fm!" );
        return;
    }

    QDomNodeList nodes = d.elementsByTagName( "track" );
    QList<TitleArtistPair> similarTracks;
    for( int i =0; i < nodes.size(); ++i )
    {
        QDomElement n = nodes.at( i ).toElement();
        //  n.firstChildElement( "match" ).text().toFloat() * 100,
        TitleArtistPair pair( n.firstChildElement( "name" ).text(),
                              n.firstChildElement( "artist" ).firstChildElement( "name" ).text() );
        similarTracks.append( pair );
    }

    QMutexLocker locker( &m_mutex );

    TitleArtistPair key( m_currentTrack, m_currentArtist );
    m_similarTrackMap.insert( key, similarTracks );

    saveDataToFile();

    // -- try again to do the query
    newQuery();
}


void
Dynamic::LastFmBias::queryFailed( const char *message )
{
    debug() << message;

    m_tracks.reset( false );
    Q_EMIT resultReady( m_tracks );
    return;
}


void
Dynamic::LastFmBias::saveDataToFile() const
{
    QFile file( Amarok::saveLocation() + "dynamic_lastfm_similar.xml" );
    if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
        return;

    QXmlStreamWriter writer( &file );
    writer.setAutoFormatting( true );

    writer.writeStartDocument();
    writer.writeStartElement( QLatin1String("lastfmSimilar") );

    // -- write the similar artists
    for( const QString& key : m_similarArtistMap.keys() )
    {
        writer.writeStartElement( QLatin1String("similarArtist") );
        writer.writeTextElement( QLatin1String("artist"), key );
        for( const QString& name : m_similarArtistMap.value( key ) )
        {
            writer.writeTextElement( QLatin1String("similar"), name );
        }
        writer.writeEndElement();
    }

    // -- write the similar tracks
    for( const TitleArtistPair& key : m_similarTrackMap.keys() )
    {
        writer.writeStartElement( QLatin1String("similarTrack") );
        writer.writeStartElement( QLatin1String("track") );
        writer.writeTextElement( QLatin1String("title"), key.first );
        writer.writeTextElement( QLatin1String("artist"), key.second );
        writer.writeEndElement();

        for( const TitleArtistPair& name : m_similarTrackMap.value( key ) )
        {
            writer.writeStartElement( QLatin1String("similar") );
            writer.writeTextElement( QLatin1String("title"), name.first );
            writer.writeTextElement( QLatin1String("artist"), name.second );
            writer.writeEndElement();
        }
        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeEndDocument();
}

void
Dynamic::LastFmBias::readSimilarArtists( QXmlStreamReader *reader )
{
    QString key;
    QList<QString> artists;

    while (!reader->atEnd()) {
        reader->readNext();
        QStringView name = reader->name();

        if( reader->isStartElement() )
        {
            if( name == QLatin1String("artist") )
                key = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else if( name == QLatin1String("similar") )
                artists.append( reader->readElementText(QXmlStreamReader::SkipChildElements) );
            else
                reader->skipCurrentElement();
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }

    m_similarArtistMap.insert( key, artists );
}

Dynamic::LastFmBias::TitleArtistPair
Dynamic::LastFmBias::readTrack( QXmlStreamReader *reader )
{
    TitleArtistPair track;

    while (!reader->atEnd()) {
        reader->readNext();
        QStringView name = reader->name();

        if( reader->isStartElement() )
        {
            if( name == QLatin1String("title") )
                track.first = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else if( name == QLatin1String("artist") )
                track.second = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else
                reader->skipCurrentElement();
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }

    return track;
}

void
Dynamic::LastFmBias::readSimilarTracks( QXmlStreamReader *reader )
{
    TitleArtistPair key;
    QList<TitleArtistPair> tracks;

    while (!reader->atEnd()) {
        reader->readNext();
        QStringView name = reader->name();

        if( reader->isStartElement() )
        {
            if( name == QLatin1String("track") )
                key = readTrack( reader );
            else if( name == QLatin1String("similar") )
                tracks.append( readTrack( reader ) );
            else
                reader->skipCurrentElement();
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }

    m_similarTrackMap.insert( key, tracks );
}

void
Dynamic::LastFmBias::loadDataFromFile()
{
    m_similarArtistMap.clear();
    m_similarTrackMap.clear();

    QFile file( Amarok::saveLocation() + "dynamic_lastfm_similar.xml" );

    if( !file.exists() ||
        !file.open( QIODevice::ReadOnly ) )
        return;

    QXmlStreamReader reader( &file );

    while (!reader.atEnd()) {
        reader.readNext();

        QStringView name = reader.name();
        if( reader.isStartElement() )
        {
            if( name == QLatin1String("lastfmSimilar") )
            {
                ; // just recurse into the element
            }
            else if( name == QLatin1String("similarArtist") )
            {
                readSimilarArtists( &reader );
            }
            else if( name == QLatin1String("similarTrack") )
            {
                readSimilarTracks( &reader );
            }
            else
            {
                reader.skipCurrentElement();
            }
        }
        else if( reader.isEndElement() )
        {
            break;
        }
    }
}

Dynamic::LastFmBias::MatchType
Dynamic::LastFmBias::match() const
{ return m_match; }

void
Dynamic::LastFmBias::setMatch( Dynamic::LastFmBias::MatchType value )
{
    m_match = value;
    invalidate();
    Q_EMIT changed( BiasPtr(this) );
}

void
Dynamic::LastFmBias::setMatchTypeArtist( bool matchArtist )
{
    setMatch( matchArtist ? SimilarArtist : SimilarTrack );
}

QString
Dynamic::LastFmBias::nameForMatch( Dynamic::LastFmBias::MatchType match )
{
    switch( match )
    {
    case SimilarArtist: return "artist";
    case SimilarTrack:  return "track";
    }
    return QString();
}

Dynamic::LastFmBias::MatchType
Dynamic::LastFmBias::matchForName( const QString &name )
{
    if( name == "artist" )     return SimilarArtist;
    else if( name == "track" ) return SimilarTrack;
    else return SimilarArtist;
}




