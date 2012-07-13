/****************************************************************************************
 * Copyright (c) 2004 Frederik Holljen <fh@ez.no>                                       *
 * Copyright (c) 2004,2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2004-2010 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2008 Jason A. Donenfeld <Jason@zx2c4.com>                              *
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
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

#ifndef AMAROK_ENGINECONTROLLER_H
#define AMAROK_ENGINECONTROLLER_H

#include "core/capabilities/BoundedPlaybackCapability.h"
#include "core/meta/Meta.h"

#include <QMutex>
#include <QObject>
#include <QWeakPointer>

#include <Phonon/Path>
#include <Phonon/MediaController>
#include <Phonon/MediaObject>
#include <Phonon/Effect>
#include <Phonon/EffectParameter>
#include <phonon/audiodataoutput.h>

class QTimer;

namespace Capabilities { class MultiPlayableCapability; class MultiSourceCapability; }
namespace Phonon { class AudioOutput; class MediaSource; class VolumeFaderEffect; }

/**
 * A thin wrapper around Phonon that implements Amarok-specific functionality like
 * replay gain, fade-out on stop and various track capabilities that affect
 * playback.
 */
class AMAROK_EXPORT EngineController : public QObject, public Meta::Observer
{
    Q_OBJECT

public:
    EngineController();
    ~EngineController();

    /**
     * Returns the global EngineController instance
     */
    static EngineController* instance();

    /**
     * Loads and plays the track that was playing when endSession() was last
     * called (ie: when Amarok was quit)
     */
    void restoreSession();

    /**
     * Saves the currently playing track and the playing/paused/stopped state
     */
    void endSession();

    /**
     * Returns a list of backend supported mime types. This method is thread-safe.
     */
    QStringList supportedMimeTypes();

    /** @return track position (elapsed time) in seconds */
    int trackPosition() const;

    /** @return track position (elapsed time) in milliseconds */
    int trackPositionMs() const;

    /**
     * Returns the current track that is loaded into the engine.
     * @return a Meta::TrackPtr which is either the track, or empty if phonon
     * has a state of Phonon::ErrorState or Phonon::StoppedState
     */
    Meta::TrackPtr currentTrack() const;
    /**
     * @return the length of the current track in milliseconds
     */
    qint64 trackLength() const;

    /**
     * Used to enqueue a track before it starts to play, for gapless playback.
     *
     * This will clear any tracks currently in the queue.  If no track is playing,
     * @p track will be played immediately.
     */
    void setNextTrack( Meta::TrackPtr track );

    /*enum Filetype { MP3 };*/ //assuming MP3 for time being
    /*AMAROK_EXPORT*/ static bool installDistroCodec();

    /** Returns the media object Amarok is using for playback.
     *  Provides access to the Phonon MediaObject for components that need more information
     *  This is not for normal use, except maybe when you really need
     *  the stateChanged signal.
     */
    // const so that it can only be used for info
    const Phonon::MediaObject* phononMediaObject() const { return m_media.data(); }

    /**
     * Gets the volume
     * @return the volume as a percentage
     */
    int volume() const;

    /**
     * @return @c true if sound output is disabled, @false otherwise
     */
    bool isMuted() const;

    /**
     * @return @c true if Amarok is paused, @c false if it is stopped or playing
     */
    bool isPaused() const;

    /**
     * @return @c true if Amarok is playing, @c false if it is stopped or pause
     * Note: A fading out track is considered already stopped.
     */
    bool isPlaying() const;

    /**
     * @return @c true if Amarok is stopped, @c false if it is playing or pause
     * Note: A fading out track is considered already stopped.
     */
    bool isStopped() const;

    /**
     * @return @c true if Amarok is currently buffering or loading.
     * Usually Amarok isPlaying is true in those cases too but that depends on the
     * Phonon backend.
     */
    bool isBuffering() const;

    /**
     * Streams sometimes have to be treated specially.
     * For example, it is typically not possible to rewind a stream (at least,
     * not without returning to the start of it).
     * However for rewinding we have isSeekable().
     * Also for streams usually the meta data received by currentTrack() is only
     * for the whole stream while the meta data received by currentMetaDataChanged
     * will be more current (or contain advertisment)
     *
     * @return @c true if the current track is a stream, @c false otherwise
     */
    bool isStream();

