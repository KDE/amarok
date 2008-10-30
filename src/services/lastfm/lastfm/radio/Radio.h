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

#ifndef LASTFM_RADIO_H
#define LASTFM_RADIO_H

#include <lastfm/DllExportMacro.h>
#include <lastfm/types/Track.h>
#include <lastfm/radio/RadioStation.h>
#include <lastfm/ws/WsError.h>
#include <phonon/phononnamespace.h>
#include <QList>
#include <QObject>

namespace Phonon
{
	class MediaObject;
	class AudioOutput;
	class MediaSource;
}
class QAction;


/** @author <max@last.fm>
 */
class LASTFM_RADIO_DLLEXPORT Radio : public QObject
{
    Q_OBJECT

public:
    Radio( Phonon::AudioOutput* );
    ~Radio();
	
	enum State
	{
        Stopped,
		TuningIn,
        Buffering,
		Playing
	};

	/** we own these as we control whether or not they are enabled */
	QAction* stopAction();
	QAction* skipAction();
	
	State state() const { return m_state; }
	RadioStation station() const { return m_station; }
	Track track() const { return m_track; }

	Phonon::AudioOutput* audioOutput() const { return m_audioOutput; }
	/** provided for giggles, but it shouldn't concern you */
	Phonon::State phononState() const; 
	
public slots:
    void play( const RadioStation& );
    void skip();
    void stop();

signals:
    /** emitted up to twice, as first time may not have a title for the station
      * but the second time will */
    void tuningIn( const RadioStation& );
    void trackSpooled( const Track& ); /** and we're now prebuffering */
    void trackStarted( const Track& );
    void buffering( int );
    void stopped();
	
	/** playback may still be occuring, so you may want to hold the error
	  * eg. NotEnoughContent mid-track means we can't get anymore tracks after
	  * the current playlist finishes */
	//TODO hold error until it's relevant?
	void error( Ws::Error );

private slots:
    void enqueue( const QList<Track>& );
    void onPhononStateChanged( Phonon::State, Phonon::State );
	void onPhononCurrentSourceChanged( const Phonon::MediaSource &);
	void onTunerError( Ws::Error );

	/** we get a "proper" station name from the tune webservice */
	void setStationNameIfCurrentlyBlank( const QString& );
	
private:
    /** resets internals to what Stopped means, used by changeState() */
    void clear();
    
    /** @returns true if successfully spooled or already spooled
      * spooling here basically means aligning m_track and m_mediaObjects 
      * currentSource */
    bool spoolNextTrack();
    
	/** emits signals if appropriate */
	void changeState( State );
	
	class Tuner* m_tuner;
	Phonon::AudioOutput* m_audioOutput;
	Phonon::MediaObject* m_mediaObject;
	Radio::State m_state;
	Track m_track;
	RadioStation m_station;
	
    QMap<QUrl, Track> m_queue;
};


#define CASE(x) case x: return d << #x
#include <QDebug>
inline QDebug operator<<( QDebug d, Radio::State s )
{
	switch (s)
	{
		CASE(Radio::TuningIn);
        CASE(Radio::Buffering);
		CASE(Radio::Playing);
		CASE(Radio::Stopped);
	}
    return d;
}

inline QDebug operator<<( QDebug d, Phonon::State s )
{
	switch (s)
	{
		CASE(Phonon::LoadingState);
		CASE(Phonon::StoppedState);
		CASE(Phonon::PlayingState);
		CASE(Phonon::BufferingState);
		CASE(Phonon::PausedState);
		CASE(Phonon::ErrorState);
	}
    return d;
}
#undef CASE


Q_DECLARE_METATYPE( Radio::State );

#endif //RADIO_H
