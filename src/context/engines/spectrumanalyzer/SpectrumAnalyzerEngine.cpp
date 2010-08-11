/****************************************************************************************
 * Copyright (c) 2010 Daniel Dewald <Daniel.Dewald@time-shift.de>                       *
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

#include "SpectrumAnalyzerEngine.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "ContextObserver.h"
#include "ContextView.h"

#include <QVariant>

using namespace Context;

SpectrumAnalyzerEngine::SpectrumAnalyzerEngine( QObject* parent, const QList<QVariant>& args )
    : DataEngine( parent )
    , m_requested( true )
{
    Q_UNUSED( args )
    DEBUG_BLOCK

    m_audioData.clear();
    this->connect( The::engineController(), SIGNAL(audioDataReady(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> >&)), this, SLOT(receiveData(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> >&)));
    
    m_sources = QStringList();
    m_sources << "audioData";
    update();
}

SpectrumAnalyzerEngine::~ SpectrumAnalyzerEngine()
{
    this->disconnect( The::engineController(), NULL, this, NULL );
    m_audioData.clear();
}

QStringList
SpectrumAnalyzerEngine::sources() const
{
    return m_sources;
}

bool
SpectrumAnalyzerEngine::sourceRequestEvent( const QString& name )
{
    m_requested = true;
    QStringList tokens = name.split( ':' );

    if ( tokens.contains( "stopped" ) && tokens.size() > 1 )
    {
        if ( tokens.at( 1 ) == QString( "stopped" ) )
        {
            removeSource( "spectrum-analyzer" );
            return false;
        }
    }
    
    setData( name, QVariant() );
    update();
    return m_requested;
}

void
SpectrumAnalyzerEngine::message( const ContextState& state )
{
    if ( state == Current && m_requested )
        update();
}

void
SpectrumAnalyzerEngine::metadataChanged( Meta::TrackPtr track )
{
    m_dataHasChanged = ( track != m_currentTrack );

    if ( m_dataHasChanged )
        update();
}

void
SpectrumAnalyzerEngine::receiveData( const QMap<Phonon::AudioDataOutput::Channel,QVector<qint16> > &data )
{
    m_audioData = data;
    m_dataHasChanged = true;
    update();
}

void
SpectrumAnalyzerEngine::update()
{
    if ( !m_dataHasChanged )
    {
        return;
    }
    else
    {
        Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
        m_dataHasChanged = false;

        removeData( "spectrum-analyzer", "message" );
        removeData( "spectrum-analyzer", "artist" );
        removeData( "spectrum-analyzer", "title" );
        removeData( "spectrum-analyzer", "data_left" );
        removeData( "spectrum-analyzer", "data_right" );
        removeData( "spectrum-analyzer", "data_center" );
        removeData( "spectrum-analyzer", "data_lefts" );
        removeData( "spectrum-analyzer", "data_rights" );
        removeData( "spectrum-analyzer", "data_sub" );
        
        if ( m_currentTrack != currentTrack )
        {
            m_audioData.clear();
            m_currentTrack = currentTrack;
            setData( "spectrum-analyzer", "message", "clear");

            if ( !m_currentTrack.isNull() )
            {
                setData( "spectrum-analyzer", "artist", m_currentTrack->artist()->name() );
                setData( "spectrum-analyzer", "title", m_currentTrack->name() );
            }
        }

        if ( m_audioData.count() > 0 )
        {
            if ( m_audioData.contains( Phonon::AudioDataOutput::LeftChannel ) && ( m_audioData.value( Phonon::AudioDataOutput::LeftChannel ).count() > 0 ) )
            {
                QVariant var;
                var.setValue< QVector< qint16 > > ( m_audioData.value( Phonon::AudioDataOutput::LeftChannel ) );
                setData( "spectrum-analyzer", "data_left", var );
            }

            if ( m_audioData.contains( Phonon::AudioDataOutput::RightChannel ) && ( m_audioData.value( Phonon::AudioDataOutput::RightChannel ).count() > 0 ) )
            {
                QVariant var;
                var.setValue< QVector< qint16 > > ( m_audioData.value( Phonon::AudioDataOutput::RightChannel ) );
                setData( "spectrum-analyzer", "data_right", var );
            }

            if ( m_audioData.contains( Phonon::AudioDataOutput::CenterChannel ) && ( m_audioData.value( Phonon::AudioDataOutput::CenterChannel ).count() > 0 ) )
            {
                QVariant var;
                var.setValue< QVector< qint16 > > ( m_audioData.value( Phonon::AudioDataOutput::CenterChannel ) );
                setData( "spectrum-analyzer", "data_center", var );
            }

            if ( m_audioData.contains( Phonon::AudioDataOutput::LeftSurroundChannel ) && ( m_audioData.value( Phonon::AudioDataOutput::LeftSurroundChannel ).count() > 0 ) )
            {
                QVariant var;
                var.setValue< QVector< qint16 > > ( m_audioData.value( Phonon::AudioDataOutput::LeftSurroundChannel ) );
                setData( "spectrum-analyzer", "data_lefts", var );
            }

            if ( m_audioData.contains( Phonon::AudioDataOutput::RightSurroundChannel ) && ( m_audioData.value( Phonon::AudioDataOutput::RightSurroundChannel ).count() > 0 ) )
            {
                QVariant var;
                var.setValue< QVector< qint16 > > ( m_audioData.value( Phonon::AudioDataOutput::RightSurroundChannel ) );
                setData( "spectrum-analyzer", "data_rights", var );
            }

            if ( m_audioData.contains( Phonon::AudioDataOutput::SubwooferChannel ) && ( m_audioData.value( Phonon::AudioDataOutput::SubwooferChannel ).count() > 0 ) )
            {
                QVariant var;
                var.setValue< QVector< qint16 > > ( m_audioData.value( Phonon::AudioDataOutput::SubwooferChannel ) );
                setData( "spectrum-analyzer", "data_sub", var );
            }
        }
    }
}

#include "SpectrumAnalyzerEngine.moc"


