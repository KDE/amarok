/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
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

#include "NepomukRegistry.h"

#include "NepomukCollection.h"

#include "core/support/Debug.h"
#include "core/collections/QueryMaker.h"

#include <QDateTime>
#include <QUrl>
#include <QUuid>

#include <KMD5>
#include <KSharedPtr>
#include <Nepomuk/Resource>
#include <Nepomuk/Variant>
#include <Soprano/BindingSet>
#include <Soprano/Model>
#include <Soprano/Statement>
#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

class NepomukWriteJob : public ThreadWeaver::Job
{
    public:
        NepomukWriteJob( Nepomuk::Resource &resource, const QUrl property,  const Nepomuk::Variant value )
             : ThreadWeaver::Job()
                , m_resource( resource )
                , m_property( property )
                , m_value( value )
                {
            //nothing to do
                }
       NepomukWriteJob( const QString &resourceUri, const QUrl property,  const Nepomuk::Variant value )
                   : ThreadWeaver::Job()
                        , m_property( property )
                        , m_value( value )
        {
            m_resource =  Nepomuk::Resource( QUrl( resourceUri ) );
        }
    protected:
        virtual void run()
        {
            if (m_resource.isValid() )
            {
                    m_resource.setProperty( m_property, m_value );
            }
            else
                debug() << "m_resource is not not valid" << endl;
        }
    
    private:
        Nepomuk::Resource m_resource;
        const QUrl m_property;
        const Nepomuk::Variant m_value;
};


NepomukRegistry::NepomukRegistry( NepomukCollection *collection, Soprano::Model *model )
    : m_collection( collection )
    , m_model ( model )
{
    m_timer = new QTimer( this );
    m_timer->setInterval( 60000 );  //try to clean up every 60 seconds, change if necessary
    m_timer->setSingleShot( false );
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( cleanHash() ) );
    m_timer->start();

    connect ( model, SIGNAL( statementAdded(const Soprano::Statement& ) ),
                      this, SLOT ( nepomukUpdate( const Soprano::Statement& ) ) );

    m_weaver = new ThreadWeaver::Weaver();
    m_weaver->setMaximumNumberOfThreads( 1 );

    connect ( m_weaver, SIGNAL( jobDone (ThreadWeaver::Job* ) ),
                        this, SLOT ( jobDone (ThreadWeaver::Job* ) ) );

}


NepomukRegistry::~NepomukRegistry()
{
    m_weaver->finish();
    delete m_weaver;
}


Meta::TrackPtr
NepomukRegistry::trackForBindingSet( const Soprano::BindingSet &set )
{
    QString url = set[ "r"].uri().toString();
    if ( m_tracks.contains( url ) )
        return (Meta::TrackPtr) m_tracks[ url ].data();
    else 
    {
        Meta::NepomukTrackPtr tp( new Meta::NepomukTrack( m_collection, this, set ) );
        if ( tp->uid().isEmpty() )
        {
            tp->setUid( createUuid() );
            ThreadWeaver::Job *job =
                    new NepomukWriteJob( tp->resource() , QUrl( "http://amarok.kde.org/metadata/1.0/track#uid" ), Nepomuk::Variant( tp->uid() ) );
            m_weaver->enqueue( job );
        }
        m_tracks[ tp->resource().resourceUri().toString() ] =  Meta::TrackPtr::staticCast( tp );
        m_tracksFromId[ tp->uid() ] = Meta::TrackPtr::staticCast( tp );
        
        return  Meta::TrackPtr::staticCast( tp );
    }
}

Meta::AlbumPtr
NepomukRegistry::albumForArtistAlbum( const QString &artist, const QString &album )
{
    QString id = albumId( artist, album );
    if ( m_albums.contains( id ) )
        return m_albums[ id ];
    else
    {
        Meta::AlbumPtr ap( new Meta::NepomukAlbum( m_collection, album, artist ) );
        m_albums[ id ] = ap;
        return ap;
    }
}

Meta::ArtistPtr
NepomukRegistry::artistForArtistName( const QString &artist )
{
    if ( m_artists.contains( artist ) )
        return m_artists[ artist ];
    else
    {
        Meta::ArtistPtr ap ( new Meta::NepomukArtist( m_collection, artist ) );
        m_artists[ artist ] = ap;
        return ap;
    }
}

void
NepomukRegistry::writeToNepomukAsync( Nepomuk::Resource &resource, const QUrl property,  const Nepomuk::Variant value ) const
{
    // TODO: Find a way to block when the queue is already very long (more than 100 jobs?)
    ThreadWeaver::Job *job =
            new NepomukWriteJob( resource , property , value );
    m_weaver->enqueue( job );
}

