// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#ifndef AMAROK_TAGDIALOG_H
#define AMAROK_TAGDIALOG_H

#include "config.h"

#ifdef HAVE_MUSICBRAINZ
#include "musicbrainzquery.h"
#endif

#include "tagdialogbase.h"    //baseclass

class MetaBundle;

#ifndef HAVE_MUSICBRAINZ
// Dummy MusicbrainzQuery::TrackList class for queryDone argument.
namespace MusicBrainzQuery
{
    class TrackList {};
}
#endif

class TagDialog : public TagDialogBase
{
    Q_OBJECT

    public:    
        TagDialog( const MetaBundle& mb, QWidget* parent = 0 );
    
    private slots:
        void okPressed();
        void checkModified();
        
        void musicbrainzQuery();
        void queryDone( const MusicBrainzQuery::TrackList& tracklist );
    
    private:
        MetaBundle m_metaBundle;
        QString m_buttonMbText;
};


#endif /*AMAROK_TAGDIALOG_H*/

