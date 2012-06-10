#ifndef SCRIPTRESOLVER_H
#define SCRIPTRESOLVER_H

#include <QProcess>

#include <QVariantMap>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QtEndian>

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>

class QObject;

class ScriptResolver: QObject
{
    Q_OBJECT
public:
    ScriptResolver( const QString& exec );
    virtual ~ScriptResolver();

    virtual unsigned int timeout() const { return m_timeout; }
    virtual void setTimeout( const unsigned int timeout ) { m_timeout = timeout; }
    virtual QString name() const { return m_name; }
    virtual QString filePath() const { return m_filePath; }
    virtual bool running() const;

    virtual void reload();

    virtual void sendMessage( const QVariantMap& map );
    
    /* Initial a new query, send query data to the Spotify resolver
     * @returns a new query object whose qid field is set, the qid should be used to check query status after that.
     */
    virtual Spotify::Query sendQuery( const Spotify::Query& query );

    /* Check the current status of a query
     * @returns a query result object which contains the status information, if the results are returned, the status will be successfull, and it contains the results
     */
    virtual Spotify::QueryResult checkQueryStatus( const Spotify::Query& query );
    
    /* Get playlist 
     */
    virtual void getPlaylist( /* playlist id */ );

    /* Remove a playlist from sync list
     */
    virtual void removeFromSyncList( /* playlist id */ );

    /* Set collaborative
     */
    virtual void setCollaborative( /* playlist id */ );

    /* Subscribe a playlist
     */
    virtual void setSubscription( /* playlist id */ );

    /* Remove tracks from playlist
     */
    virtual void removeTracksFromPlaylist( /* playlist id, tracks */ );

    /* Add tracks to playlist
     */
    virtual void addTracksToPlaylist( /* playlist id, tracks */ );

    /* Move tracks in playlist
     */
    virtual void moveTracksInPlaylist( /* playlist id, tracks, start position */ );

    /* Create a new playlist 
     */
    virtual void createPlaylist( /* playlist name */ );

    /* Delete a playlist
     */
    virtual void deletePlaylist( /* playlist id */ );

    /* Rename a playlist
     */
    virtual void renamePlaylit( /* playlist id, new name */ );

    /* Search tracks
     */
    virtual void searchTracks( /* query */ );

signals:
    void started();
    void terminated();
    void customMessage( const QString& msgType, const QVariantMap& map );
    void changed();

    void errorMsgReceived( const QString& msg );
    void userChanged();

    void loggedIn();

public slots:
    virtual void stop();
    virtual void start();

private slots:
    void readStderr();
    void readStdout();
    void procExited( int code, QProcess::ExitStatus status );

    // An empty message contains a qid represents the query is successfull or not
    void statusReceived( const QVariantMap& msg );

private:
    // Core private methods
    void sendRaw( const QByteArray& msg );
    void handleMsg( const QByteArray& msg );
    void sendMsg( const QByteArray& msg );

    void sendConfig();
    void doSetup( const QVariantMap& map );

    void startProcess();

    // Message handler methods
    void handleSpotifyError( const QVariantMap& map );
    void handleUserchanged( const QVariantMap& map );
    void handleResults( const QVariantMap& map );
    void handlePlaylist( const QVariantMap& map );
    void handlePlaylistRenamed( const QVariantMap& map );
    void handlePlaylistDeleted( const QVariantMap& map );
    void handleTracksAdded( const QVariantMap& map );
    void handleTracksDeleted( const QVariantMap& map );
    void handleTracksMoved( const QVariantMap& map );
    void handleTracksRemoved( const QVariantMap& map );
    void handleLoginResponse( const QVariantMap& map );
    void handleCredentials( const QVariantMap& map );
    void handleSettingsReceived( const QVariantMap& map );
    void handleAllPlaylists( const QVariantMap& map );

    // Members
    QProcess m_proc;
    QString m_name;
    QString m_filePath;

    quint32 m_msgSize, m_timeout;
    QByteArray m_msg;

    bool m_ready, m_stopped, m_deleting;

    QJson::Parser m_parser;
    QJson::Serializer m_serializer;
};

#endif