    /**
     * @return @c true if the current track is seekable, @c false otherwise
     */
    bool isSeekable() const;

    /**
     * Phonon kequalizer support is required for Amarok to enable equalizer
     * this method return whatever phonon support kequalizer effect.
     *
     * @return @c true if the phonon support kequalizer effect, @c false otherwise
     */
    bool isEqSupported() const;

    /**
     * kequalizer implementation for different backends may have different
     * gain scale. To properly display it we need to get a scale from effect
     *
     * @return maximum gain value for kequalizer parameters.
     */
    double eqMaxGain() const;

     /**
     * kequalizer implementation for different backends may have different
     * frequency bands. For proper display this will try to extract frequency values
     * from effect parameters info.
     *
     * @return QStringList with band labels (form xxx Hz or xxx kHz).
     */
    QStringList eqBandsFreq() const;

    /**
    * @return whether the currently playing track is from an audiocd
    */
    bool isPlayingAudioCd();

    /**
     * @return QString with a pretty name for the current track
     * @param whether to include the playing progress (default false)
     */
    QString prettyNowPlaying( bool progress = false ) const;

public slots:
    /**
     * Plays the current track, if there is one
     * This happens asynchronously.
     */
    void play();

    /**
     * Plays the specified track
     * This happens asynchronously.
     */
    void play( Meta::TrackPtr track, uint offset = 0 );

    /**
     * Replays the current track
     *
     * This is a convenience method, calls seek( 0 ) actually
     */
    void replay();

    /**
     * Pauses the current track
     * This happens asynchronously.
     */
    void pause();

    /**
     * Stops playing
     * This happens asynchronously.
     */
    void stop( bool forceInstant = false );

    /**
     * Pauses if Amarok is currently playing, plays if Amarok is stopped or paused
     * This happens asynchronously.
     */
    void playPause(); //pauses if playing, plays if paused or stopped

    /**
     * Seeks to a position in the track
     *
     * If the media is not seekable, or the state is something other than
     * PlayingState, BufferingState or PausedState, has no effect.
     *
     * Deals correctly with tracks that have the BoundedPlayback capability.
     *
     * @param ms the position in milliseconds (counting from the start of the track)
     */
    void seek( int ms );

    /**
     * Seeks forward or backward in the track
     *
     * If the media is not seekable, or the state is something other than
     * PlayingState, BufferingState or PausedState, has no effect.
     *
     * Deals correctly with tracks that have the BoundedPlayback capability.
     *
     * A negative value seeks backwards, a positive value seeks forwards.
     *
     * If the value of @p ms would move the position to before the start of the track,
     * the position is moved to the start of the track.
     *
     * @param ms the offset from the current position in milliseconds
     */
    void seekRelative( int ms );

    /**
     * Seeks forward in the track
     *
     * Same as seekRelative()
     */
    void seekForward( int ms = 10000 );

    /**
     * Seeks backward in the track
     *
     * Works identically to seekRelative(), but seeks in the opposite direction.
     */
    void seekBackward( int ms = 10000 );

    /**
     * Increases the volume
     *
     * @param ticks the amount to increase the volume by, given as a percentage of the
     * maximum possible volume (ie: the same units as for setVolume()).
     */
    int increaseVolume( int ticks = 100/25 );

    /**
     * Decreases the volume
     *
     * @param ticks the amount to decrease the volume by, given as a percentage of the
     * maximum possible volume (ie: the same units as for setVolume()).
     */
    int decreaseVolume( int ticks = 100/25 );

    /**
     * Sets the volume
     *
     * @param percent the new volume as a percentage of the maximum possible volume.
     */
    // this amplifier does not go up to 11
    int setVolume( int percent );

    /**
     * Mutes or unmutes playback
     *
     * @param mute if @c true, audio output will be disabled; if @c false, audio output
     * will be enabled.
     */
    void setMuted( bool mute );

    /**
     * Toggles mute
     *
     * Works like setMuted( !isMuted() );
     */
    void toggleMute();

