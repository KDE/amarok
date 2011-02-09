/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2010,2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "PartBias"

#include "PartBias.h"

#include "core/support/Debug.h"
#include "DynamicBiasWidgets.h"

#include <QtGlobal> // for qRound
#include <QXmlStreamReader>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <klocale.h>

QString
Dynamic::PartBiasFactory::i18nName() const
{ return i18nc("Name of the \"Part\" bias", "Partition"); }

QString
Dynamic::PartBiasFactory::name() const
{ return Dynamic::PartBias::sName(); }

QString
Dynamic::PartBiasFactory::i18nDescription() const
{ return i18nc("Description of the \"Part\" bias",
                   "The \"Part\" bias adds tracks they match at least one of the sub biases."); }

Dynamic::BiasPtr
Dynamic::PartBiasFactory::createBias()
{ return Dynamic::BiasPtr( new Dynamic::PartBias() ); }

Dynamic::BiasPtr
Dynamic::PartBiasFactory::createBias( QXmlStreamReader *reader )
{ return Dynamic::BiasPtr( new Dynamic::PartBias( reader ) ); }




/* Note:
   We use the Ford-Fulkerson method to compute the maximum match between the bias
   and the tracks.
   The MatchState class will do this matching and keep track of all the needed
   data.
   We are not building up the full graph and we don't even compute in advance
   all the edges.
   */

/** This is the helper object to calculate the maximum match.
    For the sake of the algorithm we are using every sub-bias as a source with
    a capacity depending on it's weight.
    Every track in the playlist is a drain with capacity 1.
*/
class MatchState
{
    public:
        /** Creates the matching
            @param ignoreTrack a track number that should be ignored for matching. -1 if no track should be ignored.
        */
        MatchState( const Dynamic::PartBias *bias,
                    int ignoreTrack,
                    const Meta::TrackList& playlist, int contextCount )
            : m_bias( bias )
            , m_ignoreDrain( ignoreTrack - contextCount )
            , m_playlist( playlist )
            , m_contextCount( contextCount )
            , m_sourceCount( bias->weights().count() )
            , m_drainCount( playlist.count() - contextCount )
            , m_edges( m_sourceCount * m_drainCount, false )
            , m_edgesUsed( m_sourceCount * m_drainCount, false )
            , m_sourceCapacity( m_sourceCount )
            , m_sourceFlow( m_sourceCount )
            , m_drainFlow( m_drainCount )
            , m_drainSource( m_drainCount )
        {
            QList<qreal> weights = m_bias->weights();

            int assignedDrainCount = 0;
            for( int source = 0; source < m_sourceCount-1; source++ )
            {
                m_sourceCapacity[source] = qRound( weights[source] * m_drainCount );
                assignedDrainCount += m_sourceCapacity[source];
                // debug() << "MatchState: bias"<<m_bias->biases()[source]->name()<<"should match"<<m_sourceCapacity[source]<<"of"<< m_drainCount << "tracks.";
            }

            // the last bias get's all the rest
            if( m_sourceCount > 0 )
                m_sourceCapacity[m_sourceCount - 1] = m_drainCount - assignedDrainCount;

            compute();
        }

