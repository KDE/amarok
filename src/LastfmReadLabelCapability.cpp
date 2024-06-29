/***************************************************************************************
* Copyright (c) 2009 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
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

#include "LastfmReadLabelCapability.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/meta/Meta.h"

#include <QMap>
#include <QNetworkReply>

#include <XmlQuery.h>

namespace Capabilities
{
LastfmReadLabelCapability::LastfmReadLabelCapability( Meta::Track *track )
    : ReadLabelCapability()
    , m_track( track )
{
    DEBUG_BLOCK
    fetchLabels();
}

void
LastfmReadLabelCapability::fetchGlobalLabels()
{
    DEBUG_BLOCK
    AMAROK_NOTIMPLEMENTED
}

void
LastfmReadLabelCapability::fetchLabels()
{
    DEBUG_BLOCK
    QMap<QString,QString> query;
    query[ QStringLiteral("method") ] = QStringLiteral("track.getTopTags");
    query[ QStringLiteral("track")  ] = m_track->name();
    query[ QStringLiteral("artist") ] = m_track->artist() ? m_track->artist()->name() : QString();
    query[ QStringLiteral("api_key")] = QLatin1String(Amarok::lastfmApiKey());
    m_job  = lastfm::ws::post( query );

    connect( m_job, &QNetworkReply::finished, this, &LastfmReadLabelCapability::onTagsFetched );
}


void
LastfmReadLabelCapability::onTagsFetched()
{
    DEBUG_BLOCK
    if( !m_job )
    {
        debug() << "WARNING: GOT RESULT but no object";
        return;
    }

    switch ( m_job->error() )
    {
        case QNetworkReply::NoError:
        {
            lastfm::XmlQuery lfm;
            lfm.parse(m_job->readAll());
            QList<lastfm::XmlQuery> tags = lfm.children( QStringLiteral("tag") );
            QStringList ret;
            for( const lastfm::XmlQuery &child : tags )
                ret.append( child[QStringLiteral("name")].text() );
            m_labels = ret;
            Q_EMIT labelsFetched( ret );
            break;
        }
        default:
            break;
    }
}


QStringList
LastfmReadLabelCapability::labels()
{
    return m_labels;
}

}