    /**
     * Update equalizer status - enabled,disabled,set values
     */
    void eqUpdate();


Q_SIGNALS:
    /**
     * Emitted when the playback stops while playing a track.
     * This signal is not emitted when the track pauses or the playback stopps because
     * Amarok was closed and "resume at start" is configured.
     * It is also not emitted if the playback continues with another track. In such
     * a case you would just get another trackPlaying signal.
     * Both parameters are in milli seconds.
     */
    void stopped( qint64 /*ms*/ finalPosition, qint64 /*ms*/ trackLength );

    /**
     * Called when the playback is paused.
     * When the playback is resumed a trackPlaying signal will be emitted.
     * When the playback is stopped then a stopped signal will be emitted.
     */
    void paused();

    /** While trying to play the track an error occured.
     *  This usually means that the engine will try to play the next track in
     *  the playlist until it gives up.
     *  So you will get a trackPlaying or stopped signal next.
     */
    void trackError( Meta::TrackPtr track );

    /**
     * Called when a new track starts playing or an old track starts playing now.
     *
     * It also might be called several time in sequence
     * with the same track in cases when e.g. you have
     * a multi source track.
     *
     * Unlike trackChanged(), this is not called when playback stops.
     */
    void trackPlaying( Meta::TrackPtr track );

    /**
     * Called when the current track changes
     *
     * Note that this is possibly only called once in case of a stream or on
     * the other hand multiple times with the same track in cases when e.g. you have
     * a multi source track.
     *
     * Unlike trackPlaying(), this is called when playback stops with Meta::TrackPtr( 0 ) for @p track.
     *
     * @param track The new track; may be null
     */
    void trackChanged( Meta::TrackPtr track );

    /** Emitted when the metadata of the current track changes.
        You might want to connect also to trackChanged to get more changes because
        this signal is only emitted when the track metadata changes while it's playing.
        Also connecting to currentMetadataChanged would give you information when the current track
        is a stream. In such a case the meta information from track might be unreliable.
     */
    void trackMetadataChanged( Meta::TrackPtr track );

    /** Emitted when the metadata of the current album changes.
     */
    void albumMetadataChanged( Meta::AlbumPtr album );

    /**
     * Emitted then the information for the current changed.
     * This signal contains data from Phonon about the meta data of the track or stream.
     * This signal is expecially emitted when a stream changes it's metadata.
     * This can happen e.g. in a ogg stream where the currentTrack data will probably
     * not be updated.
     *
     * MetaStream::Track::Private in Stream_p.h will connect to this signal to update it's internal data
     * and then itself trigger a trackMetadataChanged.
     * @param metadata Contains the url, artist, album title, title, genre, tracknumber and length
     */
    void currentMetadataChanged( QVariantMap metadata );

    /**
     * Called when the seekable value was changed
     */
    void seekableChanged( bool seekable );

    /**
     * Called when the volume was changed
     */
    void volumeChanged( int percent );

    /**
     * Called when audio output was enabled or disabled
     *
     * NB: if setMute() was called on the engine controller, but it didn't change the
     * mute state, this will not be called
     */
    void muteStateChanged( bool mute );

    /** Called when the track position changes.
        If the track just progresses you will get a notification every couple of milliseconds.
        @parem position The current position in milliseconds
        @param userSeek True if the position change was caused by the user
    */
    void trackPositionChanged( qint64 position, bool userSeek );

    /**
       Called when the track length changes, typically because the track has changed but
       also when phonon manages to determine the full track length.
    */
    void trackLengthChanged( qint64 milliseconds );

    /**
     * Called when Amarok is closed and we disconnect from Phonon.
     * @param resumePlayback True if amarok will continue playback after a restart.
     */
    void sessionEnded( bool resumePlayback );

    /**
     * Called when playback state changes to PlayingState, StoppedState or PausedState.
     */
    void playbackStateChanged();
    
