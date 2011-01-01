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
#include "BiasFactory.h"

#include "core/support/Debug.h"
#include "DynamicBiasWidgets.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>


/* Note:
   We use the Ford-Fulkerson method to compute the maximum match between the bias
   and the tracks.
   The MatchState class will do this matching and keep track of all the needed
   data.
   We are not building up the full graph and we don't even compute in advance
   all the edges.
   */

class MatchState
{
    public:
        MatchState( Dynamic::PartBias *bias,
                    const Meta::TrackList& playlist, int contextCount )
            : m_bias( bias )
            , m_playlist( playlist )
            , m_contextCount( contextCount )
            , m_sourceCount( bias->weights().count() )
            , m_drainCount( playlist.count() - contextCount )
            , m_edges( m_sourceCount * m_drainCount, false )
            , m_knownEdges( m_sourceCount * m_drainCount, false )
            , m_sourceCapacity( m_sourceCount )
            , m_sourceFlow( m_sourceCount )
        {
            QList<qreal> weights = m_bias->weights();
            for( int i = 0; i < m_sourceCount; i++ )
            {
                m_sourceCapacity[i] = qround( weights[i] * m_sourceCount );
                m_sourceFlow[i] = 0;
            }
        }

        void computeMatching()
        {
            bool finished = true;
            do
            {
                // find a source whose capacity is not full and whose edges are
                // not already fully known

            } while( !finished );
        }

        QVector<int> flows()
        {
            return m_sourceFlow;
        }


    private:
        Dynamic::PartBias *m_bias;
        const Meta::TrackList& m_playlist;
        int m_contextCount;

        int m_sourceCount;
        int m_drainCount;
        QBitArray m_edges;
        QBitArray m_knownEdges; // true if we have already called matchingTracks

        QVector<int> m_sourceCapacity;
        QVector<int> m_sourceFlow;
};


Dynamic::PartBias::PartBias()
    : AndBias()
{
    m_weights.append( 1.0 );
}

Dynamic::PartBias::PartBias( QXmlStreamReader *reader )
    : AndBias()
{
    // the weight of the implicite random bias
    m_weights.append( reader->attributes().value("weight").toString().toFloat() );

    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            Dynamic::BiasPtr bias( Dynamic::BiasFactory::fromXml( reader ) );
            if( bias )
            {
                appendBias( bias );
                m_weights[ m_weights.count() - 1 ] = reader->attributes().value( "weight" ).toString().toFloat();
            }
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
Dynamic::PartBias::weights()
{
    return m_weights;
}

Dynamic::TrackSet
Dynamic::PartBias::matchingTracks( int position,
                                  const Meta::TrackList& playlist, int contextCount,
                                  Dynamic::TrackCollectionPtr universe ) const
{
    m_tracks = Dynamic::TrackSet( universe, true );
    m_outstandingMatches = 0;

    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        Dynamic::TrackSet tracks = bias->matchingTracks( position, playlist, contextCount, universe );
        if( tracks.isOutstanding() )
            m_outstandingMatches++;
        else
            m_tracks.intersect( tracks );

        if( m_tracks.isEmpty() )
            break;
    }

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
    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        if( !bias->trackMatches( position, playlist, contextCount ) )
            return false;
    }
    return true;
}

double
Dynamic::PartBias::energy( const Meta::TrackList& playlist, int contextCount ) const
{
    double result = 0.0;

    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        result = qMax( result, bias->energy( playlist, contextCount ) );
    }
    return result;
}

void
Dynamic::PartBias::appendBias( Dynamic::BiasPtr bias )
{
    AndBias::appendBias( bias );
    m_weights.append( qreal(0.0) );
}

void
Dynamic::PartBias::moveBias( int from, int to )
{
    AndBias::moveBias( from, to );
    m_weights.insert( to, m_weights.takeAt( from ) );
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

    emit weightsChanged();
    return;
}

void
Dynamic::PartBias::biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias )
{
    int index = m_biases.indexOf( oldBias );
    AndBias::biasReplaced( oldBias, newBias );

    if( !newBias )
    {
        m_weights.takeAt(index);
        changeBiasWeight( 0, m_weights.at(0) ); // fix the weights to 1.0 again.
    }
}

#include "PartBias.moc"

