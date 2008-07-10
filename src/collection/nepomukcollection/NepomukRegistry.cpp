/* 
   Copyright (C) 2008 Daniel Winter <dw@danielwinter.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "NepomukCollection.h"
#include "NepomukRegistry.h"
#include "NepomukTrack.h"

#include "Debug.h"
#include "QueryMaker.h"
#include <QHash>
#include <QUrl>
#include <QUuid>

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
        Nepomuk::Resource &m_resource;
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
        Meta::NepomukTrackPtr tp ( new Meta::NepomukTrack( m_collection, this, set ) );
        if ( tp->uid().isEmpty() )
        {
            tp->setUid( QUuid::createUuid().toString().mid( 1, 36 ) );
            ThreadWeaver::Job *job =
                    new NepomukWriteJob( tp->resource() , QUrl( "http://amarok.kde.org/metadata/1.0/track#uid" ), Nepomuk::Variant( tp->uid() ) );
            m_weaver->enqueue( job );
        }
        m_tracks[ tp->resource().resourceUri().toString() ] = tp;
        m_tracksFromId[ tp->uid() ] = tp;
        
        return (Meta::TrackPtr)tp.data();
    }
}

void
NepomukRegistry::cleanHash()
{
    for( QMutableHashIterator< QString , KSharedPtr<Meta::NepomukTrack> > it( m_tracks); it.hasNext(); )
    {
        Meta::NepomukTrackPtr track = it.next().value();
        if( track.count() == 3 )
        {
            it.remove();
        }
    }
    for( QMutableHashIterator< QString , KSharedPtr<Meta::NepomukTrack> > it( m_tracksFromId); it.hasNext(); )
    {
        Meta::NepomukTrackPtr track = it.next().value();
        if( track.count() == 2 )
        {
            it.remove();
        }
    }
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
                    m_tracks[ uri ].data()->valueChangedInNepomuk( value, object.literal() );
            }
        }
        else if ( statement.predicate().uri().toString() == "http://amarok.kde.org/metadata/1.0/track#uid" )
        {
            Soprano::Node object = statement.object();
            QString uid = object.literal().toString();
            
            debug() << "nepomuk resource moved uid: " << uid << endl;
            if ( m_tracksFromId.contains( uid ) )
            {
                Meta::NepomukTrackPtr tp = m_tracksFromId[ uid ];
                QString oldurl = tp->resource().resourceUri().toString();
                debug() << "nepo old uld: " << oldurl << " new url" << uri << endl;
                tp->setResource( Nepomuk::Resource( uri ) );
                tp->valueChangedInNepomuk( QueryMaker::valUrl, uri );

                // update hash
                debug() << "nepo length before " << m_tracks.count() << endl;
                m_tracks[ uri ] = tp;
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
