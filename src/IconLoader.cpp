/***************************************************************************
 *   Copyright (C) 2006 by Mark Kretschmann <markey@web.de>                *
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

#include "amarok.h"
#include "amarokconfig.h"

#include <qmap.h>
#include <qapplication.h>


QString
Amarok::icon( const QString& name ) //declared in amarok.h
{
    bool isRTL = QApplication::isRightToLeft();

    // We map our Amarok icon theme names to system icons, instead of using the same
    // naming scheme. This has two advantages:
    // 1. Our icons can have simpler and more meaningful names
    // 2. We can map several of our icons to one system icon, if necessary
    static QMap<QString, QString> iconMap;

    if( iconMap.empty() ) {
        iconMap["add_lyrics"]           = "list-add-amarok";
        iconMap["add_playlist"]         = "list-add-amarok";
        iconMap["add_collection"]       = "list-add-amarok";
        iconMap["album"]                = "media-optical-audio-amarok";
        iconMap["artist"]               = "view-media-artist-amarok";
        iconMap["audioscrobbler"]       = "audioscrobbler-amarok";
        iconMap["love"]                 = "love-amarok";
        iconMap["back"]                 = "media-skip-backward-amarok";
        iconMap["burn"]                 = "tools-media-optical-burn-amarok";
        iconMap["change_language"]      = "preferences-desktop-locale-amarok";
        iconMap["clock"]                = "view-history-amarok";
        iconMap["collection"]           = "collection-amarok";
        iconMap["configure"]            = "configure-amarok";
        iconMap["covermanager"]         = "covermanager-amarok";
        iconMap["device"]               = "multimedia-player-amarok";
        iconMap["download"]             = "get-hot-new-stuff-amarok";
        iconMap["dynamic"]              = "dynamic-amarok";
        iconMap["edit_properties"]      = "document-properties-amarok";
        iconMap["editcopy"]             = "edit-copy-amarok";
        iconMap["equalizer"]            = "view-media-equalizer-amarok";
        iconMap["external"]             = "system-run-amarok";
        iconMap["fastforward"]          = "media-seek-forward-amarok";
        iconMap["favourite_genres"]     = "system-file-manager-amarok";
        iconMap["files"]                = "folder-amarok";
        iconMap["files2"]               = "folder-red-amarok";
        iconMap["info"]                 = "help-about-amarok";
        iconMap["lyrics"]               = "view-media-lyrics-amarok";
        iconMap["magnatune"]            = "services-amarok";
        iconMap["mostplayed"]           = "favorites-amarok";
        iconMap["music"]                = "x-media-podcast-amarok";
        iconMap["next"]                 = "media-skip-forward-amarok";
        iconMap["pause"]                = "media-playback-pause-amarok";
        iconMap["play"]                 = "media-playback-start-amarok";
        iconMap["playlist"]             = "view-media-playlist-amarok";
        iconMap["playlist_clear"]       = "edit-clear-list-amarok";
        iconMap["playlist_refresh"]     = "view-refresh-amarok";
        iconMap["queue"]                = "go-bottom-amarok";
        iconMap["queue_track"]          = isRTL ? "go-previous" : "go-next-amarok"; // todo jpetso: move-left/right?
        iconMap["dequeue_track"]        = isRTL ? "go-next" : "go-previous-amarok"; // todo jpetso: move-left/right?
        iconMap["random"]               = "media-playlist-shuffle-amarok";
        iconMap["random_album"]         = "media-optical-audio-shuffle-amarok"; // please request a separate icon and call it "media-album-shuffle". the current name at least falls back to the audio CD icon.
        iconMap["random_no"]            = "go-next-amarok";
        iconMap["random_track"]         = "media-playlist-shuffle-amarok";
        iconMap["redo"]                 = "edit-redo-amarok";
        iconMap["refresh"]              = "view-refresh-amarok";
        iconMap["remove"]               = "edit-delete-amarok";
        iconMap["remove_from_playlist"] = "list-remove-amarok";
        iconMap["rename"]               = "edit-rename-amarok";
        iconMap["repeat_album"]         = "media-optical-audio-repeat-amarok"; // please request a separate icon and call it "media-album-repeat". the current name at least falls back to the audio CD icon.
        iconMap["repeat_no"]            = "go-down-amarok";
        iconMap["repeat_playlist"]      = "media-playlist-repeat-amarok";
        iconMap["repeat_track"]         = "audio-x-generic-repeat-amarok"; // please request a separate icon and call it "media-track-repeat". the current name at least falls back to the audio file icon.
        iconMap["rescan"]               = "view-refresh-amarok";
        iconMap["rewind"]               = "media-seek-backward-amarok";
        iconMap["save"]                 = "document-save-amarok";
        iconMap["scripts"]              = "preferences-plugin-script-amarok";
        iconMap["search"]               = "edit-find-amarok";
        iconMap["settings_engine"]      = "amarok-amarok";
        iconMap["settings_general"]     = "preferences-other-amarok";
        iconMap["settings_indicator"]   = "preferences-desktop-display-amarok";
        iconMap["settings_playback"]    = "preferences-desktop-sound-amarok";
        iconMap["settings_view"]        = "preferences-desktop-theme-amarok";
        iconMap["statistics"]           = "view-statistics-amarok";
        iconMap["stop"]                 = "media-playback-stop-amarok";
        iconMap["podcast"]              = "x-media-podcast-amarok";
        iconMap["track"]                = "audio-x-generic-amarok";
        iconMap["undo"]                 = "edit-undo-amarok";
        iconMap["visualizations"]       = "view-media-visualization-amarok";
        iconMap["zoom"]                 = "zoom-in-amarok";
    }

    static QMap<QString, QString> amarokMap;
    if( amarokMap.empty() ) {
        amarokMap["queue_track"]          = "fastforward-amarok";
        amarokMap["dequeue_track"]        = "rewind-amarok";
    }

    if( iconMap.contains( name ) )
    {
        if( AmarokConfig::useCustomIconTheme() )
        {
            if( amarokMap.contains( name ) )
                return QString( "amarok_" ) + amarokMap[name];
            return QString( "amarok_" ) + name;
        }
        else
            return iconMap[name];
    }

    return name;
}


