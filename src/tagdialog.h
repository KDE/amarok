// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>
// See COPYING file for licensing information.

#ifndef AMAROK_TAGDIALOG_H
#define AMAROK_TAGDIALOG_H

#include "config.h"

#include "ktrm.h"
#include "metabundle.h"       //stack alloc
#include "tagdialogbase.h"    //baseclass
#include "qwidget.h"

#include <kurl.h>             //stack alloc
#include <qdatetime.h>
#include <qmap.h>


class PlaylistItem;

class TagDialog : public TagDialogBase
{
    Q_OBJECT

    public:

        enum Changes { NOCHANGE=0, SCORECHANGED=1, TAGSCHANGED=2, LYRICSCHANGED=4 };

        TagDialog( const KURL& url, QWidget* parent = 0 );
        TagDialog( const KURL::List list, QWidget* parent = 0 );
        TagDialog( const MetaBundle& mb, PlaylistItem* item, QWidget* parent = 0 );
        ~TagDialog();
        friend class TagSelect;

    signals:
        void lyricsChanged( const QString& );


    private slots:
        void accept();
        void cancelPressed();
        void openPressed();
        void previousTrack();
        void nextTrack();
        void checkModified();

        void musicbrainzQuery();
        void queryDone( KTRMResultList results );
        void fillSelected( KTRMResult selected );

    private:
        void init();
        void readTags();
        void readMultipleTracks();
        void setMultipleTracksMode();
        bool hasChanged();
        int changes();
        void storeTags();
        void saveTags();
        void saveMultipleTracks();
        bool writeTag( MetaBundle mb, bool updateCB=true );
        const QString unknownSafe( QString );
        const QStringList statisticsData();

        MetaBundle m_bundle;
        int m_score;
        int m_playcount;
        QDateTime m_firstPlay;
        QDateTime m_lastPlay;
        PlaylistItem* m_playlistItem;
        QMap<QString, MetaBundle> storedTags;
        QMap<QString, int> storedScores;
        QMap<QString, QString> storedLyrics;
        KURL::List m_urlList;
        QString m_buttonMbText;
        QString m_path;
        QString m_currentCover;
        QString m_mbTrack;
};


#endif /*AMAROK_TAGDIALOG_H*/

