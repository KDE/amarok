// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#ifndef AMAROK_TAGDIALOG_H
#define AMAROK_TAGDIALOG_H

#include "config.h"

#include "metabundle.h"       //stack alloc
#include "tagdialogbase.h"    //baseclass

#ifdef HAVE_MUSICBRAINZ
    #include "ktrm.h"
#else
    // Dummy class for queryDone argument.
    class KTRMResultList {};
#endif

#include <kurl.h>             //stack alloc
#include <qmap.h>

class PlaylistItem;


class TagDialog : public TagDialogBase
{
    Q_OBJECT

    public:
        TagDialog( const KURL& url, QWidget* parent = 0 );
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
        bool hasChanged();
        void storeTags();
        void saveTags();
        bool writeTag( MetaBundle mb );
        void syncItemText( MetaBundle mb );
        bool equalString( const QString&, const QString& );

        MetaBundle m_bundle;
        PlaylistItem* m_playlistItem;
        QMap<QString, MetaBundle> storedTags;
        QString m_buttonMbText;
        QString m_path;
        QString m_currentCover;
};


#endif /*AMAROK_TAGDIALOG_H*/

