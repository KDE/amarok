/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2010, 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "EchoNestBias.h"

#include "core/support/Debug.h"

#include "TrackSet.h"
#include "DynamicBiasWidgets.h"

#include <QDomDocument>
#include <QDomNode>
#include <QDomElement>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QTimer>

#include "core/meta/Meta.h"
#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <kio/job.h>

#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>


QString
Dynamic::EchoNestBiasFactory::i18nName() const
{ return i18nc("Name of the \"EchoNest\" bias", "EchoNest similar artist"); }

QString
Dynamic::EchoNestBiasFactory::name() const
{ return Dynamic::EchoNestBias::sName(); }

QString
Dynamic::EchoNestBiasFactory::i18nDescription() const
{ return i18nc("Description of the \"EchoNest\" bias",
                   "The \"EchoNest\" bias looks up tracks on echo nest and only adds similar tracks."); }

Dynamic::BiasPtr
Dynamic::EchoNestBiasFactory::createBias()
{ return Dynamic::BiasPtr( new Dynamic::EchoNestBias() ); }


// ----- EchoNestBias --------

Dynamic::EchoNestBias::EchoNestBias()
    : SimpleMatchBias()
    , m_artistSuggestedQuery( 0 )
    , m_match( PreviousTrack )
    , m_mutex( QMutex::Recursive )
{ }

Dynamic::EchoNestBias::~EchoNestBias()
{
    // TODO: kill all running queries
}

void
Dynamic::EchoNestBias::fromXml( QXmlStreamReader *reader )
{
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();
            if( name == "match" )
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
Dynamic::EchoNestBias::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( "match", nameForMatch( m_match ) );
}

QString
Dynamic::EchoNestBias::sName()
{
    return QLatin1String( "echoNestBias" );
}

QString
Dynamic::EchoNestBias::name() const
{
    return Dynamic::EchoNestBias::sName();
}

QString
Dynamic::EchoNestBias::toString() const
{
    switch( m_match )
    {
    case PreviousTrack:
        return i18nc("EchoNest bias representation",
                     "Similar to the previous track (as reported by EchoNest)");
    case Playlist:
        return i18nc("EchoNest bias representation",
                     "Similar any track in the current playlist (as reported by EchoNest)");
    }
    return QString();
}

QWidget*
Dynamic::EchoNestBias::widget( QWidget* parent )
{
    QWidget *widget = new QWidget( parent );
    QVBoxLayout *layout = new QVBoxLayout( widget );

    QLabel *label = new QLabel( i18n( "Echo nest thinks the track is similar to" ) );

    QComboBox *combo = new QComboBox();
    combo->addItem( i18n( "the previous Track" ),
                    nameForMatch( PreviousTrack ) );
    combo->addItem( i18n( "one of the tracks in the current playlist" ),
                    nameForMatch( Playlist ) );
    switch( m_match )
    {
    case PreviousTrack: combo->setCurrentIndex(0); break;
    case Playlist:      combo->setCurrentIndex(1); break;
    }
    connect( combo, SIGNAL( currentIndexChanged(int) ),
             this, SLOT( selectionChanged( int ) ) );
    label->setBuddy( combo );
    layout->addWidget( label );
    layout->addWidget( combo );

    return widget;
}

Dynamic::TrackSet
Dynamic::EchoNestBias::matchingTracks( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount,
                                       Dynamic::TrackCollectionPtr universe ) const
{
    Q_UNUSED( contextCount );

    // collect the artist
    QStringList artists = currentArtists( position, playlist );
    if( artists.isEmpty() )
        return Dynamic::TrackSet( universe, true );

    {
        QMutexLocker locker( &m_mutex );
        QString key = artists.join("|");
        if( m_tracksValid && m_tracksMap.contains( key ) )
            return m_tracksMap.value( key );
    }

    m_tracks = Dynamic::TrackSet( universe, false );
    m_currentArtists = artists;
    QTimer::singleShot(0,
                       const_cast<EchoNestBias*>(this),
                       SLOT(newQuery())); // create the new query from my parent thread

    return Dynamic::TrackSet();
}


bool
Dynamic::EchoNestBias::trackMatches( int position,
                                     const Meta::TrackList& playlist,
                                     int contextCount ) const
{
    Q_UNUSED( contextCount );

    // collect the artist
    QStringList artists = currentArtists( position, playlist );
    if( artists.isEmpty() )
        return true;

    // the artist of this track
    if( position < 0 || position >= playlist.count() )
        return false;

    Meta::TrackPtr track = playlist[position];
    Meta::ArtistPtr artist = track->artist();
    if( !artist || artist->name().isEmpty() )
        return false;

    {
        QMutexLocker locker( &m_mutex );
        QString key = artists.join("|");
        if( m_similarArtistMap.contains( key ) )
            return m_similarArtistMap.value( key ).contains( artist->name() );
    }
    warning() << "didn't have artist suggestions saved for this artist:" << artist->name();
    return false;
}


