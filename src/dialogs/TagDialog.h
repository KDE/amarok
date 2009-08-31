/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>                         *
 * Copyright (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_TAGDIALOG_H
#define AMAROK_TAGDIALOG_H

#include <config-amarok.h>

#include "amarok_export.h"
#include "playlist/PlaylistItem.h"

#include "meta/Meta.h"

#include <khtml_part.h>
#include <KDialog>

#include <QDateTime>
#include <QLabel>
#include <QListIterator>
#include <QMap>
#include <QVariant>
#include <QtGui/QWidget>

namespace Ui
{
    class TagDialogBase;
}

class QueryMaker;
class QComboBox;

class AMAROK_EXPORT TagDialog : public KDialog, public Meta::Observer
{
    Q_OBJECT

    public:

        enum Changes { NOCHANGE=0, SCORECHANGED=1, TAGSCHANGED=2, LYRICSCHANGED=4, RATINGCHANGED=8, LABELSCHANGED=16 };
        enum Tabs { SUMMARYTAB, TAGSTAB, LYRICSTAB, STATSTAB, LABELSTAB };

        explicit TagDialog( const Meta::TrackList &tracks, QWidget *parent = 0 );
        explicit TagDialog( Meta::TrackPtr track, QWidget *parent = 0 );
        explicit TagDialog( QueryMaker *qm );
        ~TagDialog();

        void setTab( int id );

        friend class TagSelect;

        // inherited from Meta::Observer
        using Observer::metadataChanged;
        void metadataChanged( Meta::AlbumPtr album );

    signals:
        void lyricsChanged( const QString& );

    public slots:
        void openUrlRequest(const KUrl &url );

    private slots:
        void accept();
        void cancelPressed();
        void openPressed();
        void previousTrack();
        void nextTrack();
        void perTrack();
        void checkModified();

        void showCoverMenu( const QPoint &pos );
        void loadCover();

        void guessFromFilename();

        void resultReady( const QString &collectionId, const Meta::TrackList &tracks );
        void queryDone();

        void resultReady( const QString &collectionId, const Meta::AlbumList &albums );
        void resultReady( const QString &collectionId, const Meta::ArtistList &artists );
        void resultReady( const QString &collectionId, const Meta::ComposerList &composers );
        void resultReady( const QString &collectionId, const Meta::GenreList &genres );
        void dataQueryDone();
        
        //individual item-specific slots, so we know which have been changed by the user
        // useful when editing multiple tracks and we can't compare against the tag itself
        void composerModified();
        void artistModified();
        void albumModified();
        void genreModified();
        void ratingModified();
        void yearModified();
        void scoreModified();
        void commentModified();
        void discNumberModified();
        
    private:
        void init();
        void setCurrentTrack( Meta::TrackPtr track );
        void startDataQuery();
        void readTags();
        void readMultipleTracks();
        void setMultipleTracksMode();
        void setSingleTrackMode();
        void enableItems();
        bool hasChanged();
        int changes();
        void storeTags();
        void storeTags( const Meta::TrackPtr &track );
        void storeTags( const Meta::TrackPtr &track, int changes, const QVariantMap &data );
        void storeLabels( const Meta::TrackPtr &track, const QStringList &labels );
        void loadTags( const Meta::TrackPtr &track );
        void loadLyrics( const Meta::TrackPtr &track );
        void loadLabels( const Meta::TrackPtr &track );
        QVariantMap dataForTrack( const Meta::TrackPtr &track );
        double scoreForTrack( const Meta::TrackPtr &track );
        int ratingForTrack( const Meta::TrackPtr &track );
        QString lyricsForTrack( const Meta::TrackPtr &track );
        QStringList labelsForTrack( const Meta::TrackPtr &track );
        QStringList getCommonLabels();
        void saveTags();
        const QString unknownSafe( QString );
        const QStringList statisticsData();
        void applyToAllTracks();

        const QStringList filenameSchemes();

        QStringList labelListFromText( const QString &text );
        void generateDeltaForLabelList( const QStringList &list );
        QString generateHTML( const QStringList &labels );

        void selectOrInsertText( const QString &text, QComboBox *comboBox );

        QString m_lyrics;
        bool m_perTrack;
        QMap<Meta::TrackPtr, QVariantMap > m_storedTags;
        QMap<Meta::TrackPtr, double> m_storedScores;
        QMap<Meta::TrackPtr, int> m_storedRatings;
        QMap<Meta::TrackPtr, QString> m_storedLyrics;
        QMap<Meta::TrackPtr, QStringList> m_newLabels;
        QMap<Meta::TrackPtr, QStringList> m_originalLabels;
        QString m_path;
        QString m_currentCover;
        QStringList m_labels;
        QStringList m_addedLabels;
        QStringList m_removedLabels;
        QString m_commaSeparatedLabels;
        KHTMLPart *m_labelCloud;

        //2.0 stuff
        Meta::TrackList m_tracks;
        Meta::TrackPtr m_currentTrack;
        QListIterator<Meta::TrackPtr > m_trackIterator;
        QMap< QString, bool > m_fieldEdited;
        QVariantMap m_currentData;
        QueryMaker *m_queryMaker;
        QueryMaker *m_dataQueryMaker;
        QStringList m_artists;
        QStringList m_albums;
        QStringList m_composers;
        QStringList m_genres;

        Ui::TagDialogBase *ui;
};


#endif /*AMAROK_TAGDIALOG_H*/

