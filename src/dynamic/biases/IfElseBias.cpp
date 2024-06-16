/****************************************************************************************
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "IfElseBias"

#include "IfElseBias.h"

#include "amarokconfig.h"
#include "core/meta/Meta.h"

#include <QtGlobal> // for qRound
#include <QPainter>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <KLocalizedString>

QString
Dynamic::IfElseBiasFactory::i18nName() const
{ return i18nc("Name of the \"IfElse\" bias", "If Else"); }

QString
Dynamic::IfElseBiasFactory::name() const
{ return Dynamic::IfElseBias::sName(); }

QString
Dynamic::IfElseBiasFactory::i18nDescription() const
{ return i18nc("Description of the \"IfElse\" bias",
                   "The \"IfElse\" bias adds tracks that match at least one of the sub biases.\nIt will only check the second sub-bias if the first doesn't return any results."); }

Dynamic::BiasPtr
Dynamic::IfElseBiasFactory::createBias()
{ return Dynamic::BiasPtr( new Dynamic::IfElseBias() ); }




Dynamic::IfElseBias::IfElseBias()
    : OrBias()
{ }

QString
Dynamic::IfElseBias::sName()
{
    return QStringLiteral( "ifElseBias" );
}

QString
Dynamic::IfElseBias::name() const
{
    return Dynamic::IfElseBias::sName();
}

QString
Dynamic::IfElseBias::toString() const
{
    return i18nc("IfElse bias representation", "Match all sequentially");
}

void
Dynamic::IfElseBias::paintOperator( QPainter* painter, const QRect& rect, Dynamic::AbstractBias* bias )
{
    if( m_biases.indexOf( Dynamic::BiasPtr(bias) ) > 0 )
        painter->drawText( rect.adjusted(2, 0, -2, 0),
                           Qt::AlignRight,
                           i18nc("Prefix for IfElseBias. Shown in front of a bias in the dynamic playlist view", "else" ) );
}

Dynamic::TrackSet
Dynamic::IfElseBias::matchingTracks( const Meta::TrackList& playlist,
                                     int contextCount, int finalCount,
                                     const Dynamic::TrackCollectionPtr &universe ) const
{
    // store the parameters in case we need to request additional matching tracks later
    m_playlist = playlist;
    m_contextCount = contextCount;
    m_finalCount = finalCount;
    m_universe = universe;

    m_tracks = Dynamic::TrackSet( universe, false );
    m_outstandingMatches = 0;

    for( Dynamic::BiasPtr bias : m_biases )
    {
        m_tracks = bias->matchingTracks( playlist, contextCount, finalCount, universe );
        if( m_tracks.isOutstanding() )
        {
            m_outstandingMatches++;
            return m_tracks;
        }
        else
        {
            removeDuplicate();
            if( !m_tracks.isEmpty() ) {
                return m_tracks;
            }
        }
    }

    return m_tracks;
}

void
Dynamic::IfElseBias::resultReceived( const Dynamic::TrackSet &tracks )
{
    m_tracks = tracks;
    --m_outstandingMatches;

    // we got some tracks result
    removeDuplicate();
    if( !m_tracks.isEmpty() )
    {
        Q_EMIT resultReady( m_tracks );
        return;
    }

    // ok. check the next biases
    bool alreadyChecked = true;
    for( Dynamic::BiasPtr bias : m_biases )
    {
        // jump over already checked biases
        if( bias.data() == sender() )
        {
            alreadyChecked = false;
            continue;
        }
        else if( alreadyChecked )
        {
            continue;
        }

        // check the next bias
        m_tracks = bias->matchingTracks( m_playlist, m_contextCount, m_finalCount, m_universe );
        if( m_tracks.isOutstanding() )
        {
            m_outstandingMatches++;
            return; // wait for the next results
        }
        else
        {
            removeDuplicate();
            if( !m_tracks.isEmpty() ) {
                Q_EMIT resultReady( m_tracks );
                return;
            }
        }
    }
    Q_EMIT resultReady( m_tracks );
}

// we need to eliminate duplicates now to have a proper check for an empty result
void
Dynamic::IfElseBias::removeDuplicate() const
{
    if( AmarokConfig::dynamicDuplicates() )
        return;

    for( int i = 0; i < m_playlist.count(); i++ )
        if( m_playlist[i] )
            m_tracks.subtract( m_playlist[i] );
}


