/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#include "NepomukComposer.h"
#include "NepomukTrack.h"

#include "core/support/Debug.h"
#include "core/meta/Meta.h"

#include <Nepomuk/Resource>
#include <Nepomuk/Vocabulary/NMM>
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/AndTerm>
#include <Nepomuk/Query/ResourceTypeTerm>
#include <Nepomuk/Query/Result>
#include <Nepomuk/Query/QueryServiceClient>
#include <Nepomuk/Query/LiteralTerm>
#include <QString>

using namespace Meta;
using namespace Nepomuk::Query;

NepomukComposer::NepomukComposer( QString &name )
    : Meta::Composer()
    , m_name( name )
{

}

TrackList
NepomukComposer::tracks()
{
    // get all audio tracks
    ResourceTypeTerm tracks( Nepomuk::Vocabulary::NFO::Audio() );
    // get all composers/performers with given name
    ComparisonTerm composers( Nepomuk::Vocabulary::NMM::composer(),
            LiteralTerm( m_name ) );
    // now 'and' the two
    Query query( AndTerm( tracks, composers ) );
    // get the result set from the constructed query
    QList<Result> results =
        QueryServiceClient::syncQuery( query );

    TrackList tracklist;

    // construct tracklist from the obtained result list
    Q_FOREACH( const Result & result, results )
    {

        debug() << "NepomukComposer : track : " << result.resource().genericLabel();

        NepomukTrackPtr track( new NepomukTrack( result.resource() ) );
        tracklist.append( Meta::TrackPtr::staticCast( track ) );

    }

    return tracklist;
}

QString
NepomukComposer::name() const
{
    return m_name;
}

void
NepomukComposer::notifyObservers() const
{

}
