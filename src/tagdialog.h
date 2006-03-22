// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>
// See COPYING file for licensing information.

#ifndef AMAROK_TAGDIALOG_H
#define AMAROK_TAGDIALOG_H

#include "config.h"

#include "ktrm.h"
#include "metabundle.h"       //stack alloc
#include "tagdialogbase.h"    //baseclass

#include <kurl.h>             //stack alloc
#include <qmap.h>


class PlaylistItem;

class TagDialog : public TagDialogBase
{
    Q_OBJECT

    public:

        enum Changes { NOCHANGE=0, SCORECHANGED=1, TAGSCHANGED=2 };

        TagDialog( const KURL& url, QWidget* parent = 0 );
        TagDialog( const KURL::List list, QWidget* parent = 0 );
        TagDialog( const MetaBundle& mb, PlaylistItem* item, QWidget* parent = 0 );
        friend class TagSelect;

    private slots:
        void accept();
        void cancelPressed();
        void openPressed();
        void previousTrack();
        void nextTrack();
        void checkModified();

        void musicbrainzQuery();
        void queryDone( KTRMResultList results, QString error );
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

        MetaBundle m_bundle;
        int m_score;
        int m_playcount;
        PlaylistItem* m_playlistItem;
        QMap<QString, MetaBundle> storedTags;
        QMap<QString, int> storedScores;
        KURL::List m_urlList;
        QString m_buttonMbText;
        QString m_path;
        QString m_currentCover;
        QString m_mbTrack;
};


#endif /*AMAROK_TAGDIALOG_H*/

