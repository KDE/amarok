/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi            <lfranchi@kde.org>
 *   Copyright 2012 Ryan Feng                    <odayfans@gmail.com>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SCRIPTRESOLVER_H
#define SCRIPTRESOLVER_H

#include "../SpotifyMeta.h"
#include "./Query.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/interfaces/Logger.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QVariantMap>
#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>


class QObject;

namespace Collections
{
    class SpotifyCollection;
    class SpotifyQueryMaker;
}

namespace Spotify { class Controller; }

namespace The
{
    Spotify::Controller* SpotifyController( const QString& resolverPath = QString() );
}

namespace Spotify
{

class Controller: public QObject
{
    Q_OBJECT
public:
    enum ErrorState {
        ResolverNotConnected,
        ResolverNotFound,
        InvalidCredential
    };
    explicit Controller( const QString& exec = QString() );
    virtual ~Controller();

    virtual unsigned int timeout() const { return m_timeout; }
    virtual void setTimeout( const unsigned int timeout ) { m_timeout = timeout; }
    virtual QString name() const { return m_name; }
    virtual QString resolverPath() const { return m_filePath; }
    virtual void setFilePath( const QString& resolverPath ) { m_filePath = resolverPath; }
    virtual QProcessEnvironment& environment() { return m_procEnv; }
    virtual void login(const QString& username, const QString& password, const bool highQuality = false);
    virtual bool loggedIn() const { return m_loggedIn; }
    virtual bool running() const;
    virtual bool loaded() const;

    virtual void reload();
    virtual void unload();

    virtual void sendMessage( const QVariantMap& map );


    /* Resolve a query
     */
    virtual Spotify::Query* makeQuery( Collections::SpotifyCollection* collection, const QString& title = QString(), const QString& artist = QString(), const QString& album = QString(), const QString& genre = QString());
    virtual void resolve( Query *query );

    /* Get playlist
     */
    virtual void getPlaylist( /* playlist id */ ) {};

    /* Remove a playlist from sync list
     */
    virtual void removeFromSyncList( /* playlist id */ ) {};

    /* Set collaborative
     */
    virtual void setCollaborative( /* playlist id */ ) {};

    /* Subscribe a playlist
     */
    virtual void setSubscription( /* playlist id */ ) {};

    /* Remove tracks from playlist
     */
    virtual void removeTracksFromPlaylist( /* playlist id, tracks */ ) {};

    /* Add tracks to playlist
     */
    virtual void addTracksToPlaylist( /* playlist id, tracks */ ) {};

    /* Move tracks in playlist
     */
    virtual void moveTracksInPlaylist( /* playlist id, tracks, start position */ ) {};

    /* Create a new playlist
     */
    virtual void createPlaylist( /* playlist name */ ) {};

    /* Delete a playlist
     */
    virtual void deletePlaylist( /* playlist id */ ) {};

    /* Rename a playlist
     */
    virtual void renamePlaylit( /* playlist id, new name */ ) {};

    /* Search tracks
     */
    virtual void searchTracks( /* query */ ) {};

signals:
    void started();
    void terminated();
    void customMessage( const QString& msgType, const QVariantMap& map );
    void spotifyError( const Spotify::Controller::ErrorState error );

    void errorMsgReceived( const QString& msg );

    void userChanged();
    void loginSuccess( const QString& user );
    void loginFailed( const QString &message );

    void spotifyReady();
    void queryReady( const Spotify::Query* query );

public slots:
    virtual void stop();
    virtual void start();

private slots:
    void readStderr();
    void readStdout();
    void procExited( int code, QProcess::ExitStatus status );
    void removeQueryFromCache( const QString& qid );

private:
    // Core private methods
    inline void showMessage( const QString& msg ) { Amarok::Components::logger()->shortMessage( msg ); }
    /* Send raw bytes to Spotify resolver,
     * the difference between sendRaw and sendMsg is,
     * sendRaw only sends raw data, that it, msg should contain the message header which is the length of the message body,
     * while sendMsg inserts the message header to the message then calls sendRaw
     */
    void sendRaw( const QByteArray& msg );

    /* Handles all incoming messages from Spotify resolver,
     * this parses all messages into QVariantMap and resend to specific message handlers.
     */
    void handleMsg( const QByteArray& msg );

    /* Calculates the length of @param msg and inserts it to the message then calls sendRaw
     */
    void sendMsg( const QByteArray& msg );

    /* Send config data to the Spotify resolver, not much useful in this scenario.
     */
    void sendConfig();

    /* This is the method first called when connected to the Spotify resolver,
     * it sends the proxy settinsg to the Spotify resolver so it will initialize correctly.
     * Currently the Spotify resolver only runs locally, so the proxy is set to none.
     */
    void doSetup( const QVariantMap& map );

    /* Start the Spotify resolver in background.
     */
    void startProcess();

    /* Message handler methods
     * the following methods handle all possible message types received from the Spotify resolver.
     */
    void handleSpotifyError( const QVariantMap& map );
    void handleUserchanged( const QVariantMap& map );
    void handlePlaylistReceived( const QVariantMap& map );
    void handlePlaylistRenamed( const QVariantMap& map );
    void handlePlaylistDeleted( const QVariantMap& map );
    void handleTracksAdded( const QVariantMap& map );
    void handleTracksDeleted( const QVariantMap& map );
    void handleTracksMoved( const QVariantMap& map );
    void handleTracksRemoved( const QVariantMap& map );
    void handleLoginResponse( const QVariantMap& map );
    void handleCredentials( const QVariantMap& map );
    void handleCredentialsReceived( const QVariantMap& map );
    void handleSettingsReceived( const QVariantMap& map );
    void handleAllPlaylists( const QVariantMap& map );
    void handleSearchResults( const QVariantMap& map );
    void handleQueryResponse( const QVariantMap& map );


    // Members
    QProcess  m_proc;
    QProcessEnvironment  m_procEnv;
    QString   m_name;
    QString   m_filePath;
    QString   m_lastUsername;

    quint32     m_msgSize;
    quint32     m_timeout;
    QByteArray  m_msg;

    bool m_ready;
    bool m_stopped;
    bool m_loaded;
    bool m_deleting;
    bool m_configSent;
    bool m_loggedIn;

    QJson::Parser      m_parser;
    QJson::Serializer  m_serializer;

    // This stores all queries
    QMap< QString, Spotify::QueryPtr > m_queryCache;
    QList< Spotify::QueryPtr > m_queryQueue;

    quint64 m_queryCounter;
};

} // namespace Spotify
#endif
