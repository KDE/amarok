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


class TagDialog : public TagDialogBase
{
    Q_OBJECT

    public:    
        TagDialog( const MetaBundle& mb, QWidget* parent = 0 );
    
    private slots:
        void okPressed();

#ifdef HAVE_MUSICBRAINZ
        void musicbrainzQuery();
        void queryDone( const MusicBrainzQuery::TrackList& tracklist );
#endif
    
    private:
        MetaBundle m_metaBundle;
};


#endif /*AMAROK_TAGDIALOG_H*/

