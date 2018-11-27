/********************************************************************
 This file is part of the KDE project.

Copyright (C) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

/**
 * @file upnptypes.h
 * @brief Provides extra UDSEntry field types for UPnP specific meta-data
 */

#ifndef UPNPTYPES_H
#define UPNPTYPES_H

#include <kio/udsentry.h>

namespace KIO {

/**
* This enumeration defines some custom UDS fields to allow
* UPnP metadata to be exposed. UPnP provides a number
* of fields for meta-data which adheres to the recommended
* properties in the DLNA specification ( August 2009 )
*
* Certain fields are shared, such as UPNP_DURATION which applies
* to audioItems and videoItems.
* 
* When a resource is available, size is advertised using the
* UDS_SIZE value, there is no special UPNP_SIZE.
*
* Note: when adding new fields, remember, the restriction is 39
* fields.
*
* Very Important: It seems fields using UDS_EXTRA as a base
* can only be of type String. I don't know why since the UDSEntry
* implementation never internally uses enum values but only
* uint which should be able to fit a number too. But it doesn't seem to
* work, at least for UPNP_TRACK_NUMBER. The field is set properly,
* it is accessible within the kioslave. Even copying the entry is fine.
* 
* When emitted to the Job, the Job's UDSEntry reports as containing
* that field, but its value is reported as 0.
*/

enum UPnPFieldTypes {
    UPNP_CLASS = ( UDSEntry::UDS_EXTRA + 1 ) | UDSEntry::UDS_STRING,

    // object.item.audioItem

    UPNP_CREATOR = ( UDSEntry::UDS_EXTRA + 2 ) | UDSEntry::UDS_STRING,
    /// The Album to which the item belongs
    UPNP_ALBUM = ( UDSEntry::UDS_EXTRA + 3 ) | UDSEntry::UDS_STRING,
    /// The genre
    UPNP_GENRE = ( UDSEntry::UDS_EXTRA + 4 ) | UDSEntry::UDS_STRING,
    /// Duration of the content in "HH:MM:SS"
    UPNP_DURATION = (UDSEntry::UDS_EXTRA + 5 ) | UDSEntry::UDS_STRING,

    // object.item.imageItem

    // NOTE: for date, should we convert if the device isn't formatting properly?
    // TODO: should we use the UDS_TIME format modeled by long long
    // instead and handle the conversion?

    /// the date the image was taken. This may not be the same
    /// as the access time or file creation time and so has a
    /// separate field.
    /// Date is in ISO format, see the UPnP ContentDirectory service
    /// specification for details.
    UPNP_DATE = ( UDSEntry::UDS_EXTRA + 6 ) | UDSEntry::UDS_STRING,
    /// Image resolution, "[0-9]+x[0-9]+" in pixels
    UPNP_IMAGE_RESOLUTION = ( UDSEntry::UDS_EXTRA + 7 ) | UDSEntry::UDS_STRING,

    // object.item.videoItem
    // none unique

    // object.container.album.musicAlbum
    UPNP_ALBUM_CHILDCOUNT = ( UDSEntry::UDS_EXTRA + 8 ) | UDSEntry::UDS_NUMBER,

    // object.item.videoItem.videoBroadcast, object.item.audioItem.audioBroadcast
    UPNP_CHANNEL_NAME = ( UDSEntry::UDS_EXTRA + 9 ) | UDSEntry::UDS_STRING,
    UPNP_CHANNEL_NUMBER = ( UDSEntry::UDS_EXTRA + 10 ) | UDSEntry::UDS_NUMBER,

    /// Track number in the Album (audioItems)
    UPNP_TRACK_NUMBER = ( UDSEntry::UDS_EXTRA + 11 ) | UDSEntry::UDS_STRING,


    // These are types not recommended by DLNA but good for
    // Amarok :)
    UPNP_BITRATE = ( UDSEntry::UDS_EXTRA + 12 ) | UDSEntry::UDS_STRING,

    // and these are good for UPNP-aware applications
    // which will need to keep track of changes and so on.
    UPNP_ID = ( UDSEntry::UDS_EXTRA + 13 ) | UDSEntry::UDS_STRING,
    UPNP_PARENT_ID = ( UDSEntry::UDS_EXTRA + 14 ) | UDSEntry::UDS_STRING,

    UPNP_ALBUMART_URI = ( UDSEntry::UDS_EXTRA + 15 ) | UDSEntry::UDS_STRING,

    UPNP_ARTIST = ( UDSEntry::UDS_EXTRA + 16 ) | UDSEntry::UDS_STRING,

    UPNP_REF_ID = ( UDSEntry::UDS_EXTRA + 17 ) | UDSEntry::UDS_STRING
};
}

#endif