void
Dynamic::EchoNestBias::invalidate()
{
    SimpleMatchBias::invalidate();
    m_tracksMap.clear();
}

void
Dynamic::EchoNestBias::newQuery()
{
    DEBUG_BLOCK;

    // - get the similar artists
    QStringList similar;
    {
        QMutexLocker locker( &m_mutex );
        QString key = m_currentArtists.join("|");
        if( m_similarArtistMap.contains( key ) )
        {
            similar = m_similarArtistMap.value( key );
            debug() << "got similar artists:" << similar.join(", ");
        }
        else
        {
            newSimilarArtistQuery();
            return; // not yet ready to do construct a query maker
        }
    }

    // ok, I need a new query maker
    m_qm.reset( CollectionManager::instance()->queryMaker() );

    // - construct the query
    m_qm->beginOr();
    foreach( const QString &artistName, similar )
    {
        m_qm->addFilter( Meta::valArtist, artistName, true, true );

    }
    m_qm->endAndOr();

    m_qm->setQueryType( Collections::QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );

    connect( m_qm.data(), SIGNAL(newResultReady( QString, QStringList )),
             this, SLOT(updateReady( QString, QStringList )) );
    connect( m_qm.data(), SIGNAL(queryDone()),
             this, SLOT(updateFinished()) );

    // - run the query
    m_qm.data()->run();
}

void
Dynamic::EchoNestBias::newSimilarArtistQuery()
{
    QMultiMap< QString, QString > params;
    {
        QMutexLocker locker( &m_mutex );

        // -- collect all the artist ids
        bool artistMissing = false;
        foreach( const QString &artist, m_currentArtists )
        {
            if( !m_artistIds.contains(artist) )
            {
                newArtistIdQuery( artist );
                artistMissing = true;
            }
            else if( m_artistIds[artist] == "-1" ) // still waiting
            {
                debug() << "not done yet with query. Missing"<< artist;
                artistMissing = true;
            }
            else
            {
                params.insert("id", m_artistIds[ artist ] );
            }
        }
        if( artistMissing )
            return; // not yet ready to do construct a similar artist query
    }

    // -- start the query
    params.insert( "rows", "30" );
    m_artistSuggestedQuery = KIO::storedGet( createUrl( "get_similar", params ), KIO::NoReload, KIO::HideProgressInfo );
    connect( m_artistSuggestedQuery, SIGNAL( result( KJob* ) ),
             this, SLOT( similarArtistQueryDone( KJob* ) ) );
}

void
Dynamic::EchoNestBias::newArtistIdQuery( const QString &artist )
{
    debug() << "searching for artist" << artist;

    QMutexLocker locker( &m_mutex );
    QMap< QString, QString > params;

    params[ "query" ] = artist;
    params[ "exact" ] = 'Y';
    params[ "sounds_like" ] = 'N'; // mutually exclusive with exact

    KIO::StoredTransferJob* job = KIO::storedGet( createUrl( "search_artists", params ), KIO::NoReload, KIO::HideProgressInfo );
    m_artistNameQueries[ job ] = artist;

    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( artistIdQueryDone( KJob* ) ) );

    m_artistIds[artist] = "-1"; // mark as not being searched for
}

void
Dynamic::EchoNestBias::artistIdQueryDone( KJob* job )
{
    KIO::StoredTransferJob* stjob = static_cast< KIO::StoredTransferJob* >( job );

    if( !m_artistNameQueries.contains( stjob ) )
    {
        debug() << "job was deleted before the slot was called..wtf?";
        return;
    }

    QDomDocument doc;
    if( ! doc.setContent( stjob->data() ) )
    {
        debug() << "Got invalid XML from EchoNest on solo artist name query!";
        return;
    }

    QDomNodeList artists = doc.elementsByTagName( "artist" );
    if( artists.isEmpty() )
    {
        debug() << "got no artist for given name :-/";
        return;
    }

    QDomNode artist = artists.at( 0 );
    QString id = artist.firstChildElement( "id" ).text();
    debug() << "got element ID:" << id;

    if( id.isEmpty() )
    {
        debug() << "Got empty ID though :(";
        return;
    }

    m_artistIds[ m_artistNameQueries[ stjob ] ] = id;
    job->deleteLater();

    // -- try again to do the query
    newQuery();
}

