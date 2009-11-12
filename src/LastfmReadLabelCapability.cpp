/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "LastfmReadLabelCapability.h"

#include "Debug.h"
#include "meta/Meta.h"

#include <QMap>
#include <QNetworkReply>

#include <lastfm/XmlQuery>
#include <ws.h>

static const QString &APIKEY = "402d3ca8e9bc9d3cf9b85e1202944ca5";
namespace Meta
{
LastfmReadLabelCapability::LastfmReadLabelCapability( Meta::Track *track )
    : ReadLabelCapability()
    , m_track( track )
{
    DEBUG_BLOCK
    fetchLabels();
}

void
LastfmReadLabelCapability::fetchLabels()
{
    DEBUG_BLOCK
    QMap<QString,QString> query;
    query[ "method" ] = "track.getTopTags";
    query[ "track"  ] = m_track->name();
    query[ "artist" ] = m_track->artist() ? m_track->artist()->name() : QString();
    query[ "api_key"] = APIKEY;
    m_job  = lastfm::ws::post( query );

    connect( m_job, SIGNAL( finished() ), SLOT(onTagsFetched()) );
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
            lastfm::XmlQuery lfm = m_job->readAll();
            QList<lastfm::XmlQuery> tags = lfm.children( "tag" );
            QStringList ret;
            foreach( lastfm::XmlQuery child, tags )
                ret.append( child["name"].text() );
            m_labels = ret;
            emit labelsFetched( ret );
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

#include "LastfmReadLabelCapability.moc"