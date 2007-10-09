#ifndef MYDIRLISTER_H
#define MYDIRLISTER_H

//TODO wait for lister to finish, if there are no files shown, but there are
//     media files in that directory show a longMessage (preferably one that disappears when given a signal)

#include "enginecontroller.h"
#include "PlaylistHandler.h"

#include <kdirlister.h>
#include <kfileitem.h>

class MyDirLister : public KDirLister {
public:
    MyDirLister( bool delayedMimeTypes ) : KDirLister() { setDelayedMimeTypes( delayedMimeTypes ); setAutoUpdate( true ); }

protected:
    virtual bool matchesFilter( const KFileItem &item ) const {
        PlaylistHandler ph;
        return
            ( item.isDir() && !item.isHidden() ) ||
            EngineController::canDecode( item.url() ) ||
            item.url().protocol() == "audiocd" ||
            ph.isPlaylist( item.url() ) ||
            item.name().endsWith( ".mp3", Qt::CaseInsensitive ) || //for now this is less confusing for the user
            item.name().endsWith( ".aa", Qt::CaseInsensitive ) || //for adding to iPod
            item.name().endsWith( ".mp4", Qt::CaseInsensitive ) || //for adding to iPod
            item.name().endsWith( ".m4v", Qt::CaseInsensitive ) || //for adding to iPod
            item.name().endsWith( ".m4b", Qt::CaseInsensitive ) || //for adding to iPod
            item.name().endsWith( ".ogg", Qt::CaseInsensitive ) ||
            item.name().endsWith( ".flac", Qt::CaseInsensitive ) ||
            item.name().endsWith( ".wma", Qt::CaseInsensitive );
    }
};

#endif
