/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
 *      Jono Cole, Last.fm Ltd <jono@last.fm>                              *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef WEB_SERVICE_REQUEST_H
#define WEB_SERVICE_REQUEST_H

#include "metadata.h"
#include "fwd.h"
#include "StationUrl.h"
#include "CachedHttp.h"
#include "UnicornCommon.h"
#include "WeightedStringList.h"

#include <QTimer>
#include <QHttpHeader>

//TODO mxcl check error handling, since that was the point of all this
//TODO escape query paramaeters in paths in get function
//TODO intercept the response header, error on anything but status code 200
//TODO do all things need to be url percent encoded for get?
//TODO metadata timeout stuff used to have a progressively increasing timer


 /**
  * @author <max@last.fm>
  * @short Abstract base class for requests to the last.fm WebService
  *
  * rationale: free error handling for basic types, avoids some repeated code
  * encourages more thorough error handling through error( Type ) signal
  *
  * Reimplement Request, reimplement start() and success().
  * 
  * After success or failure The::webService() will alert the rest of the app
  * of the result. You can connect to that or to the instantiation result()
  * signal and process the data there.
  */

class UNICORN_DLLEXPORT Request : public QObject
{
    Q_OBJECT

public:

    virtual ~Request();

    RequestType type() const { return m_type; }

    WebRequestResultCode resultCode() const { return m_result; }

    /**
     * This function provides access to the HTTP response header status code
     * (200, 404, 503) etc should client code need to look at it.
     */
    int responseHeaderCode() const { return m_responseHeaderCode; }

    bool failed() const { return m_result != Request_Success; }
    bool succeeded() const { return m_result == Request_Success; }
    bool aborted() const { return m_result == Request_Aborted; }

    QString errorMessage() const;

    /// default is true
    void setAutoDelete( bool b ) { m_auto_delete = b; }
    bool autoDelete() const { return m_auto_delete; }

    // This needs to be called by the Handshake request
    static void setBaseHost( QString host ) { m_baseHost = host; }
    static QString baseHost() { return m_baseHost; }

    /// The application needs to call this at initialisation time.
    static void setLanguage( QString l ) { s_language = l; }
    static QString language() { return s_language; }

public slots:
    /**
     * reimplement and make a call to get() or request()
     * this function may be called multiple times, so do
     * initialisations, etc. in the ctor
     */
    virtual void start() = 0;

    /// result will be set to ResultAborted, but the abort is asynchronous
    void abort();

    /// tries again in increasing time intervals
    /// user gets a status message
    void tryAgain();

signals:
    /**
     * you can connect to this or to the generic WebService version
     */
    void result( Request* );

protected:
    /**
    * protected - this is an abstract base class
    * The::webService() owns the Request, and will handle deletion
    *
    * @p type add a value to the enum in fwd.h
    * @p name set to same as class name or something
    */
    Request( RequestType type,
             const char *name,
             CachedHttp::ProxyOverrideState proxyOverride = CachedHttp::NONE );

    /**
     * We successfully received a header from Last.fm.
     * If you're interested in specific header status codes, reimplement
     * this function in the subclass and return true to indicate you have
     * handled the header. This default implementaion returns false which
     * causes some default error handling of http header codes to take place.
     */
    virtual bool headerReceived( const class QHttpResponseHeader& )
    {
        return false;
    }

    /**
     * We successfully received data from Last.fm.
     * Reimplement and parse the data. After this function is called,
     * emit result( this ) will be called and then The::webService() will
     * receive this object, and it will emit The::webService()->result()
     * You may still call setFailed() inside your implementation to classify the
     * request as failed.
     */
    virtual void success( QByteArray /* data */ ) {}

    /// will do QHttp::setHost( @p path, @p port )
    void setHost( QString path, int port = 80 );

    /// will do QHttp::get( @p path )
    void get( QString path );

    void post( QString path, QByteArray& data );
    void post( QHttpRequestHeader& header, QByteArray& data );

    /// performs the XmlRpc request
    void request( const class XmlRpc& );

    /// calls to failed() will return true
    void setFailed( WebRequestResultCode type, QString message = "" )
    {
        m_result = type;
        if ( !message.isEmpty() )
        {
            m_error = message;
        }
    }

    void setOverrideCursor();

    /// archaic, don't use
    QString parameter( QString key, QString data );

    CachedHttp* http() { return m_http; } //FIXME: should we add this one?
    QTimer& timeoutTimer() { return m_timeout_timer; } //and this one?

private:
    PROP_GET( QByteArray, data )

    CachedHttp* m_http;

    QString m_error;

    WebRequestResultCode m_result;
    int m_responseHeaderCode;

