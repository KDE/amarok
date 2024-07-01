/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>                         *
 * Copyright (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_TAGDIALOG_H
#define AMAROK_TAGDIALOG_H

#include <config.h>

#include "amarok_export.h"
#include "playlist/PlaylistItem.h"
#include "LabelListModel.h"

#include "core/meta/Observer.h"
#include "core/collections/MetaQueryMaker.h"

#include <QDateTime>
#include <QDialog>
#include <QLabel>
#include <QMap>
#include <QSet>
#include <QVariant>
#include <QWidget>

namespace Ui
{
    class TagDialogBase;
}

class QComboBox;

class AMAROK_EXPORT TagDialog : public QDialog, public Meta::Observer
{
    Q_OBJECT

    public:

        enum Tabs { SUMMARYTAB, TAGSTAB, LYRICSTAB, LABELSTAB };

        explicit TagDialog( const Meta::TrackList &tracks, QWidget *parent = nullptr );
        explicit TagDialog( Meta::TrackPtr track, QWidget *parent = nullptr );
        explicit TagDialog( Collections::QueryMaker *qm );
        ~TagDialog() override;

        // inherited from Meta::Observer
        using Observer::metadataChanged;
        void metadataChanged( const Meta::AlbumPtr &album ) override;

    private Q_SLOTS:
        void accept() override;
        void cancelPressed();
        void openPressed();
        void previousTrack();
        void nextTrack();
        void perTrack( bool );
        void checkChanged();

        /**
        *   removes selected label from list
        */
        void removeLabelPressed();

        /**
        *   adds label to list
        */
        void addLabelPressed();

        void showCoverMenu( const QPoint &pos );

        /**
        *   Shows FileNameLayoutDialog to guess tags from filename
        */
        void guessFromFilename();

        void musicbrainzTagger();
        void musicbrainzTaggerResult( const QMap<Meta::TrackPtr, QVariantMap > &result );

        /** Safely adds a track to m_tracks.
            Ensures that tracks are not added twice.
        */
        void addTrack( Meta::TrackPtr &track );

        void tracksReady( const Meta::TrackList &tracks );
        void queryDone();

        void albumsReady( const Meta::AlbumList &albums );
        void artistsReady( const Meta::ArtistList &artists );
        void composersReady( const Meta::ComposerList &composers );
        void genresReady( const Meta::GenreList &genres );
        /**
        *   Updates global label list by querying all collections for all existing labels.
        */
        void labelsReady( const Meta::LabelList &labels );
        void dataQueryDone();

        /**
        * Updates Add label button
        */
        void labelModified();

        /**
        * Updates Remove label button
        */
        void labelSelected();

    private:
        /** Sets some further properties and connects all the signals */
        void initUi();

        /** Set's the current track to the number.
            Will check against invalid numbers, so the caller does not have to do that.
        */
        void setCurrentTrack( int num );

        /** Start a query maker for the given query type */
        void startDataQuery( Collections::QueryMaker::QueryType type, const QMetaMethod &signal, const QMetaMethod &slot );

        /** Start queries for artists, albums, composers, genres and labels to fill out the combo boxes */
        void startDataQueries();

        /** Sets the tags in the UI, cleaning unset tags */
        void setTagsToUi( const QVariantMap &tags );

        /** Sets the tags in the UI, cleaning unset tags depending on m_perTrack */
        void setTagsToUi();

        /** Gets the changed tags from the UI */
        QVariantMap getTagsFromUi( const QVariantMap &tags ) const;

        /** Gets all the needed tags (just the one that we display or edit) from the track */
        QVariantMap getTagsFromTrack( const Meta::TrackPtr &track ) const;

        /** Gets a summary of all the tags from m_tracks */
        QVariantMap getTagsFromMultipleTracks() const;

        /** Overwrites all values in the stored tags with the new ones. Tags not in "tags" map are left unchanged. */
        void setTagsToTrack( const Meta::TrackPtr &track, const QVariantMap &tags );

        /** Overwrites all values in the stored tags with the new ones.
            Tags not in "tags" map are left unchanged.
            Exception are labels which are not set.
        */
        void setTagsToMultipleTracks( QVariantMap tags );

        /** Smartly writes back tags data depending on m_perTrack */
        void setTagsToTrack();

        /** Sets the UI to edit either one or the complete list of tracks.
            Don't forget to save the old ui values and set the new tags afterwards.
        */
        void setPerTrack( bool isEnabled );

        void updateButtons();
        void updateCover();
        void setControlsAccessability();

        /** Writes all the tags to all the tracks.
            This finally updates the Meta::Tracks
        */
        void saveTags();

        /**
        * Returns "Unknown" if the value is null or not known
        * Otherwise returns the string
        */
        const QString unknownSafe( const QString &s ) const;
        const QString unknownSafe( int i ) const;

        const QStringList filenameSchemes();

        void selectOrInsertText( const QString &text, QComboBox *comboBox );

        QString m_path; // the directory of the current track/tracks

        LabelListModel *m_labelModel; //!< Model MVC Class for Track label list

        bool m_perTrack;
        Meta::TrackList m_tracks;
        Meta::TrackPtr m_currentTrack;
        Meta::AlbumPtr m_currentAlbum;
        int m_currentTrackNum;

        /** True if m_storedTags contains changed.
            The pushButton_ok will be activated if this one is true and the UI
            has further changes
        */
        bool m_changed;

        /** The tags for the tracks.
            If the tags are edited then this structure is updated when switching
            between single and multiple mode or when pressing the save button.
        */
        QMap<Meta::TrackPtr, QVariantMap > m_storedTags;

        // the query maker to get the tracks to be edited
        Collections::QueryMaker *m_queryMaker;

        QSet<QString> m_artists;
        QSet<QString> m_albums;
        QSet<QString> m_albumArtists;
        QSet<QString> m_composers;
        QSet<QString> m_genres;
        QSet<QString> m_allLabels; //! all labels known to currently active collections, used for autocompletion

        Ui::TagDialogBase *ui;
};


#endif /*AMAROK_TAGDIALOG_H*/