void
Dynamic::EchoNestBias::similarArtistQueryDone( KJob* job ) // slot
{
    if( job != m_artistSuggestedQuery )
    {
        debug() << "job was deleted from under us...wtf! blame the gerbils.";
        return;
    }

    QDomDocument doc;
    if( !doc.setContent( m_artistSuggestedQuery->data() ) )
    {
        debug() << "got invalid XML from EchoNest::get_similar!";
        return;
    }
    QDomNodeList list = doc.elementsByTagName( "similar" );
    if( list.size() != 1 )
    {
        debug() << "Got no similar artists! Bailing!";
        return;
    }

    // -- decode the result
    QDomElement similar = list.at( 0 ).toElement();
    //debug() << "got similar artists:" << similar.values();

    QStringList similarArtists;
    QDomNodeList rel = similar.childNodes();
    for( int i = 0; i < rel.count(); i++ )
    {
        QDomNode n = rel.at( i );
        similarArtists.append( n.firstChildElement( "name" ).text() );
    }

    // -- commit the result
    {
        QMutexLocker locker( &m_mutex );
        QString key = m_currentArtists.join("|");
        m_similarArtistMap.insert( key, similarArtists );
    }
    job->deleteLater();

    newQuery();
}

void
Dynamic::EchoNestBias::updateFinished()
{
    // -- store away the result for future reference
    QString key = m_currentArtists.join( "|" );
    m_tracksMap.insert( key, m_tracks );
    debug() << "saving found similar tracks to key:" << key;

    SimpleMatchBias::updateFinished();
}

QStringList
Dynamic::EchoNestBias::currentArtists( int position, const Meta::TrackList& playlist ) const
{
    QStringList result;

    if( m_match == PreviousTrack )
    {
        if( position > 0 && position < playlist.count() - 1 )
        {
            Meta::ArtistPtr artist = playlist[ position-1 ]->artist();
            if( artist && !artist->name().isEmpty() )
                result.append( artist->name() );
        }
    }
    else if( m_match == Playlist )
    {
        for( int i=0; i < position && i < playlist.count(); i++ )
        {
            Meta::ArtistPtr artist = playlist[i]->artist();
            if( artist && !artist->name().isEmpty() )
                result.append( artist->name() );
        }
    }

    return result;
}


// this method shamelessly inspired by liblastfm/src/ws/ws.cpp
KUrl Dynamic::EchoNestBias::createUrl( QString method, QMultiMap< QString, QString > params )
{
    params.insert( "api_key", "DD9P0OV9OYFH1LCAE" );
    params.insert( "version", "3" );

    // debug() << "got param map:" << params;

    KUrl url;
    url.setScheme( "http" );
    url.setHost( "developer.echonest.com" );
    url.setPath( "/api/" + method );

    // take care of the ID possibility  manually
    // Qt setQueryItems doesn't encode a bunch of stuff, so we do it manually
    QMapIterator<QString, QString> i( params );
    while ( i.hasNext() ) {
        i.next();
        QByteArray const key = QUrl::toPercentEncoding( i.key() );
        QByteArray const value = QUrl::toPercentEncoding( i.value() );
        url.addEncodedQueryItem( key, value );
    }

    // debug() << "created url for EchoNest request:" << url;

    return url;
}


Dynamic::EchoNestBias::MatchType
Dynamic::EchoNestBias::match() const
{ return m_match; }

void
Dynamic::EchoNestBias::setMatch( Dynamic::EchoNestBias::MatchType value )
{
    m_match = value;
    invalidate();
    emit changed( BiasPtr(this) );
}


void
Dynamic::EchoNestBias::selectionChanged( int which )
{
    if( QComboBox *box = qobject_cast<QComboBox*>(sender()) )
        setMatch( matchForName( box->itemData( which ).toString() ) );
}


QString
Dynamic::EchoNestBias::nameForMatch( Dynamic::EchoNestBias::MatchType match )
{
    switch( match )
    {
    case Dynamic::EchoNestBias::PreviousTrack: return "previous";
    case Dynamic::EchoNestBias::Playlist:      return "playlist";
    }
    return QString();
}

Dynamic::EchoNestBias::MatchType
Dynamic::EchoNestBias::matchForName( const QString &name )
{
    if( name == "previous" )      return PreviousTrack;
    else if( name == "playlist" ) return Playlist;
    else return PreviousTrack;
}



#include "EchoNestBias.moc"
