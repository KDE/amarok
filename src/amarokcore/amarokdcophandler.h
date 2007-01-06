/***************************************************************************
                          amarokdcophandler.h  -  DCOP Implementation
                             -------------------
    begin                : Sat Oct 11 2003
    copyright            : (C) 2003 by Stanislav Karchebny
                           (C) 2005 Ian Monroe
                           (C) 2005 Seb Ruiz
    email                : berkus@users.sf.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_DCOP_HANDLER_H
#define AMAROK_DCOP_HANDLER_H

#include <qobject.h>
#include "amarokdcopiface.h"
class AmarokConfig;

namespace Amarok
{

class DcopPlayerHandler : public QObject, virtual public AmarokPlayerInterface
{
      Q_OBJECT

   public:
      DcopPlayerHandler();

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
      virtual float score ();
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
      virtual QString engine();
      virtual QString genre();
      virtual QString lyrics();
      virtual QString lyricsByPath( QString path );
      virtual QString lastfmStation();
      virtual QString nowPlaying();
      virtual QString path();
      virtual QString setContextStyle(const QString&);
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
      virtual void mediaDeviceMount();
      virtual void mediaDeviceUmount();
      virtual void mute();
      virtual void next();
      virtual void pause();
      virtual void play();
      virtual void playPause();
      virtual void prev();
      virtual void queueForTransfer( KURL url );
      virtual void seek( int s );
      virtual void seekRelative( int s );
      virtual void setEqualizer(int preamp, int band60, int band170, int band310, int band600, int band1k, int band3k, int band6k, int band12k, int band14k, int band16k);
      virtual void setEqualizerEnabled( bool active );
      virtual void setEqualizerPreset( QString name );
      virtual void setLyricsByPath( const QString& url, const QString& lyrics );
      virtual void setScore( float score );
      virtual void setScoreByPath( const QString &url, float score );
      virtual void setBpm( float bpm );
      virtual void setBpmByPath( const QString &url, float bpm );
      virtual void setRating( int rating );
      virtual void setRatingByPath( const QString &url, int rating );
      virtual void setVolume( int );
      virtual void setVolumeRelative( int );
      virtual void showBrowser( QString browser );
      virtual void showOSD();
      virtual void stop();
      virtual void volumeDown();
      virtual void volumeUp();
      virtual void transferDeviceFiles();

    private:
      virtual void transferCliArgs( QStringList args );
};


class DcopPlaylistHandler : public QObject, virtual public AmarokPlaylistInterface
{
        Q_OBJECT

   public:
      DcopPlaylistHandler();

   public:
      virtual int     getActiveIndex();
      virtual int     getTotalTrackCount();
      virtual QString saveCurrentPlaylist();
      virtual void    addMedia(const KURL &);
      virtual void    addMediaList(const KURL::List &);
      virtual void    clearPlaylist();
      virtual QString currentTrackUniqueId();
      virtual void    playByIndex(int);
      virtual void    playMedia(const KURL &);
      virtual void    popupMessage(const QString&);
      virtual void    removeCurrentTrack();
      virtual void    removeByIndex(int);
      virtual void    repopulate();
      virtual void    saveM3u(const QString& path, bool relativePaths);
      virtual void    setStopAfterCurrent(bool);
      virtual void    shortStatusMessage(const QString&);
      virtual void    shufflePlaylist();
      virtual void    togglePlaylist();
      virtual QStringList filenames();
};

class DcopPlaylistBrowserHandler : public QObject, virtual public AmarokPlaylistBrowserInterface
{
        Q_OBJECT

   public:
      DcopPlaylistBrowserHandler();

   public:
      virtual void addPodcast( const QString &url );
      virtual void scanPodcasts();
      virtual void addPlaylist( const QString &url );
      virtual int loadPlaylist( const QString &playlist );
};

class DcopContextBrowserHandler : public QObject, virtual public AmarokContextBrowserInterface
{
        Q_OBJECT

   public:
      DcopContextBrowserHandler();

   public:
      virtual void showCurrentTrack();
      virtual void showLyrics();
      virtual void showWiki();
      virtual void showLyrics( const QCString& lyrics );
};


class DcopCollectionHandler : public QObject, virtual public AmarokCollectionInterface
{
   Q_OBJECT

   public:
       DcopCollectionHandler();

   public /* DCOP */ slots:
      virtual int totalAlbums();
      virtual int totalArtists();
      virtual int totalComposers();
      virtual int totalCompilations();
      virtual int totalGenres();
      virtual int totalTracks();
      virtual bool isDirInCollection( const QString &path );
      virtual bool moveFile( const QString &oldURL, const QString &newURL, bool overwrite );
      virtual QStringList query(const QString& sql);
      virtual QStringList similarArtists( int artists );
      virtual void migrateFile( const QString &oldURL, const QString &newURL );
      virtual void scanCollection();
      virtual void scanCollectionChanges();
      virtual void disableAutoScoring( bool disable );
      virtual void scanUnpause();
      virtual void scanPause();
      virtual void scannerAcknowledged();
      virtual int addLabels( const QString &url, const QStringList &labels );
      virtual void removeLabels( const QString &url, const QStringList &oldLabels );
      virtual int deviceId( const QString &url );
      virtual QString relativePath( const QString &url );
      virtual QString absolutePath( int deviceid, const QString &relativePath );
};


class DcopScriptHandler : public QObject, virtual public AmarokScriptInterface
{
   Q_OBJECT

   public:
       DcopScriptHandler();

   public /* DCOP */ slots:
      virtual bool runScript(const QString&);
      virtual bool stopScript(const QString&);
      virtual QStringList listRunningScripts();
      virtual void addCustomMenuItem(QString submenu, QString itemTitle );
      virtual void removeCustomMenuItem(QString submenu, QString itemTitle );
      virtual QString readConfig(const QString& key);
      virtual QStringList readListConfig(const QString& key);
      virtual QString proxyForUrl(const QString& url);
      virtual QString proxyForProtocol(const QString& protocol);
};

class DcopDevicesHandler : public QObject, virtual public AmarokDevicesInterface
{
   Q_OBJECT

   public:
       DcopDevicesHandler();

   public /* DCOP */ slots:
      virtual void mediumAdded(QString name);
      virtual void mediumRemoved(QString name);
      virtual void mediumChanged(QString name);
      virtual QStringList showDeviceList();
};

class DcopMediaBrowserHandler : public QObject, virtual public AmarokMediaBrowserInterface
{
    Q_OBJECT

    public:
        DcopMediaBrowserHandler();

    public /* DCOP */ slots:
      virtual void deviceConnect();
      virtual void deviceDisconnect();
      virtual QStringList deviceList();
      virtual void deviceSwitch( QString name );
      virtual void queue( KURL url );
      virtual void queueList( KURL::List urls );
      virtual void transfer();
      virtual void transcodingFinished( QString src, QString dest );
};

} // namespace Amarok

#endif
