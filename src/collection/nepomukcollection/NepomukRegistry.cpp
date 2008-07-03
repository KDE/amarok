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

#include "Debug.h"
#include "QueryMaker.h"

#include <QHash>
#include <QUrl>

#include <KSharedPtr>
#include <Soprano/BindingSet>
#include <Soprano/Model>
#include <Soprano/Statement>

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

}


NepomukRegistry::~NepomukRegistry()
{
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
        m_tracks[ url ] = tp;
        
        return (Meta::TrackPtr)tp.data();
    }
}

void
NepomukRegistry::cleanHash()
{
    for( QMutableHashIterator< QString , KSharedPtr<Meta::NepomukTrack> > it( m_tracks); it.hasNext(); )
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
            QString uriProperty = statement.predicate().uri();
            qint64 value = m_collection->valueForUrl( uriProperty );
            if ( value != 0)
            {
                Soprano::Node object = statement.object();
                if ( object.isLiteral() )
                    m_tracks[ uri ].data()->valueChangedInNepomuk( value, object.literal() );
            }
        }
    }
}

#include "NepomukRegistry.moc"