    bool m_auto_delete;

    RequestType m_type;

    QTimer m_timeout_timer;
    QTimer m_retry_timer;

    bool m_override_cursor;

    static QString m_baseHost;

    static QString s_language;

private slots:
    void onHeaderReceivedPrivate( const class QHttpResponseHeader& header );
    void onFailurePrivate( int error_code, const QString &error_string );
    void onSuccessPrivate( QByteArray data );
};


/**
 * @author <max@last.fm>
 * @short Verify with server that a supplied user/pass combo is valid. Password
 * should be MD5 hashed. */
class UNICORN_DLLEXPORT VerifyUserRequest : public Request
{
    PROP_GET_SET( QString, username, Username )
    PROP_GET_SET( QString, passwordMd5, PasswordMd5 )
    PROP_GET_SET( QString, passwordMd5Lower, PasswordMd5Lower )

    PROP_GET( BootStrapCode, bootStrapCode )
    PROP_GET( UserAuthCode, userAuthCode )

public:
    VerifyUserRequest();

    virtual void start();
    virtual void success( QByteArray data );
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT RecommendRequest : public Request
{
    QString m_target_username;
    QString m_message;
    QString m_artist;
    QString m_album;
    QString m_track;
    QString m_token;
    int m_type;

    PROP_GET_SET( QString, language, Language )

public:
    RecommendRequest();
    RecommendRequest( Track, QString username ); ///convenience
    RecommendRequest( const QMimeData*, QString username ); ///convenience

    void setType( int t ) { m_type = t; }
    int type() { return m_type; }
    void setMessage( QString message ) { m_message = message; }
    void setArtist( QString artist ) { m_artist = artist; }
    void setToken( QString token ) { m_token = token; }
    void setTargetUsername( QString username ) { m_target_username = username; }
    QString targetUsername() { return m_target_username; }

    QString artist() { return m_artist; }
    QString album() { return m_album; }
    QString track() { return m_track; }

    virtual void start();
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT RecentTracksRequest : public Request
{
    PROP_GET( QList<Track>, tracks )

protected:
    RecentTracksRequest( RequestType type, const char *key );

    const char *m_key;

public:
    RecentTracksRequest();

    virtual void start();
    virtual void success( QByteArray data );
};

class UNICORN_DLLEXPORT RecentlyBannedTracksRequest : public RecentTracksRequest 
{
public:
    RecentlyBannedTracksRequest()
            : RecentTracksRequest( TypeRecentlyBannedTracks, "RecentlyBannedTracks" )
    {
        m_key = "recentbannedtracks";
    } 
};

class UNICORN_DLLEXPORT RecentlyLovedTracksRequest : public RecentTracksRequest 
{
public:
    RecentlyLovedTracksRequest() 
            : RecentTracksRequest( TypeRecentlyLovedTracks, "RecentlyLovedTracks" )
    {
        m_key = "recentlovedtracks";
    } 
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT DeleteFriendRequest : public Request
{
    QString m_friend_username;

public:
    DeleteFriendRequest( QString target_user );

    QString deletedUsername() const { return m_friend_username; }

    virtual void start();
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT ChangeStationRequest : public Request
{
    Q_OBJECT

    PROP_GET_SET( StationUrl, stationUrl, StationUrl )
    PROP_GET_SET( QString, session, Session )
    PROP_GET_SET( QString, basePath, BasePath )
    PROP_GET_SET( QString, language, Language )

    PROP_GET( QString, stationName )
    PROP_GET( bool, hasXspf )
    PROP_GET( QByteArray, xspf )
    PROP_GET( bool, discoverable )

public:
    ChangeStationRequest();

    void setId( int id )
    {
        m_stationUrl = StationUrl( "lastfm://play/tracks/" + QString::number( id ) );
    }

    virtual void start();
    virtual void success( QByteArray data );
};


/** @author <chris@last.fm> */

class UNICORN_DLLEXPORT ReportRebufferingRequest : public Request
{
    PROP_GET_SET( QString, streamerHost, StreamerHost )
    PROP_GET_SET( QString, userName, UserName )

public:
    ReportRebufferingRequest();

    virtual void start();
    virtual void success( QByteArray data );
};


/** @author <max@last.fm> */

#include "TrackInfo.h" //FIXME

class UNICORN_DLLEXPORT ActionRequest : public Request
{
    PROP_GET_SET( QString, artist, Artist )
    PROP_GET_SET( QString, title, Title )

    const char * const m_method;

protected:
    ActionRequest( const char *method, RequestType t );

public:
    virtual void start();

    Track track() const { Track t; t.setArtist( m_artist ); t.setTitle( m_title ); return t; }    
};

#define CLASSM( method, Name ) class Name##Request : public ActionRequest { \
        public: Name##Request( Track track ) : ActionRequest( method, Type##Name ) { setArtist( track.artist() ); setTitle( track.title() ); } \
        public: Name##Request( TrackInfo track ) : ActionRequest( method, Type##Name ) { setArtist( track.artist() ); setTitle( track.track() ); } }
    CLASSM( "removeRecentlyListenedTrack", UnListen );
    CLASSM( "loveTrack", Love );
    CLASSM( "unLoveTrack", UnLove );
    CLASSM( "banTrack", Ban );
    CLASSM( "unBanTrack", UnBan );
    CLASSM( "addTrackToUserPlaylist", AddToMyPlaylist );
#undef CLASSM


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT FriendsRequest : public Request
{
    PROP_GET_SET( QString, username, Username )
    PROP_GET( QStringList, usernames )
    PROP_GET_SET( ImageSize, imageSize, ImageSize )

    QMap<QString, QString> m_avatars;

public:
    FriendsRequest();

    QMap<QString, QString> avatars() const { return m_avatars; }

    virtual void start();
    virtual void success( QByteArray data );
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT UserPicturesRequest : public Request
{
    PROP_GET_SET( QStringList, names, Names )
    PROP_GET_SET( ImageSize, imageSize, ImageSize )

    QMap<QString, QString> m_urls;

public:
    UserPicturesRequest();

    QMap<QString, QString> urls() const { return m_urls; }

    virtual void start();
    virtual void success( QByteArray data );
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT NeighboursRequest : public Request
{
    PROP_GET_SET( QString, username, Username ) //TODO mxcl all Requests should have a target username? or maybe a base for those
    PROP_GET( WeightedStringList, usernames )
    PROP_GET_SET( ImageSize, imageSize, ImageSize )

    QMap<QString, QString> m_avatars;

public:
    NeighboursRequest();

    QMap<QString, QString> avatars() const { return m_avatars; }

    virtual void start();
    virtual void success( QByteArray data );
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT SimilarArtistsRequest : public Request
{
    PROP_GET( QString const, artist )
    PROP_GET( WeightedStringList, artists )
    PROP_GET( QStringList, images )

public:
    SimilarArtistsRequest( QString artist );

    virtual void start();
    virtual void success( QByteArray data );
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT TagsRequest : public Request
{
public:
    WeightedStringList tags() const { return m_tags; }

protected:
    TagsRequest( RequestType type, const char *name )
            : Request( type, name )
    {}

    WeightedStringList m_tags;
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT UserTagsRequest : public TagsRequest
{
    PROP_GET_SET( QString, username, Username )

protected:
    /// for reimplementations
    UserTagsRequest( RequestType type, const char *name )
            : TagsRequest( type, name )
    {}

    virtual QString path() const;

public:
    UserTagsRequest();

    virtual void start();
    virtual void success( QByteArray data );
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT UserArtistTagsRequest : public UserTagsRequest
{
    PROP_GET_SET( QString, artist, Artist )

protected:
    /// for reimplementations
    UserArtistTagsRequest( RequestType type, const char *name )
            : UserTagsRequest( type, name )
    {}

public:
    UserArtistTagsRequest()
            : UserTagsRequest( TypeUserArtistTags, "UserArtistTags" )
    {}

    virtual QString path() const
    {
        return "/artisttags.xml?artist=" + QUrl::toPercentEncoding( m_artist );
    }
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT UserAlbumTagsRequest : public UserArtistTagsRequest
{
    PROP_GET_SET( QString, album, Album )

public:
    UserAlbumTagsRequest()
            : UserArtistTagsRequest( TypeUserAlbumTags, "UserAlbumTags" )
    {}

    virtual QString path() const
    {
        return "/albumtags.xml?artist=" + QUrl::toPercentEncoding( artist() ) +
               "&album=" + QUrl::toPercentEncoding( m_album );
    }
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT UserTrackTagsRequest : public UserArtistTagsRequest
{
    PROP_GET_SET( QString, track, Track )

public:
    UserTrackTagsRequest()
            : UserArtistTagsRequest( TypeUserTrackTags, "UserTrackTags" )
    {}
    
    virtual QString path() const
    {
        return "/tracktags.xml?artist=" + QUrl::toPercentEncoding( artist() ) +
               "&track=" + QUrl::toPercentEncoding( m_track );
    }    
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT TopTagsRequest : public TagsRequest
{
public:
    TopTagsRequest();

    virtual void start();
    virtual void success( QByteArray data );    
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT ArtistTagsRequest : public TagsRequest
{
    PROP_GET_SET( QString, artist, Artist )
    
public:
    ArtistTagsRequest( QString artist = "" );

    virtual void start();
    virtual void success( QByteArray data );
};

/** @author <petgru@last.fm> */

class UNICORN_DLLEXPORT TrackTagsRequest : public ArtistTagsRequest
{
    PROP_GET_SET( QString, track, Track )
    
public:
    TrackTagsRequest( QString artist = "", QString track = "" );

    virtual void start();
    virtual void success( QByteArray data );
};

/** @author <petgru@last.fm> */

class UNICORN_DLLEXPORT AlbumTagsRequest : public ArtistTagsRequest
{
    PROP_GET_SET( QString, album, Album )
    
public:
    AlbumTagsRequest( QString artist = "", QString album = "" );

    virtual void start();
    virtual void success( QByteArray data );
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT SimilarTagsRequest : public TagsRequest
{
    PROP_GET_SET( QString, tag, Tag )

public:
    SimilarTagsRequest( QString tag );
    
    virtual void start();
    virtual void success( QByteArray data );
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT SearchTagRequest : public TagsRequest
{
    PROP_GET_SET( QString, tag, Tag )
    
public:
    SearchTagRequest();

    virtual void start();
    virtual void success( QByteArray data );
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT SetTagRequest : public Request
{
    int m_type;
    int m_mode;
    QStringList m_tags;
    
    QString m_username;
    
    QString m_artist;
    QString m_token;
    
    QString m_track;
    QString m_album;

public:
    SetTagRequest();
    
    static SetTagRequest* append( Track, QString tag );
    static SetTagRequest* append( const QMimeData*, QString tag );
    
    void setType( int type ) { m_type = type; }
    void setMode( int mode ) { m_mode = mode; }
    void setTag( QString tag ) { m_tags = tag.split( "," ); }
    void setArtist( QString artist ) { m_artist = artist; }
    void setToken( QString token ) { m_token = token; }

    QString artist() const { return m_artist; }
    QString title() const;

    virtual void start();
    virtual void success( QByteArray data );
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT RadioMetaDataRequest : public Request
{
    PROP_GET( QString, stationFeed )
    PROP_GET( QString, stationName )
       
public:
    RadioMetaDataRequest();

    MetaData const &metaData() const;

    virtual void start();
    virtual void success( QByteArray data );
};


class UNICORN_DLLEXPORT ArtistMetaDataRequest : public Request
{
    PROP_GET_SET( QString, artist, Artist )
    PROP_GET_SET( QString, language, Language )

    PROP_GET( MetaData, meta_data )
    
public:
    ArtistMetaDataRequest();

    virtual void start();
    virtual void success( QByteArray data );
    
    MetaData const &metaData() { return m_meta_data; }
};


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT TrackMetaDataRequest : public Request
{
    PROP_GET_SET( MetaData, track, Track )
    PROP_GET_SET( QString, language, Language )

public:
    TrackMetaDataRequest();

    /// convenience
    MetaData const &metaData() const { return m_track; }

    virtual void start();
    virtual void success( QByteArray data );
};


/** Simple Request to test proxy settings */

/** @author <jono@last.fm> */

class UNICORN_DLLEXPORT ProxyTestRequest : public Request
{
    Q_OBJECT

public:
    ProxyTestRequest( bool proxyUsed );

    virtual void start();

    bool proxyUsed(){ return m_proxyUsed; }

private:
    bool m_proxyUsed;

private slots:    
        virtual void success( QByteArray data );
};

/** Very Simple Requests */

/** @author <max@last.fm> */

class UNICORN_DLLEXPORT Handshake : public Request
{
    Q_OBJECT
    
    PROP_GET_SET( QString, username, Username )
    PROP_GET_SET( QString, password, Password )
    PROP_GET_SET( QString, version, Version )
    PROP_GET_SET( QString, language, Language )
    PROP_GET_SET( QString, fingerprintUploadUrl, FingerprintUploadUrl )
    
    PROP_GET( QString, session )
    PROP_GET( QString, baseHost )
    PROP_GET( QString, basePath )
    PROP_GET( QUrl, streamUrl )
    PROP_GET( bool, isSubscriber )
    PROP_GET( QString, message )

    bool m_permitBootstrap;

public:
    Handshake();
    
    bool isMessage() const { return !m_message.isEmpty(); }
    bool isBootstrapPermitted() const { return m_permitBootstrap; }

    virtual void start();

private slots:    
    virtual void success( QByteArray data );
};

// EJ: I like having separate header files as my header/cpp switch macro
// won't work otherwise :'(
#include "GetXspfPlaylistRequest.h"
#include "TrackToIdRequest.h"

#endif
