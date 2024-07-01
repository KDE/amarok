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

#define DEBUG_PREFIX "PartBias"

#include "PartBias.h"

#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "widgets/SliderWidget.h"

#include <KLocalizedString>

#include <QtGlobal> // for qRound
#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QRandomGenerator>
#include <QSlider>
#include <QStyle>
#include <QStyleOption>
#include <QWidget>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

QString
Dynamic::PartBiasFactory::i18nName() const
{ return i18nc("Name of the \"Part\" bias", "Partition"); }

QString
Dynamic::PartBiasFactory::name() const
{ return Dynamic::PartBias::sName(); }

QString
Dynamic::PartBiasFactory::i18nDescription() const
{ return i18nc("Description of the \"Part\" bias",
                   "The \"Part\" bias fills parts of the playlist from different sub-biases."); }

Dynamic::BiasPtr
Dynamic::PartBiasFactory::createBias()
{ return Dynamic::BiasPtr( new Dynamic::PartBias() ); }



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
        */
        MatchState( const Dynamic::PartBias *bias,
                    const Meta::TrackList& playlist,
                    int contextCount, int finalCount )
            : m_bias( bias )
            , m_playlist( playlist )
            , m_contextCount( contextCount )
            , m_sourceCount( bias->weights().count() )
            , m_drainCount( finalCount - contextCount )
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
                    m_edgesUsed[ source * m_drainCount + drain ] = false;

                    if( drain + m_contextCount >= m_playlist.count() )
                        continue;

                    m_edges[ source * m_drainCount + drain ] =
                        biases[source]->trackMatches( drain + m_contextCount,
                                                      m_playlist,
                                                      m_contextCount );
                    // debug() << "edge:" << source << "x" << drain << ":" << m_edges[ source * m_drainCount + drain ];
                }

            // find a source whose capacity is not full
            for( int source = m_sourceCount-1; source >= 0; --source )
            {
                if( m_sourceFlow[source] >= m_sourceCapacity[source] )
                    continue;

                for( int drain = 0; drain < m_drainCount; drain++ )
                {
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

// -------- PartBiasWidget -----------

Dynamic::PartBiasWidget::PartBiasWidget( Dynamic::PartBias* bias, QWidget* parent )
    : QWidget( parent )
    , m_inSignal( false )
    , m_bias( bias )
{
    connect( bias, &PartBias::biasAppended,
             this, &PartBiasWidget::biasAppended );

    connect( bias, &PartBias::biasRemoved,
             this, &PartBiasWidget::biasRemoved );

    connect( bias, &PartBias::biasMoved,
             this, &PartBiasWidget::biasMoved );

    connect( bias, &PartBias::weightsChanged,
             this, &PartBiasWidget::biasWeightsChanged );

    m_layout = new QGridLayout( this );

    // -- add all sub-bias widgets
    for( Dynamic::BiasPtr bias : m_bias->biases() )
    {
        biasAppended( bias );
    }
}

void
Dynamic::PartBiasWidget::biasAppended( Dynamic::BiasPtr bias )
{
    int index = m_bias->biases().indexOf( bias );

    Amarok::Slider* slider = nullptr;
    slider = new Amarok::Slider( Qt::Horizontal, 100 );
    slider->setValue( m_bias->weights()[ m_bias->biases().indexOf( bias ) ] * 100.0 );
    slider->setToolTip( i18n( "This controls what portion of the playlist should match the criteria" ) );
    connect( slider, &Amarok::Slider::valueChanged, this, &PartBiasWidget::sliderValueChanged );

    QLabel* label = new QLabel( bias->toString() );

    m_sliders.append( slider );
    m_widgets.append( label );
    // -- add the widget (with slider)
    m_layout->addWidget( slider, index, 0 );
    m_layout->addWidget( label, index, 1 );
}

void
Dynamic::PartBiasWidget::biasRemoved( int pos )
{
    m_layout->takeAt( pos * 2 );
    m_layout->takeAt( pos * 2 );
    m_sliders.takeAt( pos )->deleteLater();
    m_widgets.takeAt( pos )->deleteLater();
}

void
Dynamic::PartBiasWidget::biasMoved( int from, int to )
{
    QSlider* slider = m_sliders.takeAt( from );
    m_sliders.insert( to, slider );

    QWidget* widget = m_widgets.takeAt( from );
    m_widgets.insert( to, widget );

    // -- move the item in the layout
    // TODO
    /*
    m_layout->insertWidget( to * 2, slider );
    m_layout->insertWidget( to * 2 + 1, widget );
    */
}

void
Dynamic::PartBiasWidget::sliderValueChanged( int val )
{
    DEBUG_BLOCK;
    // protect against recursion
    if( m_inSignal )
        return;

    for( int i = 0; i < m_sliders.count(); i++ )
    {
        if( m_sliders.at(i) == sender() )
            m_bias->changeBiasWeight( i, qreal(val) / 100.0 );
    }
}

void
Dynamic::PartBiasWidget::biasWeightsChanged()
{
    DEBUG_BLOCK;
    // protect against recursion
    if( m_inSignal )
        return;

    m_inSignal = true;

    QList<qreal> weights = m_bias->weights();
    for( int i = 0; i < weights.count() && i < m_sliders.count(); i++ )
        m_sliders.at(i)->setValue( weights.at(i) * 100.0 );

    m_inSignal = false;
}



// ----------- PartBias ----------------

Dynamic::PartBias::PartBias()
    : AndBias()
{
    // add weights for already existing biases
    for( int i = 0; i < biases().count(); i++ )
        m_weights.append( 1.0 / biases().count() );
}

void
Dynamic::PartBias::fromXml( QXmlStreamReader *reader )
{
    QList<qreal> weights; // we have to add all biases before we can set their weights

    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            float weight = reader->attributes().value( QStringLiteral("weight") ).toString().toFloat();
            Dynamic::BiasPtr bias( Dynamic::BiasFactory::fromXml( reader ) );
            if( bias )
            {
                appendBias( bias );
                weights.append( weight );
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

    m_weights = weights;
}

void
Dynamic::PartBias::toXml( QXmlStreamWriter *writer ) const
{
    for( int i = 0; i < m_biases.count(); i++ )
    {
        writer->writeStartElement( m_biases[i]->name() );
        writer->writeAttribute( QStringLiteral("weight"), QString::number(m_weights[i]) );
        m_biases[i]->toXml( writer );
        writer->writeEndElement();
    }
}

QString
Dynamic::PartBias::sName()
{
    return QStringLiteral( "partBias" );
}

QString
Dynamic::PartBias::name() const
{
    return Dynamic::PartBias::sName();
}

QString
Dynamic::PartBias::toString() const
{
    return i18nc("Part bias representation", "Partition");
}


QWidget*
Dynamic::PartBias::widget( QWidget* parent )
{
    return new Dynamic::PartBiasWidget( this, parent );
}

void
Dynamic::PartBias::paintOperator( QPainter* painter, const QRect& rect, Dynamic::AbstractBias* bias )
{
    int index = m_biases.indexOf( Dynamic::BiasPtr(bias) );
    if( index < 0 )
        return;

    QStyleOptionProgressBar progressBarOption;
    progressBarOption.rect = rect.adjusted( 2, 2, -2, -2 );
    progressBarOption.minimum = 0;
    progressBarOption.maximum = 100;
    progressBarOption.progress = m_weights[index] * 100.0;

    QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);
}

QList<qreal>
Dynamic::PartBias::weights() const
{
    return m_weights;
}

Dynamic::TrackSet
Dynamic::PartBias::matchingTracks( const Meta::TrackList& playlist,
                                   int contextCount, int finalCount,
                                   const Dynamic::TrackCollectionPtr &universe ) const
{
    DEBUG_BLOCK;

    // store the parameters in case we need to request additional matching tracks later
    m_playlist = playlist;
    m_contextCount = contextCount;
    m_finalCount = finalCount;
    m_universe = universe;

    m_tracks = Dynamic::TrackSet();
    m_matchingTracks.resize( m_biases.length() );

    // get the matching tracks from all sub-biases
    for( int i = 0; i < m_biases.length(); ++i )
        m_matchingTracks[i] = m_biases[i]->matchingTracks( playlist, contextCount, finalCount, universe );
    updateResults();

    return m_tracks;
}

void
Dynamic::PartBias::updateResults() const
{
    DEBUG_BLOCK;

    // -- first check if we have valid tracks from all sub-biases
    for( const Dynamic::TrackSet &tracks : m_matchingTracks )
        if( tracks.isOutstanding() )
            return;

    // -- determine the current matching
    MatchState state( this, m_playlist, m_contextCount, m_finalCount );

    debug()<<"compute matching tracks for"<<m_finalCount<<"pc"<<m_playlist.count()<<"context:"<<m_contextCount;

    // -- add all the tracks from one bias that has not fulfilled their capacity
    //    biases still missing more tracks are picked more often
    //    this prevents the problem that biases with only a few tracks always add their tracks
    //    last
    int missingCapacity = 0;
    for( int source = 0; source < state.m_sourceCount; source++ )
    {
        if( state.m_sourceFlow[source] < state.m_sourceCapacity[source] &&
            m_matchingTracks[source].trackCount() > 0 )
            missingCapacity += state.m_sourceCapacity[source] - state.m_sourceFlow[source];
    }

    m_tracks = Dynamic::TrackSet( m_universe, false );

    // if we have some biases under-represented
    if( missingCapacity > 0 )
    {
        int random = QRandomGenerator::global()->generate() % missingCapacity;
        for( int source = 0; source < state.m_sourceCount; source++ )
        {
            // debug() << "PartBias::matchingTracks: biase"<<m_biases[source]->toString()<<"matches"<<state.m_sourceFlow[source]<<"out of"<<state.m_sourceCapacity[source]<<"tracks.";
            if( state.m_sourceFlow[source] < state.m_sourceCapacity[source] &&
                m_matchingTracks[source].trackCount() > 0 )
                random -= state.m_sourceCapacity[source] - state.m_sourceFlow[source];
            if( random < 0 )
            {
                m_tracks.unite( m_matchingTracks[source] );
                break;
            }
        }
    }

    // else pick a random one.
}

void
Dynamic::PartBias::resultReceived( const Dynamic::TrackSet &tracks )
{
    int index = m_biases.indexOf(Dynamic::BiasPtr(qobject_cast<Dynamic::AbstractBias*>(sender())));
    if( index < 0 ) {
        warning() << "Got results from a bias that I don't have.";
        return;
    }
    if( !m_tracks.isOutstanding() ) {
        warning() << "currently in resultReceived but we already have a solution";
        return;
    }

    m_matchingTracks[index] = tracks;
    updateResults();

    if( !m_tracks.isOutstanding() )
        Q_EMIT resultReady( m_tracks );
}

bool
Dynamic::PartBias::trackMatches( int position,
                                 const Meta::TrackList& playlist,
                                 int contextCount ) const
{
    MatchState state( this, playlist, contextCount, playlist.count() );

    return ( state.m_drainFlow[position - contextCount] >= 0 );
}

void
Dynamic::PartBias::appendBias( const Dynamic::BiasPtr &bias )
{
    DEBUG_BLOCK;
    m_weights.append( qreal(0.0) );
    changeBiasWeight( 0, m_weights.at(0) ); // fix the weights to 1.0 again.
    AndBias::appendBias( bias );
}

void
Dynamic::PartBias::moveBias( int from, int to )
{
    DEBUG_BLOCK;
    m_weights.insert( to, m_weights.takeAt( from ) );
    AndBias::moveBias( from, to );
}

void
Dynamic::PartBias::changeBiasWeight( int biasNum, qreal value )
{
    DEBUG_BLOCK;
    Q_ASSERT( biasNum >= 0 && biasNum < m_weights.count() );

    // the weights should sum up to 1.0

    // -- only one weight. that is easy
    if( m_weights.count() == 1 )
    {
        if( m_weights.at(0) != 1.0 )
        {
            m_weights[0] = 1.0;
            Q_EMIT weightsChanged();
        }
        return;
    }

    // -- more than one. we have to modify the remaining.
    m_weights[biasNum] = qBound( qreal( 0.0 ), value, qreal( 1.0 ) );

    // - sum up all the weights
    qreal sum = 0.0;
    for( qreal v : m_weights )
        sum += v;

    // -- we are always using the first value to balance it out if possible
    if( biasNum != 0 )
    {
        qreal oldV = m_weights.at(0);
        qreal newV = qBound<qreal>( qreal( 0.0 ), 1.0 - (sum - oldV), qreal( 1.0 ) );
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

    Q_EMIT weightsChanged();
    Q_EMIT changed( BiasPtr( this ) );
}

void
Dynamic::PartBias::biasReplaced( const Dynamic::BiasPtr &oldBias, const Dynamic::BiasPtr &newBias )
{
    DEBUG_BLOCK;
    int index = m_biases.indexOf( oldBias );
    if( !newBias )
    {
        m_weights.takeAt( index );
        if( !m_weights.isEmpty() )
            changeBiasWeight( 0, m_weights.at(0) ); // fix the weights to 1.0 again.
    }

    AndBias::biasReplaced( oldBias, newBias );
}