        void compute()
        {
            // -- initialize the values
            for( int source = m_sourceCount-1; source >= 0; --source )
                m_sourceFlow[source] = 0;

            for( int drain = m_drainCount-1; drain >= 0; --drain )
            {
                m_drainFlow[drain] = 0;
                m_drainSource[drain] = -1;
            }

            // -- get all the edges
            Dynamic::BiasList biases = m_bias->biases();
            for( int source = m_sourceCount-1; source >= 0; --source )
                for( int drain = m_drainCount-1; drain >= 0; --drain )
                {
                    m_edges[ source * m_drainCount + drain ] =
                        biases[source]->trackMatches( drain + m_contextCount,
                                                      m_playlist,
                                                      m_contextCount );
                    m_edgesUsed[ source * m_drainCount + drain ] = false;

                    // debug() << "edge:" << source << "x" << drain << ":" << m_edges[ source * m_drainCount + drain ];
                }

            // find a source whose capacity is not full
            for( int source = m_sourceCount-1; source >= 0; --source )
            {
                if( m_sourceFlow[source] >= m_sourceCapacity[source] )
                    continue;

                for( int drain = 0; drain < m_drainCount; drain++ )
                {
                    if( m_ignoreDrain == drain )
                        continue;
                    if( !m_edges[ source * m_drainCount + drain ] )
                        continue;

                    if( m_drainFlow[drain] < 1 )
                    {
                        // direct connections source to drain
                        // make a new connection
                        m_sourceFlow[source]++;
                        m_drainFlow[drain]++;
                        m_drainSource[drain] = source;
                        m_edgesUsed[ source * m_drainCount + drain ] = true;
                    }
                    else
                    {
                        // indirect connections source to drain to source to drain
                        // or in other words: Check if we can re-order another source
                        // to get a connection for this source
                        int source2 = m_drainSource[drain];

                        for( int drain2 = m_drainCount-1; drain2 >= 0; --drain2 )
                        {
                            if( m_ignoreDrain == drain2 )
                                continue;
                            if( m_drainFlow[drain2] > 0 )
                                continue;
                            if( !m_edgesUsed[ source2 * m_drainCount + drain ] )
                                continue;
                            if( !m_edges[ source2 * m_drainCount + drain2 ] )
                                continue;

                            // revert the old connection
                            m_sourceFlow[source2]--;
                            m_drainFlow[drain]--;
                            m_edgesUsed[ source2 * m_drainCount + drain ] = false;

                            // make two new connections
                            m_sourceFlow[source]++;
                            m_drainFlow[drain]++;
                            m_drainSource[drain] = source;
                            m_edgesUsed[ source * m_drainCount + drain ] = true;

                            m_sourceFlow[source2]++;
                            m_drainFlow[drain2]++;
                            m_drainSource[drain2] = source2;
                            m_edgesUsed[ source2 * m_drainCount + drain2 ] = true;
                            break;
                        }

                    }

                    // finished with this source?
                    if( m_sourceFlow[source] >= m_sourceCapacity[source] )
                        break;
                }
            }
        }


        const Dynamic::PartBias* const m_bias;
        int m_ignoreDrain;
        const Meta::TrackList& m_playlist;
        int m_contextCount;

        int m_sourceCount;
        int m_drainCount;
        QBitArray m_edges;
        QBitArray m_edgesUsed;

        QVector<int> m_sourceCapacity;
        QVector<int> m_sourceFlow;
        QVector<int> m_drainFlow;
        QVector<int> m_drainSource; // the source currently used by the drain
};


Dynamic::PartBias::PartBias()
    : AndBias()
{
    m_duringConstruction = true;

    m_weights.append( 1.0 );

    m_duringConstruction = false;
}

Dynamic::PartBias::PartBias( QXmlStreamReader *reader )
    : AndBias()
{
    m_duringConstruction = true;

    // the weight of the implicite random bias
    m_weights.append( reader->attributes().value("weight").toString().toFloat() );

    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            float weight = reader->attributes().value( "weight" ).toString().toFloat();
            Dynamic::BiasPtr bias( Dynamic::BiasFactory::fromXml( reader ) );
            if( bias )
            {
                appendBias( bias );
                m_weights[ m_weights.count() - 1 ] = weight;
            }
            else
            {
                warning()<<"Unexpected xml start element"<<reader->name()<<"in input";
                reader->skipCurrentElement();
            }
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }

    m_duringConstruction = false;
}

void
Dynamic::PartBias::toXml( QXmlStreamWriter *writer ) const
{
    // the weight of the implicite random bias
    writer->writeAttribute( "weight", QString::number(m_weights[0]) );

    for( int i = 1; i < m_biases.count(); i++ )
    {
        writer->writeStartElement( m_biases[i]->name() );
        writer->writeAttribute( "weight", QString::number(m_weights[i]) );
        m_biases[i]->toXml( writer );
        writer->writeEndElement();
    }
}

QString
Dynamic::PartBias::sName()
{
    return QLatin1String( "partBias" );
}

QString
Dynamic::PartBias::name() const
{
    return Dynamic::PartBias::sName();
}

QWidget*
Dynamic::PartBias::widget( QWidget* parent )
{
    return new PlaylistBrowserNS::LevelBiasWidget( this, true, parent );
}

QList<qreal>
Dynamic::PartBias::weights() const
{
    return m_weights;
}

