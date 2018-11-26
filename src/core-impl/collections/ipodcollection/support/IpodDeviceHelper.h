/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef IPODDEVICEHELPER_H
#define IPODDEVICEHELPER_H

#include "ui_IpodConfiguration.h"
#include "amarok_export.h"
#include "core/transcoding/TranscodingConfiguration.h"

#include <QDialog>

#include <QString>

#include <gpod/itdb.h>


namespace IpodDeviceHelper {

    /**
     * Tries to parse itunes db from an iPod mounted at @param mountPoint .
     * @param errorMsg is set appropriately if error occurred
     * @return valid itdb or null, in which case @param errorMsg is not empty
     */
    AMAROK_EXPORT Itdb_iTunesDB *parseItdb( const QString &mountPoint, QString &errorMsg );

    /**
     * Return pretty iPod name usable as collection name.
     * @param itdb parsed iTunes db, may be null in which cese fallback name is used
     */
    AMAROK_EXPORT QString collectionName( Itdb_iTunesDB *itdb );

    /**
     * Return iPod name without model, or placeholder if no or empty name can be read
     */
    AMAROK_EXPORT QString ipodName( Itdb_iTunesDB *itdb );

    /**
     * Unlinks playlists and tracks from itdb so that itdb no longer frees them when it
     * itself is freed. Does nothing when itdb is null.
     */
    AMAROK_EXPORT void unlinkPlaylistsTracksFromItdb( Itdb_iTunesDB *itdb );

    /**
     * Fills in a dialog with iPod configuration.
     *
     * @param configureDialog QDialog that coutains the ui. Must not be null
     * @param configureDialogUi ui of the dialog. Must not be null
     * @param itdb itdb of the device or null if could not be parsed
     * @param transcodeConfig current transcoding configuration preference
     * @param errorMessage from parsing/initializing itsb (empty if no error)
     */
    AMAROK_EXPORT void fillInConfigureDialog( QDialog *configureDialog,
                                              Ui::IpodConfiguration *configureDialogUi,
                                              const QString &mountPoint,
                                              Itdb_iTunesDB *itdb,
                                              const Transcoding::Configuration &transcodeConfig,
                                              const QString &errorMessage = QString() );

    /**
     * Try to initialize iPod using libgpod.
     *
     * @param mountPoint mount point of an already mounted iPod
     * @param configureDialogUi configure dialog ui from which some info is gathered
     * @param errorMessage if initializing fails, errorMessage will contain problem
     *
     * @return true if initialization succeeded, false otherwise
     */
    AMAROK_EXPORT bool initializeIpod( const QString& mountPoint,
                                       const Ui::IpodConfiguration *configureDialogUi,
                                       QString &errorMessage );

    /**
     * Sets iPod name to @param name . Does nothing if @param itdb is null
     */
    AMAROK_EXPORT void setIpodName( Itdb_iTunesDB *itdb, const QString &newName );

    /**
     * Return true if it is considered safe to write to itdb. itdb can be null and this
     * function is guaranteed to return false in such case.
     *
     * @param mountPoint path to a mounted ipod
     * @param itdb iTunes database, may be null
     */
    AMAROK_EXPORT bool safeToWrite( const QString &mountPoint, const Itdb_iTunesDB *itdb );

} // namespace IpodDeviceHelper

#endif // IPODDEVICEHELPER_H
