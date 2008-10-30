/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "Radio.h"
#include "Tuner.h"
#include <QThread>
#include <phonon/mediaobject.h>
#include <phonon/audiooutput.h>
#include <cmath>


Radio::Radio( Phonon::AudioOutput* output )
			   : m_tuner( 0 ),
			     m_audioOutput( output ),
			     m_state( Radio::Stopped )
{
    m_mediaObject = new Phonon::MediaObject( this );
    m_mediaObject->setTickInterval( 1000 );
    connect( m_mediaObject, SIGNAL(stateChanged( Phonon::State, Phonon::State )), SLOT(onPhononStateChanged( Phonon::State, Phonon::State )) );
	connect( m_mediaObject, SIGNAL(currentSourceChanged( const Phonon::MediaSource &)), SLOT(onPhononCurrentSourceChanged( const Phonon::MediaSource &)) );
    Phonon::createPath( m_mediaObject, m_audioOutput );
}


Radio::~Radio()
{
#ifndef WIN32 //for now I'm scared on Windows
	if (m_mediaObject->state() != Phonon::PlayingState)
        return;

    qreal starting_volume = m_audioOutput->volume();
    //sigmoid curve
    for (int x = 18; x >= -60; --x)
    {
        qreal y = x;
        y /= 10;
        y = qreal(1) / (qreal(1) + std::exp( -y ));
        y *= starting_volume;
        m_audioOutput->setVolume( y );

		struct Thread : QThread { using QThread::msleep; };
		Thread::msleep( 7 );
    } 
#endif
}


void
Radio::play( const RadioStation& station )
{
    if (m_state != Stopped)
    {
        //FIXME filthy!
        State oldstate = m_state;
        m_state = Stopped;    //prevents stateChanged() doing anything
        stop();
        clear();
        m_state = oldstate;
    }

	m_station = station;
    
	delete m_tuner;
    m_tuner = new Tuner( station );
	connect( m_tuner, SIGNAL(stationName( QString )), SLOT(setStationNameIfCurrentlyBlank( QString )) );
	connect( m_tuner, SIGNAL(tracks( QList<Track> )), SLOT(enqueue( QList<Track> )) );
	connect( m_tuner, SIGNAL(error( Ws::Error )), SLOT(onTunerError( Ws::Error )) );

    changeState( TuningIn );
}


#include <QThread>
/** my gosh, this is dangerous, FIXME!
  * done because on mac Phonon would hang in enqueu for a few seconds, which
  * sucked */
class EnqueueThread : public QThread
{
	QList<QUrl> m_urls;
	Phonon::MediaObject* m_o;
	
public:
	EnqueueThread( const QList<QUrl>& urls, Phonon::MediaObject* object )
	{
		m_o = object;
		m_urls = urls;
		connect( this, SIGNAL(finished()), SLOT(deleteLater()) );
		start();
	}
	
	virtual void run()
	{
		m_o->enqueue( m_urls );
		m_o->play();
	}
}; 


void
Radio::enqueue( const QList<Track>& tracks )
{   
	if (tracks.isEmpty()) {
		qWarning() << "Received blank playlist, Last.fm is b0rked";
		stop();
		return;
	}
	
    QList<QUrl> urls;
    foreach (const Track& t, tracks)
    {
        urls += t.url();
        m_queue[t.url()] = t;
    }
	
#ifdef Q_WS_MAC
	new EnqueueThread( urls, m_mediaObject );
#else
	m_mediaObject->enqueue( urls );
	m_mediaObject->play();
#endif
}


class SkipThread : public QThread
{
	Phonon::MediaObject* m_mediaObject;
	
public:
	SkipThread( Phonon::MediaObject* object )
	{
		m_mediaObject = object;
		connect( this, SIGNAL(finished()), SLOT(deleteLater()) );
		start();
	}
	
	virtual void run()
	{
		QList<Phonon::MediaSource> q = m_mediaObject->queue();
		Phonon::MediaSource source = q.front();
		q.pop_front();
	
		m_mediaObject->setCurrentSource( source );
		m_mediaObject->setQueue( q );
		m_mediaObject->play();
	}
};