Dynamic::TrackSet
Dynamic::PartBias::matchingTracks( int position,
                                  const Meta::TrackList& playlist, int contextCount,
                                  Dynamic::TrackCollectionPtr universe ) const
{
    // -- determine the current matching
    MatchState state( this, position, playlist, contextCount );

    debug()<<"matching tracks for"<<position<<"pc"<<playlist.count()<<"context:"<<contextCount;

    // -- find all biases that are not fulfilled
    m_tracks = Dynamic::TrackSet( universe, false );
    m_outstandingMatches = 0;

    // note: the first source is the random bias so we are finished very fast if that happens
    for( int source = 0; source < state.m_sourceCount; source++ )
    {
        // debug() << "PartBias::matchingTracks: biase"<<m_biases[source]->name()<<"only matches"<<state.m_sourceFlow[source]<<"out of"<<state.m_sourceCapacity[source]<<"tracks.";
        if( state.m_sourceFlow[source] < state.m_sourceCapacity[source] )
        {
            Dynamic::TrackSet tracks = m_biases[source]->matchingTracks( position, playlist, contextCount, universe );

            if( tracks.isOutstanding() )
                m_outstandingMatches++;
            else
                m_tracks.unite( tracks );

            if( m_tracks.isFull() )
                break;
        }
    }

    // -- return it's matching tracks
    if( m_outstandingMatches > 0 )
        return Dynamic::TrackSet();
    else
        return m_tracks;
}

bool
Dynamic::PartBias::trackMatches( int position,
                                 const Meta::TrackList& playlist,
                                 int contextCount ) const
{
    MatchState state( this, -1, playlist, contextCount );

    return ( state.m_drainFlow[position - contextCount] >= 0 );
}

double
Dynamic::PartBias::energy( const Meta::TrackList& playlist, int contextCount ) const
{
    MatchState state( this, -1, playlist, contextCount );

    int matchCount = 0;
    for( int i = 0; i < state.m_drainCount; i++ )
    {
        if( state.m_drainFlow[i] )
            matchCount++;
    }

    return 1.0 - (double(matchCount) / (state.m_drainCount));
}

void
Dynamic::PartBias::appendBias( Dynamic::BiasPtr bias )
{
    m_weights.append( qreal(0.0) );
    AndBias::appendBias( bias );
}

void
Dynamic::PartBias::moveBias( int from, int to )
{
    m_weights.insert( to, m_weights.takeAt( from ) );
    AndBias::moveBias( from, to );
}

void
Dynamic::PartBias::changeBiasWeight( int biasNum, qreal value )
{
    Q_ASSERT( biasNum >= 0 && biasNum < m_weights.count() );

    // the weights should sum up to 1.0

    // -- only one weight. that is easy
    if( m_weights.count() == 1 )
    {
        if( m_weights.at(0) != 1.0 )
        {
            m_weights[0] = 1.0;
            emit weightsChanged();
        }
        return;
    }

    // -- more than one. we have to modify the remaining.
    m_weights[biasNum] = qBound( qreal( 0.0 ), value, qreal( 1.0 ) );

    // - sum up all the weights
    qreal sum = 0.0;
    foreach( qreal v, m_weights )
        sum += v;

    // -- we are always using the first value to balance it out if possible
    if( biasNum != 0 )
    {
        qreal oldV = m_weights.at(0);
        qreal newV = qBound( qreal( 0.0 ), 1.0 - (sum - oldV), qreal( 1.0 ) );
        m_weights[0] = newV;

        sum = sum - oldV + newV;
    }

    // modify all the remaining value

    if( sum != 1.0 )
    {
        if( sum - m_weights.at(biasNum) == 0.0 )
        {
            // in this case the entry has all the weight.
            // distribute the remainder to the other weights
            for( int i = 0; i < m_weights.count(); i++ )
                if( i != biasNum )
                    m_weights[i] = sum / (m_weights.count() - 1);

        }
        else
        {
            // in this case we have some remaining weights. use a factor
            qreal factor = (1.0 - m_weights.at(biasNum)) / (sum - m_weights.at(biasNum));
            for( int i = 0; i < m_weights.count(); i++ )
                if( i != biasNum )
                    m_weights[i] *= factor;
        }
    }

    for( int i = 0; i < m_weights.count(); i++ )
        debug() << "Weight"<<i<<":"<<m_weights[i];

    emit weightsChanged();
    if( !m_duringConstruction )
        emit changed( BiasPtr( this ) );
}

void
Dynamic::PartBias::biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias )
{
    int index = m_biases.indexOf( oldBias );
    if( !newBias )
    {
        m_weights.takeAt(index);
        changeBiasWeight( 0, m_weights.at(0) ); // fix the weights to 1.0 again.
    }

    AndBias::biasReplaced( oldBias, newBias );
}

#include "PartBias.moc"

