/******************************************************************************
 * Copyright (C) 2003 Stanislav Karchebny <berk@inbox.ru>                     *
 *           (C) 2005 Ian Monroe <ian@monroe.nu>                              *
 *           (C) 2005 Seb Ruiz <ruiz@kde.org>                                 *
 *           (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#ifndef AMAROK_PLAYER_DBUS_HANDLER_H
#define AMAROK_PLAYER_DBUS_HANDLER_H

#include <kurl.h>
#include <QObject>

namespace Amarok
{

    class amarokPlayerDBusHandler : public QObject
        {
            Q_OBJECT

        public:
            amarokPlayerDBusHandler();
            ~amarokPlayerDBusHandler();

        public:
            virtual QString version();
            virtual bool dynamicModeStatus();
            virtual bool equalizerEnabled();
            virtual bool osdEnabled();
            virtual bool isPlaying();
            virtual bool randomModeStatus();
            virtual bool repeatPlaylistStatus();
            virtual bool repeatTrackStatus();
            virtual int  getVolume();
            virtual int sampleRate();
            virtual double score ();
            virtual int rating ();
            virtual int  status();
            virtual int  trackCurrentTime();
            virtual int  trackCurrentTimeMs();
            virtual int  trackPlayCounter();
            virtual int  trackTotalTime();
            virtual QStringList labels();
            virtual QString album();
            virtual QString artist();
            virtual QString bitrate();
            virtual QString comment();
            virtual QString coverImage();
            virtual QString currentTime();
            virtual QString encodedURL();
            virtual QString genre();
            virtual QString lyrics();
            virtual QString lyricsByPath( QString path );
            virtual QString streamName();
            virtual QString nowPlaying();
            virtual QString path();
            virtual QString title();
            virtual QString totalTime();
            virtual QString track();
            virtual QString type();
            virtual QString year();
            virtual void configEqualizer();
            virtual void enableOSD( bool enable );
            virtual void enableRandomMode( bool enable );
            virtual void enableRepeatPlaylist( bool enable );
            virtual void enableRepeatTrack( bool enable );
            virtual void loveTrack();
            virtual void mediaDeviceMount();
            virtual void mediaDeviceUmount();
            virtual void mute();
            virtual void next();
            virtual void pause();
            virtual void play();
            virtual void playPause();
            virtual void prev();
            virtual void queueForTransfer( KUrl url );
            virtual void seek( int s );
            virtual void seekRelative( int s );
            virtual void setEqualizer(int preamp, int band60, int band170, int band310, int band600, int band1k, int band3k, int band6k, int band12k, int band14k, int band16k);
            virtual void setEqualizerEnabled( bool active );
            virtual void setEqualizerPreset( QString name );
            virtual void setLyricsByPath( const QString& url, const QString& lyrics );
            virtual void setScore( float score );
            virtual void setScoreByPath( const QString &url, double score );
            virtual void setBpm( float bpm );
            virtual void setBpmByPath( const QString &url, float bpm );
            virtual void setRating( int rating );
            virtual void setRatingByPath( const QString &url, int rating );
            virtual void setThemeFile( const QString &url );
            virtual void setVolume( int );
            virtual void setVolumeRelative( int );
            virtual void showBrowser( QString browser );
            virtual void showOSD();
            virtual void stop();
            virtual void volumeDown();
            virtual void volumeUp();
            virtual void transferDeviceFiles();

        private:
            QString m_tempFileName;
        };
}

#endif
