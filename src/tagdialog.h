// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#ifndef AMAROK_TAGDIALOG_H
#define AMAROK_TAGDIALOG_H

#include "config.h"

#ifdef HAVE_MUSICBRAINZ
#include "musicbrainzquery.h"
#endif

#ifndef HAVE_MUSICBRAINZ
// Dummy MusicbrainzQuery::TrackList class for queryDone argument.
namespace MusicBrainzQuery {
    class TrackList {};
}
#endif

#include <kurl.h>             //stack alloc
#include "tagdialogbase.h"    //baseclass

class MetaBundle;


class TagDialog : public TagDialogBase
{
    Q_OBJECT

    public:    
        TagDialog( const KURL& url, QWidget* parent = 0 );
        TagDialog( const MetaBundle& mb, QWidget* parent = 0 );
    
    private slots:
        void okPressed();
        void checkModified();
        
        void musicbrainzQuery();
        void queryDone( const MusicBrainzQuery::TrackList& tracklist );
    
    private:
        void init();
        
        MetaBundle m_metaBundle;
        QString m_buttonMbText;
};


#endif /*AMAROK_TAGDIALOG_H*/

