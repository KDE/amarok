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

class PlaylistItem;


class TagDialog : public TagDialogBase
{
    Q_OBJECT

    public:
        TagDialog( const KURL& url, QWidget* parent = 0 );
        TagDialog( const MetaBundle& mb, PlaylistItem* item, QWidget* parent = 0 );

    private slots:
        void accept();
        void openPressed();
        void checkModified();

        void musicbrainzQuery();
        void queryDone( KTRMResultList results );

    private:
        void init();
        bool hasChanged();
        bool writeTag();
        void syncItemText();
        
        MetaBundle m_bundle;
        QListViewItem* m_playlistItem;
        QString m_buttonMbText;
        QString m_path;
};


#endif /*AMAROK_TAGDIALOG_H*/

