#ifndef MYDIRLISTER_H
#define MYDIRLISTER_H

//TODO wait for lister to finish, if there are no files shown, but there are
//     media files in that directory show a longMessage (preferably one that disappears when given a signal)

#include "enginecontroller.h"
#include "playlistloader.h"

#include <kdirlister.h>
#include <kfileitem.h>

class MyDirLister : public KDirLister {
public:
    MyDirLister( bool delayedMimeTypes ) : KDirLister( delayedMimeTypes ) { }

protected:
    virtual bool matchesMimeFilter( const KFileItem *item ) const {
        return
            item->isDir() ||
            EngineController::canDecode( item->url() ) ||
            item->url().protocol() == "audiocd" ||
            PlaylistFile::isPlaylistFile( item->name() ) ||
            item->name().endsWith( ".mp3", false ) || //for now this is less confusing for the user
            item->name().endsWith( ".aa", false ) || //for adding to iPod
            item->name().endsWith( ".mp4", false ) || //for adding to iPod
            item->name().endsWith( ".m4v", false ) || //for adding to iPod
            item->name().endsWith( ".m4b", false ) || //for adding to iPod
            item->name().endsWith( ".ogg", false ) ||
            item->name().endsWith( ".flac", false ) ||
            item->name().endsWith( ".wma", false ) ||
            item->name().endsWith( ".asf", false );

    }
};

#endif