void
Radio::skip()
{
	QList<Phonon::MediaSource> q = m_mediaObject->queue();
	if (q.size())
	{
#ifdef Q_WS_MAC
		new SkipThread( m_mediaObject );
#else
		Phonon::MediaSource source = q.front();
		q.pop_front();
		m_mediaObject->setQueue( q );
		m_mediaObject->setCurrentSource( source );
		m_mediaObject->play();
#endif
	}
	else
	{
		// we are still waiting for a playlist to come back from the tuner
		
		m_mediaObject->blockSignals( true ); //dont' tell outside world that we stopped
		m_mediaObject->stop();
		m_mediaObject->setCurrentSource( QUrl() );
		m_mediaObject->blockSignals( false );
		changeState( TuningIn );
	}
}


void
Radio::onTunerError( Ws::Error e )
{
    // otherwise leave things be, we'll stop when we run out of content
    if (m_state == TuningIn)
		stop();

	emit error( e );
}


void
Radio::stop()
{
    m_mediaObject->blockSignals( true ); //prevent the error state due to setting current source to null
	m_mediaObject->stop();
	m_mediaObject->clearQueue();
	m_mediaObject->setCurrentSource( QUrl() );
    m_mediaObject->blockSignals( false );

    clear();	
    
    changeState( Stopped );
}


void
Radio::clear()
{
    m_queue.clear();
    m_track = Track();
    m_station = RadioStation();
    
    delete m_tuner;
    m_tuner = 0;    
}


void
Radio::onPhononStateChanged( Phonon::State newstate, Phonon::State oldstate )
{
    switch (newstate)
    {
        case Phonon::ErrorState:
			if (m_mediaObject->errorType() == Phonon::FatalError)
			{
				qCritical() << m_mediaObject->errorString();
				stop(); // just in case phonon is broken
			}
			else
				skip();
            break;
			
		case Phonon::PausedState:
			// if the play queue runs out we get this for some reason
			// this means we are fetching new tracks still, we should show a 
			// tuning in state;
			if (m_mediaObject->queue().size() == 0)
                changeState( TuningIn );
			break;
			
        case Phonon::StoppedState:
            // yeah, really, we always ignore it
            // we handle our own stop state, and in all other cases we go
            // to TuningIn
            break;
			
        case Phonon::BufferingState:
            changeState( Buffering );
            break;

		case Phonon::PlayingState:
            if (spoolNextTrack())
                changeState( Playing );
            break;

		case Phonon::LoadingState:
            spoolNextTrack();
			break;
    }
}


void
Radio::onPhononCurrentSourceChanged(const Phonon::MediaSource &)
{
	spoolNextTrack();
}


bool
Radio::spoolNextTrack()
{
    QUrl const url = m_mediaObject->currentSource().url();
    
    if (url == m_track.url())
        return true;

    Track t = m_queue.take( url );
    if (t.isNull())
    {
        qWarning() << "Cannot find track for" << url;
        return false;
    }
    
    MutableTrack( t ).stamp();
    m_track = t;
    
    if (m_queue.isEmpty() && m_tuner)
        m_tuner->fetchFiveMoreTracks();
    
    changeState( Buffering );
    emit trackSpooled( m_track );
    return true;
}


void
Radio::changeState( Radio::State const newstate )
{
	State const oldstate = m_state;

    if (oldstate == newstate) 
        return;

  qDebug().nospace() << newstate << " (was " << oldstate << ')';
     
    m_state = newstate; // always assign state properties before you tell other
                        // objects about it
    
	switch (newstate)
	{
        case TuningIn:
            qDebug() << "Tuning to:" << m_station;
            emit tuningIn( m_station );
            break;
            
        case Buffering:
            break;
            
		case Playing:
            emit trackStarted( m_track );
            break;

		case Stopped:
            emit stopped();
            break;
	}
}


void
Radio::setStationNameIfCurrentlyBlank( const QString& s )
{
    if (m_station.title().isEmpty())
    {
        m_station.setTitle( s );
        emit tuningIn( m_station );
    }
}
