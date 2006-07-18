// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>
// (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>
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
namespace TagLib {
    namespace ID3v2 {
        class Tag;
    }
}

class TagDialog : public TagDialogBase
{
    Q_OBJECT

    public:

        enum Changes { NOCHANGE=0, SCORECHANGED=1, TAGSCHANGED=2, LYRICSCHANGED=4, RATINGCHANGED=8 };
        enum Tabs { SUMMARYTAB, TAGSTAB, LYRICSTAB, STATSTAB };

        TagDialog( const KURL& url, QWidget* parent = 0 );
        TagDialog( const KURL::List list, QWidget* parent = 0 );
        TagDialog( const MetaBundle& mb, PlaylistItem* item, QWidget* parent = 0 );
        ~TagDialog();

        void setTab( int id );

        friend class TagSelect;

    signals:
        void lyricsChanged( const QString& );


    private slots:
        void accept();
        void cancelPressed();
        void openPressed();
        void previousTrack();
        void nextTrack();
        void perTrack();
        void checkModified();

        void loadCover( const QString &artist, const QString &album );

        void musicbrainzQuery();
        void guessFromFilename();
        void setFileNameSchemes();
        void queryDone( KTRMResultList results, QString error );
        void fillSelected( KTRMResult selected );

    private:
        void init();
        void readTags();
        void readMultipleTracks();
        void setMultipleTracksMode();
        void setSingleTrackMode();
        void enableItems();
        bool hasChanged();
        int changes();
        void storeTags();
        void storeTags( const KURL& url );
        void storeTags( const KURL& url, int changes, const MetaBundle &mb, int score = 0 );
        void loadTags( const KURL& url );
        void loadLyrics( const KURL& url );
        MetaBundle bundleForURL( const KURL &url );
        int scoreForURL( const KURL &url );
        int ratingForURL( const KURL &url );
        QString lyricsForURL( const KURL &url );
        void saveTags();
        bool writeTag( MetaBundle mb, bool updateCB=true );
        const QString unknownSafe( QString );
        const QStringList statisticsData();
        void applyToAllTracks();

        const QStringList filenameSchemes();

        MetaBundle m_bundle;
        KURL::List::iterator m_currentURL;
        int m_score;
        QString m_lyrics;
        int m_playcount;
        int m_changedCount;
        bool m_perTrack;
        QDateTime m_firstPlay;
        QDateTime m_lastPlay;
        PlaylistItem* m_playlistItem;
        QMap<QString, MetaBundle> storedTags;
        QMap<QString, int> storedScores;
        QMap<QString, int> storedRatings;
        QMap<QString, QString> storedLyrics;
        KURL::List m_urlList;
        QString m_buttonMbText;
        QString m_path;
        QString m_currentCover;
        QString m_mbTrack;
};


#endif /*AMAROK_TAGDIALOG_H*/

