// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#ifndef AMAROK_TAGDIALOG_H
#define AMAROK_TAGDIALOG_H

#include "config.h"
#include "ktrm.h"
#include "metabundle.h"       //stack alloc
#include "tagdialogbase.h"    //baseclass

#ifndef HAVE_MUSICBRAINZ    
    // Dummy class for queryDone argument.
    class KTRMResultList {};
#endif

#include <kurl.h>             //stack alloc

class PlaylistItem;


class TagDialog : public TagDialogBase
{
    Q_OBJECT

    public:
        TagDialog( const MetaBundle& mb, QWidget* parent );

        int exec() { return TagDialogBase::exec(); }
        void exec( PlaylistItem* );

    private slots:
        void accept();
        void openPressed();
        void checkModified();

        void musicbrainzQuery();
        void queryDone( KTRMResultList results );

    private:
        bool hasChanged();
        bool writeTag();

        MetaBundle m_bundle;
        QString m_buttonMbText;
        QString m_path;
};


#endif /*AMAROK_TAGDIALOG_H*/