    /**
    *   Is emitted when new audio Data is ready
    *   @param audioData The audio data that is available
    */
    void audioDataReady( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > &audioData );

private slots:
    /**
     * Sets up the Phonon system
     */
    void initializePhonon();
    /** This slot is connected to the phonon finished signal.
        It is emitted when the queue is empty and the current media come to an end.
    */
    void slotFinished();
    void slotAboutToFinish();
    void slotNewTrackPlaying( const Phonon::MediaSource &source);
    void slotStateChanged( Phonon::State newState, Phonon::State oldState);
    void slotPlayableUrlFetched(const KUrl&);
    void slotTick( qint64 );
    void slotTrackLengthChanged( qint64 );
    void slotMetaDataChanged();
    void slotStopFadeout(); //called after the fade-out has finished
    void slotSeekableChanged( bool );

    /**
     * For volume/mute changes from the phonon side
     */
    void slotVolumeChanged( qreal );
    void slotMutedChanged( bool );

    /**
     *  Notify the engine that a new title has been reached when playing a cd. This
     *  is needed as a cd counts as basically one lone track, and we want to be able
     *  to play something else once one track has finished
     */
    void slotTitleChanged( int titleNumber );

protected:
    // reimplemented from Meta::Observer
    using Observer::metadataChanged;
    virtual void metadataChanged( Meta::TrackPtr track );
    virtual void metadataChanged( Meta::AlbumPtr album );

private:

    /** Tries to get the url of the given track setting it as current.
        This function will cause slotPlayableUrlFetched
        to be called eventually.
    */
    void getUrl( Meta::TrackPtr track, uint offset );

    /** Will try to get either the next url or the next track.
        This function will call or initiate a call to slotPlayableUrlFetched or
        setNextTrack.
        slotPlayableUrlFetched, setNextTrack or slotQueueEnded.
    */
    void getNextUrlOrTrack();

    /**
     * Plays the media at a specified URL
     *
     * @param url the URL of the media
     * @param offset the position in the media to start at in milliseconds
     */
    void playUrl( const KUrl &url, uint offset );

    void createFadeoutEffect();
    void resetFadeout();

    void setGain();

    /** Returns the meta data sub set needed for currentMetadataChanged */
    QVariantMap trackData( Meta::TrackPtr track );

    /** Try to detect MetaData spam in Streams.
        Some streams are doing advertisment in the metadata. We try to filter that out
    */
    bool isMetadataSpam( QVariantMap meta );

    /** Will change the current track, notifying everybody about the change.
        This function will also update the different variables connected to the current track e.g. m_boundedPlayback or m_fetchFirst.
    */
    void setCurrentTrack( Meta::TrackPtr track );

    Q_DISABLE_COPY( EngineController )

    static QMutex s_supportedMimeTypesMutex; // guards access to supportedMimeTypes()::mimeTable

    QWeakPointer<Phonon::MediaObject>       m_media;
    QWeakPointer<Phonon::VolumeFaderEffect> m_preamp;
    QWeakPointer<Phonon::Effect>            m_equalizer;
    QWeakPointer<Phonon::AudioOutput>       m_audio;
    QWeakPointer<Phonon::AudioDataOutput>   m_audioDataOutput;
    QWeakPointer<Phonon::MediaController>   m_controller;
    Phonon::Path                            m_path;
    Phonon::Path                            m_dataPath;

    Phonon::VolumeFaderEffect* m_fader;
    QTimer* m_fadeoutTimer;

    Meta::TrackPtr  m_currentTrack;
    Meta::AlbumPtr  m_currentAlbum;
    Meta::TrackPtr  m_lastTrack;
    Meta::TrackPtr  m_nextTrack;
    KUrl            m_nextUrl;
    Capabilities::BoundedPlaybackCapability* m_boundedPlayback;
    Capabilities::MultiPlayableCapability* m_multiPlayback;
    Capabilities::MultiSourceCapability* m_multiSource;
    bool m_playWhenFetched;
    int m_volume;
    bool m_currentIsAudioCd;
    int m_currentAudioCdTrack;

    QList<QVariantMap> m_metaDataHistory; // against metadata spam

    /**
     * Some flags to prevent feedback loops in volume updates
     */
    bool m_ignoreVolumeChangeAction;
    bool m_ignoreVolumeChangeObserve;

    // Used to get a more accurate estimate of the position for slotTick
    int m_tickInterval;
    qint64 m_lastTickPosition;
    qint64 m_lastTickCount;

    QMutex m_mutex;
};

namespace The {
    AMAROK_EXPORT EngineController* engineController();
}


#endif
