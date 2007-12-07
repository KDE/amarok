/***************************************************************************
 *   Copyright (C) 2007 Shane King <kde@dontletsstart.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "phonon-directshow"

#include "DirectShowGraph.h"
#include "DirectShowAudioOutput.h"
#include "DirectShowBackend.h"
#include "DirectShowMediaObject.h"

#include "debug.h"

#include <KLocale>

#include <complex>
#include <evcode.h>

DirectShowGraph::DirectShowGraph( DirectShowBackend *backend, DirectShowMediaObject *mediaObject, DirectShowAudioOutput *audioOutput )
    : QObject( backend ),
      m_backend( backend ),
      m_mediaObject( mediaObject ),
      m_audioOutput( audioOutput ),
      m_errorType( Phonon::NoError ),
      m_state( Phonon::StoppedState )
{
    m_initialized = createGraph();
    if( m_initialized )
    {
        m_mediaObject->setGraph( this );

        // output communicates with graph through signals
        connect( m_audioOutput, SIGNAL( volumeChanged(qreal) ), SLOT( volumeChanged(qreal) ) );
        connect( m_audioOutput, SIGNAL( outputDeviceChanged(int) ), SLOT( outputDeviceChanged(int) ) );

        volumeChanged( m_audioOutput->volume() );
        outputDeviceChanged( m_audioOutput->outputDevice() );
    }
    else
    {
        error( Phonon::FatalError, i18n( "Initialization failed" ) );
    }
}


DirectShowGraph::~DirectShowGraph()
{
    // need to turn off event manually
    if( m_event )
        m_event->SetNotifyWindow( 0, 0, 0 );

    // and remove ourself from the media object (deletion will auto detach us from the audio output signals)
    if( m_mediaObject )
        m_mediaObject->setGraph( 0 );
}


bool 
DirectShowGraph::onEvent()
{
    long evCode;
    long param1;
    long param2;

	while( SUCCEEDED( m_event->GetEvent( &evCode, &param1, &param2, 0 ) ) )
	{
        switch( evCode )
        {
            // only care about completion of track at this point
            case EC_COMPLETE:
                emit finished();
                break;
        }

        // need to free event data after all events
		HResult hr = m_event->FreeEventParams(evCode, param1, param2);
		if (FAILED(hr))
		{
            debug() << "Failed to free event: " << hr;
			return false;
		}
	}
    return true;
}


void
DirectShowGraph::stop()
{
    HResult hr = m_control->Stop();
    if( SUCCEEDED( hr ) )
    {
        seek( 0 ); // go back to start, otherwise play will resume at same place
        setState( Phonon::StoppedState );
    }
    else
    {
        debug() << "Failed to stop graph: " << hr;
        error( Phonon::NormalError, i18n( "Failed to stop" ) );
    }
}


void
DirectShowGraph::pause()
{
    HResult hr = m_control->Pause();
    if( SUCCEEDED( hr ) )
    {
        setState( Phonon::PausedState );
    }
    else
    {
        debug() << "Failed to pause graph: " << hr;
        error( Phonon::NormalError, i18n( "Failed to pause" ) );
    }
}


void
DirectShowGraph::play()
{
    if( !m_input || !m_output )
    {
        error( Phonon::NormalError, i18n( "Nothing to play" ) );
    }
    else
    {
        HResult hr = m_control->Run();
        if( SUCCEEDED( hr ) )
        {
            setState( Phonon::PlayingState );
        }
        else
        {
            debug() << "Failed to play graph: " << hr;
            error( Phonon::NormalError, i18n( "Failed to play" ) );
        }
    }
}


void
DirectShowGraph::seek( qint64 milliseconds )
{
    qint64 pos = milliseconds * 10000; // DirectShow uses 100 nanosecond units
    HResult hr = m_seek->SetPositions( &pos, AM_SEEKING_AbsolutePositioning, 0, AM_SEEKING_NoPositioning );
    if( FAILED( hr ) )
        debug() << "Failed to seek: " << hr;
}


bool
DirectShowGraph::isSeekable() const
{
    DWORD capabilities = AM_SEEKING_CanSeekAbsolute;
    return m_seek->CheckCapabilities( &capabilities ) == S_OK;
}

qint64 
DirectShowGraph::currentTime() const
{
    qint64 result = 0;
    HResult hr = m_seek->GetCurrentPosition( &result );
    if( FAILED( hr ) )
        debug() << "Failed to get current position: " << hr;
    return result / 10000; // DirectShow uses 100 nanosecond units
}


qint64 
DirectShowGraph::totalTime() const
{
    qint64 result = 0;
    HResult hr = m_seek->GetDuration( &result );
    if( FAILED( hr ) )
        debug() << "Failed to get total time: " << hr;
    return result / 10000; // DirectShow uses 100 nanosecond units
}


Phonon::State 
DirectShowGraph::state() const
{
    return m_state;
}


QString 
DirectShowGraph::errorString() const
{
    return "";
}


Phonon::ErrorType 
DirectShowGraph::errorType() const
{
    return m_errorType;
}


Phonon::MediaSource 
DirectShowGraph::source() const
{
    return m_source;
}

void 
DirectShowGraph::volumeChanged( qreal volume )
{
    const long VOLUME_RANGE = 10000; // DirectShow using -10000 to 0 for volume, equivalent to -100db to 0db gain

    // convert from what phonon gives us
    // and again to get it into something reasonable for the way DirectShow handles volume
    long dsVolume = long( std::pow( volume, 1 / ( 1.4925373 * 1.4925373 ) ) * VOLUME_RANGE ) - VOLUME_RANGE;
    
    m_audio->put_Volume( dsVolume );
}


void 
DirectShowGraph::outputDeviceChanged( int device )
{
    stop(); // can't do this while playing

    // remove existing device if present
    HResult hr;
    if( m_output )
    {
        hr = m_graph->RemoveFilter( m_output );
        if( FAILED( hr ) )
            debug() << "Failed to remove filter: " << hr;
    }

    // add new device
    ComPtr<IMoniker> moniker = m_backend->getDevice( device );
    hr = moniker->BindToObject( 0, 0, IID_IBaseFilter, reinterpret_cast<void **>( &m_output ) );
    if( SUCCEEDED( hr ) )
    {
        hr = m_graph->AddFilter( m_output, L"Output" );
        if( SUCCEEDED( hr ) )
        {
            render();
            return;
        }
        else
        {
            debug() << "Failed to add filter: " << hr;
        }
    }
    else
    {
        debug() << "Failed to get device filter: " << hr;
    }
    error( Phonon::FatalError, i18n( "Could not use device" ) );
}


void 
DirectShowGraph::setSource( const Phonon::MediaSource &source )
{
    // only handle local files for now
    QString file;
    if( source.type() == Phonon::MediaSource::LocalFile )
    {
        file = source.fileName();
    }
    else if ( source.type() == Phonon::MediaSource::Url && source.url().scheme() == "file" )
    {
        file = source.url().path();
    }

    if( file != "" )
    {
        debug() << "Setting source to: " << file;
        
        // remove existing input file if present
        HResult hr;
        if( m_input )
        {
            hr = m_graph->RemoveFilter( m_input );
            if( SUCCEEDED( hr ) )
                debug() << "Failed to remove filter: " << hr;
        }

        // add new file
        hr = m_graph->AddSourceFilter( reinterpret_cast<const wchar_t *>( file.utf16() ), L"Input", &m_input );
        if( SUCCEEDED( hr ) )
        {
            m_source = source;
            newSource();
            return;
        }
        else
        {
            debug() << "Failed to set source: " << hr;
            error( Phonon::NormalError, i18n( "Failed to set source" ) );
        }
    }
    else
    {
        debug() << "Can't handle source type: " << source.type();
        error( Phonon::NormalError, i18n( "Unable to handle source" ) );
    }
    m_source = Phonon::MediaSource();
}


void
DirectShowGraph::newSource()
{
    m_errorType = Phonon::NoError;
    m_errorString = "";
    render();

    emit currentSourceChanged( m_source );
    emit seekableChanged( isSeekable() ); // may not have "changed" but this is good enough
    emit totalTimeChanged( totalTime() );
}


void 
DirectShowGraph::error( Phonon::ErrorType type, const QString &string )
{
    debug() << "Error: " << string;
    m_errorType = type;
    m_errorString = string;
    setState( Phonon::ErrorState );
}


void 
DirectShowGraph::setState( Phonon::State state )
{
    if( m_state != state )
    {
        debug() << "State change: " << m_state << " -> " << state;
        Phonon::State old = m_state;
        m_state = state;
        emit stateChanged( state, old );
    }
}


// setup the initial filter graph and get required interfaces
bool
DirectShowGraph::createGraph()
{
    HResult hr;

    hr = m_graph.CreateInstance( CLSID_FilterGraph, IID_IGraphBuilder );
    if( SUCCEEDED( hr ) )
    {
        hr = m_graph.QueryInterface( IID_IMediaControl, m_control );
        if( SUCCEEDED( hr ) )
        {
            hr = m_graph.QueryInterface( IID_IMediaEventEx, m_event );
            if( SUCCEEDED( hr ) )
            {
                hr = m_graph.QueryInterface( IID_IMediaSeeking, m_seek );
                if( SUCCEEDED( hr ) )
                {
                    hr = m_graph.QueryInterface( IID_IBasicAudio, m_audio );
                    if( SUCCEEDED( hr ) )
                    {
                        hr = m_event->SetNotifyWindow( reinterpret_cast<OAHWND>( m_backend->window() ), DirectShowBackend::WM_GRAPH_EVENT, reinterpret_cast<LPARAM>( this ) );
                        if( SUCCEEDED( hr ) )
                            return true;
                        else
                            debug() << "Failed to connect notification event: " << hr;
                    }
                    else
                    {
                        debug() << "Failed to get basic audio interface: " << hr;
                    }
                }
                else
                {
                    debug() << "Failed to get media seeking interface: " << hr;
                }
            }
            else
            {
                debug() << "Failed to get media event interface: " << hr;
            }
        }
        else
        {
            debug() << "Failed to get media control interface: " << hr;
        }
    }
    else
    {
        debug() << "Failed to create graph builder: " << hr;
    }
    return false;
}


// connect input pin to output
void
DirectShowGraph::render()
{
    setState( Phonon::StoppedState );

    if( m_input && m_output )
    {
        ComPtr<IFilterGraph2> graph2;
        HResult hr = m_graph.QueryInterface( IID_IFilterGraph2, graph2 );
        if( SUCCEEDED( hr ) )
        {
            ComPtr<IEnumPins> pins;
            hr = m_input->EnumPins( &pins );
            if( SUCCEEDED( hr ) )
            {
                bool success = false;
	            ComPtr<IPin> pin;
	            while( pins->Next( 1, &pin, 0 ) == S_OK )
                {
                    hr = graph2->RenderEx( pin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, 0 );
                    if( SUCCEEDED( hr ) )
                        success = true;
                }

                if( !success )
                {
                    debug() << "Couldn't render to any pins";
                    error( Phonon::FatalError, i18n( "No playback devices" ) );
                }
            }
            else
            {
                debug() << "Failed to enumerate pins: " << hr;
                error( Phonon::FatalError, i18n( "Failed to setup playback" ) );
            }
        }
        else
        {
            debug() << "Failed to get graph2 interface: " << hr;
            error( Phonon::FatalError, i18n( "Failed to setup playback" ) );
        }
    }
}
