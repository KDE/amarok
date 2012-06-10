#include "ScriptResolver.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>

ScriptResolver::ScriptResolver( const QString& exec )
: QObject(0)
, m_filePath( exec )
, m_msgSize( 0 )
, m_timeout ( 5 )
, m_ready( false )
, m_stopped( true )
, m_deleting( false )
, m_configSent( false )
{
    qDebug() << Q_FUNC_INFO << "Spotify ScriptResolver created: " << exec;

    connect( &m_proc, SIGNAL( readyReadStandardError() ), SLOT( readStderr() ) );
    connect( &m_proc, SIGNAL( readyReadStandardOutput() ), SLOT( readStdout() ) );
    connect( &m_proc, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( procExited( int, QProcess::ExitStatus ) ) );

    startProcess();
}

ScriptResolver::~ScriptResolver()
{
    disconnect( &m_proc, SIGNAL( finished( int, QProcess::ExitStatus ) ), this, SLOT( procExited( int, QProcess::ExitStatus ) ) );
    m_deleting = true;

    QVariantMap msg; 
    msg[ "_msgtype" ] = "quit";
    sendMessage( msg );

    bool finished = m_proc.waitForFinished( 2500 );

    if( !finished || m_proc.state() == QProcess::Running )
    {
        qDebug() << "External resolver didn't exit after wating 2s for it to die, killing forcefully";
#ifdef Q_OS_WIN
        m_proc.kill();
#else
        m_proc.terminate();
#endif
    }
}

void
ScriptResolver::start()
{
    m_stopped = false;
    if( m_ready ) 
    {
        //TODO: Notify Amarok to enable Spotify plugin
    } else if ( !m_configSent ) {
        sendConfig();
    }
    m_stopped = false;
}

void 
ScriptResolver::sendConfig()
{
    // Sending config is optional, currently config only contains proxy information
    // Only if the Spotify resolver didn't send out settings message(not react after startProcess), 
    // this config will be sent.
    if( m_configSent ){
        return;
    }

    QVariantMap config;
    config["_msgtype"] = "config";
    // Don't use proxy
    config["proxytype"] = "none";
    m_configSent = true;
    sendMessage(config);
}

void
ScriptResolver::reload()
{
    startProcess();
}

bool
ScriptResolver::running() const
{
    return !m_stopped;
}

void
ScriptResolver::sendMessage( const QVariantMap& map )
{
    QByteArray data = m_serializer.serialize( map );
    sendRaw( data );
}

void ScriptResolver::readStderr()
{
    qDebug() << "SCRIPT_STDERR" << filePath() << m_proc.readAllStandardError();
}

void
ScriptResolver::readStdout()
{
    if( m_msgSize == 0 )
    {
        if( m_proc.bytesAvailable() < 4 )
            return;
        quint32 len;
        m_proc.read( (char*) &len, 4 );
        m_msgSize = qFromBigEndian( len );
    }

    if( m_msgSize > 0 )
    {
        m_msg.append( m_proc.read( m_msgSize - m_msg.length() ) );
    }

    if( m_msgSize == (quint32) m_msg.length() )
    {
        handleMsg( m_msg );
        m_msgSize = 0;
        m_msg.clear();

        if( m_proc.bytesAvailable() )
        {
            QTimer::singleShot( 0, this, SLOT( readStdout() ) );
        }
    }
}

void
ScriptResolver::sendRaw( const QByteArray& msg )
{
    if( !m_proc.isOpen() )
        return;

    quint32 len;
    qToBigEndian( msg.length(), (uchar*) &len );
    m_proc.write( (const char*) &len, 4 );
    m_proc.write( msg );
}

