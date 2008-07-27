/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
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

/**
 * You can include this to get just the definitions, enums, etc. for
 * the webservice and webservice-requests
 */

//TODO mxcl feedback for ALL webservices on initiation
//TODO mxcl tagdialog, show known friends list first, as loading revised one
//TODO mxcl check if I should retry after a 200 header receipt?
//TODO feedback changes based on header receipt?

#ifndef WEB_SERVICE_FWD_H
#define WEB_SERVICE_FWD_H

#include <QString>
#include <QStringList>
#include "UnicornDllExportMacro.h"


class WebService;
class Request;
class ChangeStationRequest;
class SetTagRequest;
class Handshake;
class WebService;
class SkipRequest;
class ArtistMetaDataRequest;
class TrackMetaDataRequest;
class ActionRequest;
class VerifyUserRequest;
class UserTagsRequest;
class EnableScrobblingRequest;
class EnableDiscoveryModeRequest;
class UnListenRequest;
class LoveRequest;
class UnLoveRequest;
class BanRequest;
class UnBanRequest;
class CachedHttp;
class FriendsRequest;
class UserTagsRequest;
class UserPicturesRequest;
class GetXspfPlaylistRequest;
class ReportRebufferingRequest;


namespace The
{
    WebService* webService();
}


enum RequestType
{
    TypeHandshake,
    TypeChangeStation,
    TypeGetXspfPlaylist,
    TypeSetTag,
    TypeWebService,
    TypeSkip,
    TypeArtistMetaData,
    TypeTrackMetaData,
    TypeVerifyUser,
    TypeEnableScrobbling,
    TypeEnableDiscoveryMode,
    TypeUnListen,
    TypeAddToMyPlaylist,
    TypeLove,
    TypeUnLove,
    TypeBan,
    TypeUnBan,
    TypeFriends,
    TypeUserPictures,
    TypeRecentTracks,
    TypeRecentlyBannedTracks,
    TypeRecentlyLovedTracks,
    TypeNeighbours,
    TypeSimilarArtists,
    TypeDeleteFriend,
    TypeRecommend,
    TypeUserPicturesRequest,
    TypeReportRebuffering,
    TypeSubmitFingerprint,
    TypeFingerprintQuery,
    TypeFrikkinNorman,

    TypeUserTags,
    TypeUserArtistTags,
    TypeUserAlbumTags,
    TypeUserTrackTags,
    TypeSimilarTags,
    TypeSearchTag,
    TypeArtistTags,
    TypeTrackTags,
    TypeAlbumTags,
    TypeTopTags,

    TypeUserLabels,
    TypeTrackUpload,

    TypeProxyTest
};


enum TagMode
{
    TAG_OVERWRITE = 0, // Overwrites existing tag associations
    TAG_APPEND         // Appends a tag to the list of existing associations
};


enum UserAuthCode
{
    AUTH_OK = 0,
    AUTH_OK_LOWER,
    AUTH_BADUSER,
    AUTH_BADPASS,
    AUTH_ERROR
};


enum ImageSize
{
    SIZE_SMALL = 0,
    SIZE_MEDIUM,
    SIZE_LARGE,
    SIZE_PAGE
};


enum BootStrapCode
{
    BOOTSTRAP_DENIED = 0,
    BOOTSTRAP_ALLOWED
};


/**
 * Global enum for all web request error codes. This is so that Request*
 * object pointers can be passedup a hierarchy and still be switched on
 * specific error code.
 *
 * Admittedly, this is shit as it forces you to update this enum every time
 * you add a new request subclass and introduces ugly coupling between them,
 * but I spent considerable time thinking about a way of having each subclass
 * define its own error codes whilst still keeping them globally unique (even
 * enlisting Norman's considerable template experience), but to no avail.
 * Sometimes C++ really sucks. So global enum, here we go. :(
 */
enum WebRequestResultCode
{
    // class Request codes
    // ------------------------------------------------------------------------
    Request_Undefined,
    Request_Success,

    /// We aborted it, so the user prolly doesn't care
    Request_Aborted,

    /// DNS failed
    Request_HostNotFound,

    /// HTTP response code
    Request_BadResponseCode,

    /// We've timed out waiting for an HTTP response several times
    Request_NoResponse,

    /// Proxy authentication required, probably show proxy settings dialog
    Request_ProxyAuthenticationRequired,

    /// Authentication failed, probably show user settings dialog
    Request_WrongUserNameOrPassword,

    // class Handshake codes
    // ------------------------------------------------------------------------
    Handshake_WrongUserNameOrPassword,
    Handshake_Banned,
    Handshake_SessionFailed,

    // class ChangeStationRequest codes
    // ------------------------------------------------------------------------
    ChangeStation_NotEnoughContent,     // there is not enough content to play this station.
    ChangeStation_TooFewGroupMembers,   // this group does not have enough members for radio.
    ChangeStation_TooFewFans,           // this artist does not have enough fans for radio.
    ChangeStation_TooFewNeighbours,     // there are not enough neighbours for this radio.
    ChangeStation_Unavailable,          // this item is not available for streaming.
    ChangeStation_SubscribersOnly,      // this feature is only available to subscribers.
    ChangeStation_StreamerOffline,      // the streaming system is offline for maintenance
    ChangeStation_InvalidSession,       // session has timed out, please re-handshake
    ChangeStation_UnknownError,         // no idea

    // class GetXspfPlaylistRequest codes
    // ------------------------------------------------------------------------
    Playlist_InvalidSession,            // 401, session timed out, need to re-handshake
    Playlist_RecSysDown,                // 503, recommendation systems down, treat as connection error

    // class FingerprintQueryRequest codes
    // ------------------------------------------------------------------------
    Fingerprint_QueryError,             // no fpid was returned, just an error string

    // class <insert new class name here> codes
    // ------------------------------------------------------------------------


    /// Custom undefined error
    WebRequestResult_Custom = 1000
};


enum RadioErrorCode
{
    RADIO_ERROR_NOT_ENOUGH_CONTENT = 1, // There is not enough content to play this station.
    RADIO_ERROR_FEWGROUPMEMBERS,        // This group does not have enough members for radio.
    RADIO_ERROR_FEWFANS,                // This artist does not have enough fans for radio.
    RADIO_ERROR_UNAVAILABLE,            // This item is not available for streaming.
    RADIO_ERROR_SUBSCRIBE,              // This feature is only available to subscribers.
    RADIO_ERROR_FEWNEIGHBOURS,          // There are not enough neighbours for this radio.
    RADIO_ERROR_OFFLINE                 // The streaming system is offline for maintenance, please try again later
};

#endif