void
NepomukRegistry::writeToNepomukAsync( const QString &resourceUri, const QUrl property,  const Nepomuk::Variant value ) const
{
    // TODO: Find a way to block when the queue is already very long (more than 100 jobs?)
    ThreadWeaver::Job *job =
            new NepomukWriteJob( resourceUri , property , value );
    m_weaver->enqueue( job );
}

QString
NepomukRegistry::albumId( QString artist, QString album ) const
{
    // this returns a string which should be unique for every pair of artist / album

    // "escape" | because it is used to separate artist and album
    if ( !artist.isEmpty() )
    {
        artist.replace( '|', "||" );
    }
    album.replace( '|', "||" );

    return artist + '|' + album;
}

QString
NepomukRegistry::createUuid() const
{
    KMD5 context;
    context.update( QUuid::createUuid().toString().toLocal8Bit() );
    context.update( QString::number( QDateTime::currentDateTime().toTime_t() ).toLocal8Bit() );
    return context.hexDigest();
}

void
NepomukRegistry::cleanHash()
{
    #define foreachInvalidateCache( Type, RealType, x ) \
        for( QMutableHashIterator<QString,Type > iter(x); iter.hasNext(); ) \
            RealType::staticCast( iter.next().value() )->emptyCache()

    // are these needed? they ( they (the albums and artist object, will get deleted if not needed anymore anyway
    //foreachInvalidateCache( Meta::AlbumPtr, KSharedPtr<Meta::NepomukAlbum>, m_albums );
    //foreachInvalidateCache( Meta::ArtistPtr, KSharedPtr<Meta::NepomukArtist>, m_artists );

    for( QMutableHashIterator< QString , KSharedPtr<Meta::Track> > it( m_tracks); it.hasNext(); )
    {
        Meta::TrackPtr track = it.next().value();
        if( track.count() == 3 )
        {
            it.remove();
        }
    }

    //elem.count() == 2 is correct because elem is one pointer to the object
    //and the other is stored in the hash map
    #define foreachCollectGarbage( Key, Type, x ) \
        for( QMutableHashIterator<Key,Type > iter(x); iter.hasNext(); ) \
        { \
            Type elem = iter.next().value(); \
            if( elem.count() == 2 ) \
                iter.remove(); \
        }

    foreachCollectGarbage( QString, Meta::TrackPtr, m_tracks )
    //run before artist so that album artist pointers can be garbage collected
    foreachCollectGarbage( QString, Meta::AlbumPtr, m_albums )
    foreachCollectGarbage( QString, Meta::ArtistPtr, m_artists )
}

void
NepomukRegistry::nepomukUpdate( const Soprano::Statement &statement )
{
    Soprano::Node node = statement.subject();
    if (node.isResource())
    {
        QString uri = node.uri().toString();
        if ( m_tracks.contains( uri ) )
        {
            QString uriProperty = statement.predicate().uri().toString();
            qint64 value = m_collection->valueForUrl( uriProperty );
            if ( value != 0)
            {
                Soprano::Node object = statement.object();
                if ( object.isLiteral() )
                    KSharedPtr<Meta::NepomukTrack>::staticCast( m_tracks[ uri ] )->valueChangedInNepomuk( value, object.literal() );
            }
        }
        else if ( statement.predicate().uri().toString() == "http://amarok.kde.org/metadata/1.0/track#uid" )
        {
            Soprano::Node object = statement.object();
            QString uid = object.literal().toString();
            
            debug() << "nepomuk resource moved uid: " << uid << endl;
            if ( m_tracksFromId.contains( uid ) )
            {
                Meta::NepomukTrackPtr tp = Meta::NepomukTrackPtr::staticCast( m_tracksFromId[ uid ] );
                QString oldurl = tp->resource().resourceUri().toString();
                debug() << "nepo old uld: " << oldurl << " new url" << uri << endl;
                tp->setResource( Nepomuk::Resource( uri ) );
                tp->valueChangedInNepomuk( Meta::valUrl, uri );

                // update hash
                debug() << "nepo length before " << m_tracks.count() << endl;
                m_tracks[ uri ] = Meta::TrackPtr::staticCast( tp );
                m_tracks.remove( oldurl );
                debug() << "nepo length aftere " << m_tracks.count() << endl;
            }
        }
        
    }
}

void
NepomukRegistry::jobDone( ThreadWeaver::Job *job )
{
    job->deleteLater();
}

#include "NepomukRegistry.moc"