void
ScriptResolver::handleMsg( const QByteArray& msg )
{
    if( m_deleting )
        return;

    bool ok;
    QVariant v = m_parser.parse( msg, &ok );
    if( !ok || v.type() != QVariant::Map )
    {
        Q_ASSERT(false);
        return;
    }

    QVariantMap m = v.toMap();
    QString msgtype = m.value( "_msgtype" ).toString();

    if( msgtype == "settings" )
    {
        doSetup( m );
        return;
    }
    else if( msgtype == "results" )
    {
        handleResults( m );
    }
    else if( msgtype == "loginResponse" )
    {
        handleLoginResponse( m );
    }
    else if( msgtype == "spotifyError" )
    {
        handleSpotifyError( m );
    }
    else if( msgtype == "userChanged" )
    {
        handleUserchanged( m );
    }
    else if( msgtype == "playlist" )
    {
        handlePlaylist( m );
    }
    else if( msgtype == "playlistRenamed" )
    {
        handlePlaylistRenamed( m );
    }
    else if( msgtype == "tracksAdded" )
    {
        handleTracksAdded( m );
    }
    else if( msgtype == "tracksMoved" )
    {
        handleTracksMoved( m );
    }
    else if( msgtype == "tracksRemoved" )
    {
        handleTracksRemoved( m );
    }
    else if( msgtype == "playlistDeleted" )
    {
        handlePlaylistDeleted( m );
    }
    else if( msgtype == "allPlaylists" )
    {
        handleAllPlaylists( m );
    }
    else if( msgtype.isEmpty() )
        // A query response message
    {
        handleQueryResponse( m );
    }
    else
    {
        emit customMessage( msgtype, m );
    }
}

void
ScriptResolver::procExited( int code, QProcess::ExitStatus status )
{
    m_ready = false;
    qDebug() << Q_FUNC_INFO << "RESOVER EXITED, code" << code << "status" << status << filePath();

    emit changed();

    if( m_stopped )
    {
        qDebug() << "*** Resolver stopped ";
        emit terminated();
        return;
    }
    else
    {
        qDebug() << "*** Resolver stoppped by accident, restarting...";
        startProcess();
        sendConfig();
    }
}

void
ScriptResolver::doSetup( const QVariantMap& m )
{
    m_name = m.value( "name" ).toString();
    m_timeout = m.value( "timeout", 5 ).toUInt() * 1000;

    qDebug() << "RESOLVER" << filePath() << "READY," << "name" << m_name << "timeout" << m_timeout;

    m_ready = true;
    m_configSent = false;

    emit changed();
}

void
ScriptResolver::startProcess()
{
    if( !QFile::exists( filePath() ) )
    {
        qDebug() << "*** Cannot find file" << filePath() << ", starting process failed";
        // TODO: Set error message
        return;
    }

    QFileInfo fi( filePath() );
    QString interpreter;
    QString runPath = filePath();

#ifdef Q_OS_WIN
    if( fi.suffix().toLower() != "exe" )
    {
        DWORD dwSize = MAX_PATH;

        wchar_t path[MAX_PATH] = { 0 };
        wchar_t *ext = (wchar_t *) ("." + fi.suffix()).utf16();

        HRESULT hr = AssocQueryStringW(
                (ASSOCF) 0,
                ASSOCSTR_EXECUTABLE,
                ext,
                L"open",
                path,
                &dwSize
        );

        if ( ! FAILED( hr ) )
        {
            interpreter = QString( "\"%1\"" ).arg(QString::fromUtf16((const ushort *) path));
        }
    }
    else
    {
        runPath = QString( "\"%1\"" ).arg( filePath() );
    }
#endif // Q_OS_WIN
    if( interpreter.isEmpty() )
    {
        m_proc.start( runPath );
    }
    else
    {
        m_proc.start( interpreter, QStringList() << filePath() );
    }

    sendConfig();
}

void
ScriptResolver::stop()
{
    m_stopped = true;
    //TODO: Notify Amarok to disable the Spotify plugin
    // The external Spotify resolver should not be terminated 
}

void
ScriptResolver::handlePlaylistReceived( const QVariantMap& map )
{
}

void 
ScriptResolver::handlePlaylistRenamed( const QVariantMap& map )
{
}

void 
ScriptResolver::handlePlaylistDeleted( const QVariantMap& map )
{
}

void 
ScriptResolver::handleTracksAdded( const QVariantMap& map )
{
}

void 
ScriptResolver::handleTracksDeleted( const QVariantMap& map )
{
}

void 
ScriptResolver::handleTracksMoved( const QVariantMap& map )
{
}

void 
ScriptResolver::handleTracksRemoved( const QVariantMap& map )
{
}

void
ScriptResolver::handleLoginResponse( const QVariantMap& map )
{
}

void 
ScriptResolver::handleCredentialsReceived( const QVariantMap& map )
{
}

void 
ScriptResolver::handleSettingsReceived( const QVariantMap& map )
{
}

void
ScriptResolver::handleAllPlaylists( const QVariantMap& map )
{
}

void 
ScriptResolver::handleUserchanged( const QVariantMap& map )
{
}

void
ScriptResolver::handleResults( const QVariantMap& map )
{
}

void 
ScriptResolver::handleSpotifyError( const QVariantMap& map )
{
}
