// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>
// See COPYING file for licensing information.

#ifndef AMAROK_TAGDIALOG_H
#define AMAROK_TAGDIALOG_H

#include "config.h"

#include "metabundle.h"       //stack alloc
#include "tagdialogbase.h"    //baseclass

#include "ktrm.h"

#include <kurl.h>             //stack alloc
#include <qmap.h>

class PlaylistItem;


class TagDialog : public TagDialogBase
{
    Q_OBJECT

    public:
        TagDialog( const KURL& url, QWidget* parent = 0 );
        TagDialog( const KURL::List list, QWidget* parent = 0 );
        TagDialog( const MetaBundle& mb, PlaylistItem* item, QWidget* parent = 0 );

    private slots:
        void accept();
        void cancelPressed();
        void openPressed();
        void previousTrack();
        void nextTrack();
        void checkModified();

        void musicbrainzQuery();
        void queryDone( KTRMResultList results );

    private:
        void init();
        void readTags();
        void setMultipleTracksMode();
        bool hasChanged();
        void storeTags();
        void saveTags();
        void saveMultipleTracks();
        bool writeTag( MetaBundle mb, bool updateCB=true );

        MetaBundle m_bundle;
        PlaylistItem* m_playlistItem;
        QMap<QString, MetaBundle> storedTags;
        KURL::List m_urlList;
        QString m_buttonMbText;
        QString m_path;
        QString m_currentCover;
        QString m_mbTrack;
};


#endif /*AMAROK_TAGDIALOG_H*/

