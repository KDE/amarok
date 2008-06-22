/******************************************************************************
 * Copyright (C) 2003 Stanislav Karchebny <berk@inbox.ru>                     *
 *           (C) 2004 Christian Muehlhaeuser <chris@chris.de>                 *
 *           (C) 2005 Ian Monroe <ian@monroe.nu>                              *
 *           (C) 2005 Seb Ruiz <ruiz@kde.org>                                 *
 *           (C) 2006 Alexandre Pereira de Oliveira <aleprj@gmail.com>        *
 *           (C) 2006 2007 Leonardo Franchi <lfranchi@gmail.com>              *
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

#include "amarokPlayerDBusHandler.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "App.h"
#include "collection/CollectionManager.h"
#include "Debug.h"
#include "EngineController.h"
#include "equalizersetup.h"
#include "MainWindow.h"
#include "mediabrowser.h" //KSelectAction?
#include "meta/StreamInfoCapability.h"
#include "meta/SourceInfoCapability.h"
#include "meta/MetaUtility.h"
#include "Osd.h"
#include "playlist/PlaylistModel.h"
#include "SvgHandler.h"
#include "TheInstances.h"

#include <KTemporaryFile>

#include "amarokPlayerAdaptor.h"

namespace Amarok
{

    amarokPlayerDBusHandler::amarokPlayerDBusHandler()
        : QObject( kapp ), m_tempFileName()
    {
        new amarokPlayerAdaptor(this);
        QDBusConnection::sessionBus().registerObject("/Player", this);
    }

    amarokPlayerDBusHandler::~amarokPlayerDBusHandler()
    {
        QFile::remove( m_tempFileName );
    }

    QString amarokPlayerDBusHandler::version()
    {
        return APP_VERSION;
    }

    bool amarokPlayerDBusHandler::dynamicModeStatus()
    {
        AMAROK_NOTIMPLEMENTED
        return false;
        //TODO: PORT 2.0
        //         return Amarok::dynamicMode();
    }

    bool amarokPlayerDBusHandler::equalizerEnabled()
    {
        AMAROK_NOTIMPLEMENTED
        if( false )
            return AmarokConfig::equalizerEnabled();
        else
            return false;
    }

    bool amarokPlayerDBusHandler::osdEnabled()
    {
        return AmarokConfig::osdEnabled();
    }

    bool amarokPlayerDBusHandler::isPlaying()
    {
        return The::engineController()->state() == Phonon::PlayingState;
    }

    bool amarokPlayerDBusHandler::randomModeStatus()
    {
        return AmarokConfig::randomMode();
    }

    bool amarokPlayerDBusHandler::repeatPlaylistStatus()
    {
        return Amarok::repeatPlaylist();
    }

    bool amarokPlayerDBusHandler::repeatTrackStatus()
    {
        return Amarok::repeatTrack();
    }

    int amarokPlayerDBusHandler::getVolume()
    {
        return The::engineController()->volume();
    }

    int amarokPlayerDBusHandler::sampleRate()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->sampleRate() : 0;
    }

    double amarokPlayerDBusHandler::score()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->score() : 0.0;
    }

    int amarokPlayerDBusHandler::rating()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->rating() : 0;
    }

    int  amarokPlayerDBusHandler::status()
    {
        // <0 - error, 0 - stopped, 1 - paused, 2 - playing
        switch( The::engineController()->state() )
        {
            case Phonon::PlayingState:
            case Phonon::BufferingState:
                return 2;
            case Phonon::PausedState:
                return 1;
            case Phonon::LoadingState:
            case Phonon::StoppedState:
                return 0;
            case Phonon::ErrorState:
                return -1;
        }
        return -1;
    }

    int amarokPlayerDBusHandler::trackCurrentTime()
    {
        return The::engineController()->trackPosition();
    }

    int amarokPlayerDBusHandler::trackCurrentTimeMs()
    {
        return The::engineController()->trackPosition() * 1000;
    }

    int amarokPlayerDBusHandler::trackPlayCounter()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->playCount() : 0;
    }

    int amarokPlayerDBusHandler::trackTotalTime()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->length() : 0;
    }

    QStringList amarokPlayerDBusHandler::labels()
    {
        AMAROK_NOTIMPLEMENTED
        //TODO: fix me
        return QStringList();
    }

    QString amarokPlayerDBusHandler::album()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->album()->prettyName() : QString();
    }

    QString amarokPlayerDBusHandler::artist()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->artist()->prettyName() : QString();
    }

    QString amarokPlayerDBusHandler::bitrate()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? QString::number( track->bitrate() ) : QString();
    }

    QString amarokPlayerDBusHandler::comment()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->comment() : QString();
    }

    QString amarokPlayerDBusHandler::coverImage()
    {
        if( m_tempFileName.isNull() )
        {
            KTemporaryFile tempFile; //TODO delete when we're done
            tempFile.setSuffix( ".png" );
            tempFile.setAutoRemove( false );
            if( !tempFile.open() )
                return QString();

            m_tempFileName = tempFile.fileName();
        }

        Meta::TrackPtr track = The::engineController()->currentTrack();
        if( track && track->album() )
        {
            debug() << "Saving album image.";
            track->album()->image().save( m_tempFileName, "PNG" ); // For covers w/ alpha channel
            return m_tempFileName;
        }
        else 
            return QString();
    }

    QString amarokPlayerDBusHandler::currentTime()
    {
        return Meta::secToPrettyTime( The::engineController()->trackPosition() );
    }

    QString amarokPlayerDBusHandler::encodedURL()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->playableUrl().url() : QString();
    }

    QString amarokPlayerDBusHandler::genre()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->genre()->prettyName() : QString();
    }

    QString amarokPlayerDBusHandler::lyrics()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->cachedLyrics() : QString();
    }

    QString amarokPlayerDBusHandler::lyricsByPath( QString path )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl(path) );
        return track ? track->cachedLyrics() : QString();
    }

    QString amarokPlayerDBusHandler::streamName()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if( !track )
            return QString();
        Meta::StreamInfoCapability *streamInfo = track->as<Meta::StreamInfoCapability>();
        QString streamText;
        if( streamInfo )
        {
            streamText = streamInfo->streamName();
            if( !streamInfo->streamSource().isEmpty() )
                streamText += " (" + streamInfo->streamSource() + ')';
            delete streamInfo;
        }
        else
        {
            Meta::SourceInfoCapability *sourceInfo = track->as<Meta::SourceInfoCapability>();
            if( sourceInfo )
            {
                streamText = sourceInfo->sourceName();
                delete sourceInfo;
            }
        }

        return streamText;
    }

    QString amarokPlayerDBusHandler::nowPlaying()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if( !track )
            return QString();
        QString playingText;
        if( track->artist() )
        {
            playingText = track->artist()->prettyName();
            playingText += " - ";
        }
        playingText += track->prettyName();
        if( track->album() )
        {
            playingText += '(';
            playingText += track->album()->prettyName();
            playingText += ')';
        }
        return playingText;
    }

    QString amarokPlayerDBusHandler::path()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->playableUrl().path() : QString();
    }

    QString amarokPlayerDBusHandler::title()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->prettyName() : QString();
    }

    QString amarokPlayerDBusHandler::totalTime()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? Meta::secToPrettyTime( track->length() ) : QString();
    }

    QString amarokPlayerDBusHandler::track()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? QString::number( track->trackNumber() ) : QString();
    }

    QString amarokPlayerDBusHandler::type()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        QString type = track ? track->type() : QString();
        if( type == "stream/lastfm" )
            return "LastFm Stream";
        else
            return type;
    }
    
    QString amarokPlayerDBusHandler::year()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->year()->prettyName() : QString();
    }
    
    void amarokPlayerDBusHandler::configEqualizer()
    {
        AMAROK_NOTIMPLEMENTED
        if( false ) //TODO phonon
        {
            EqualizerSetup::instance()->show();
            EqualizerSetup::instance()->raise();
        }
    }
    
    void amarokPlayerDBusHandler::enableOSD(bool enable)
    {
        Amarok::OSD::instance()->setEnabled( enable );
        AmarokConfig::setOsdEnabled( enable );
    }
    
    void amarokPlayerDBusHandler::enableRandomMode( bool enable )
    {
        static_cast<KSelectAction*>(Amarok::actionCollection()->action( "random_mode" ))
        ->setCurrentItem( enable ? AmarokConfig::EnumRandomMode::Tracks : AmarokConfig::EnumRandomMode::Off );
    }
    
    void amarokPlayerDBusHandler::enableRepeatPlaylist( bool enable )
    {
        static_cast<KSelectAction*>( Amarok::actionCollection()->action( "repeat" ) )
        ->setCurrentItem( enable ? AmarokConfig::EnumRepeat::Playlist : AmarokConfig::EnumRepeat::Off );
    }
    
    void amarokPlayerDBusHandler::enableRepeatTrack( bool enable)
    {
        static_cast<KSelectAction*>( Amarok::actionCollection()->action( "repeat" ) )
        ->setCurrentItem( enable ? AmarokConfig::EnumRepeat::Track : AmarokConfig::EnumRepeat::Off );
    }
    
    void amarokPlayerDBusHandler::loveTrack()
    {
        MainWindow::self()->loveTrack();
    }
    
    void amarokPlayerDBusHandler::mediaDeviceMount()
    {
        AMAROK_NOTIMPLEMENTED
        //if ( MediaBrowser::instance()->currentDevice() )
        //   MediaBrowser::instance()->currentDevice()->connectDevice();
    }
    
    void amarokPlayerDBusHandler::mediaDeviceUmount()
    {
        AMAROK_NOTIMPLEMENTED
        //if ( MediaBrowser::instance()->currentDevice() )
        //    MediaBrowser::instance()->currentDevice()->disconnectDevice();
    }
    
    void amarokPlayerDBusHandler::mute()
    {
        The::engineController()->mute();
    }
    
    void amarokPlayerDBusHandler::next()
    {
        The::playlistModel()->next();
    }
    
    void amarokPlayerDBusHandler::pause()
    {
        The::engineController()->pause();
    }
    
    void amarokPlayerDBusHandler::play()
    {
        The::engineController() ->play();
    }
    
    void amarokPlayerDBusHandler::playPause()
    {
        The::engineController() ->playPause();
    }
    
    void amarokPlayerDBusHandler::prev()
    {
        The::playlistModel()->back();
    }
    
    void amarokPlayerDBusHandler::queueForTransfer( KUrl url )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( url );
        //MediaBrowser::queue()->addUrl( url );
        //MediaBrowser::queue()->URLsAdded();
    }
    
    void amarokPlayerDBusHandler::seek(int s)
    {
        if ( s > 0 && The::engineController()->state() != Phonon::StoppedState )
            The::engineController()->seek( s * 1000 );
    }
    
    void amarokPlayerDBusHandler::seekRelative(int s)
    {
        The::engineController() ->seekRelative( s * 1000 );
    }
    
    void amarokPlayerDBusHandler::setEqualizer(int preamp, int band60, int band170, int band310,
                                         int band600, int band1k, int band3k, int band6k, int band12k, int band14k, int band16k)
    {
        AMAROK_NOTIMPLEMENTED
        if( false )
        {
            bool instantiated = EqualizerSetup::isInstantiated();
            EqualizerSetup* eq = EqualizerSetup::instance();
            
            QList<int> gains;
            gains << band60 << band170 << band310 << band600 << band1k
            << band3k << band6k << band12k << band14k << band16k;
            
            eq->setBands( preamp, gains );
            if( !instantiated )
                delete eq;
        }
    }
    
    void amarokPlayerDBusHandler::setEqualizerEnabled( bool active )
    {
        AMAROK_NOTIMPLEMENTED
        //TODO PhononEqualizer        EngineController::engine()->setEqualizerEnabled( active );
        AmarokConfig::setEqualizerEnabled( active );
        
        if( EqualizerSetup::isInstantiated() )
            EqualizerSetup::instance()->setActive( active );
    }
    
    void amarokPlayerDBusHandler::setEqualizerPreset( QString name )
    {
        AMAROK_NOTIMPLEMENTED
        bool instantiated = EqualizerSetup::isInstantiated();
        EqualizerSetup* eq = EqualizerSetup::instance();
        eq->setPreset( name );
        if ( !instantiated )
            delete eq;
    }
    
    void amarokPlayerDBusHandler::setLyricsByPath( const QString& url, const QString& lyrics )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl(url) );
        if( track )
            track->setCachedLyrics( lyrics );
    }
    
    void amarokPlayerDBusHandler::setScore( float score )
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if( track )
            track->setScore( score );
    }
    
    void amarokPlayerDBusHandler::setScoreByPath( const QString &url, double score )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl(url) );
        if( track )
            track->setScore( score );
    }
    
    void amarokPlayerDBusHandler::setBpm( float bpm )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( bpm )
        //Meta::Track does not provide a setBpm method
        //is it necessary?
    }
    
    void amarokPlayerDBusHandler::setBpmByPath( const QString &url, float bpm )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( url )
        Q_UNUSED( bpm )
        /*MetaBundle bundle( url );
         bundle.setBpm(bpm);
         bundle.save();
         CollectionDB::instance()->updateTags( bundle.url().path(), bundle, true );*/
    }
    
    void amarokPlayerDBusHandler::setRating( int rating )
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if( track )
            track->setRating( rating );
    }
    
    void amarokPlayerDBusHandler::setRatingByPath( const QString &url, int rating )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl(url) );
        if( track )
            track->setRating( rating );
    }
    
    void amarokPlayerDBusHandler::setVolume(int volume)
    {
        The::engineController()->setVolume(volume);
    }
    
    void amarokPlayerDBusHandler::setVolumeRelative(int ticks)
    {
        The::engineController()->increaseVolume(ticks);
    }
    
    void amarokPlayerDBusHandler::showBrowser( QString browser )
    {
        if ( browser == "collection" )
            MainWindow::self()->showBrowser( "CollectionBrowser" );
        if ( browser == "playlist" )
            MainWindow::self()->showBrowser( "PlaylistBrowser" );
        //if ( browser == "media" )
        //    MainWindow::self()->showBrowser( "MediaBrowser" );
        if ( browser == "file" )
            MainWindow::self()->showBrowser( "FileBrowser" );
    }
    
    void amarokPlayerDBusHandler::showOSD()
    {
        Amarok::OSD::instance()->forceToggleOSD();
    }
    
    void amarokPlayerDBusHandler::stop()
    {
        The::engineController()->stop();
    }
    
    void amarokPlayerDBusHandler::transferDeviceFiles()
    {
        AMAROK_NOTIMPLEMENTED
        //if ( MediaBrowser::instance()->currentDevice() )
        //    MediaBrowser::instance()->currentDevice()->transferFiles();
    }
    
    void amarokPlayerDBusHandler::volumeDown()
    {
        The::engineController()->decreaseVolume();
    }
    
    void amarokPlayerDBusHandler::volumeUp()
    {
        The::engineController()->increaseVolume();
    }
    
    void amarokPlayerDBusHandler::setThemeFile( const QString & url )
    {
        The::svgHandler()->setThemeFile( url );
    }

}

#include "amarokPlayerDBusHandler.moc"
