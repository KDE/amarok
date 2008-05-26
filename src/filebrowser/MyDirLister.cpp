#include "MyDirLister.h"
#include "PlaylistManager.h"

bool
MyDirLister::matchesFilter( const KFileItem &item ) const
{
    if( item.isHidden() )
        return false;

    return
        item.isDir() ||
        EngineController::canDecode( item.url() ) || 
        item.url().protocol() == "audiocd" ||
        PlaylistManager::isPlaylist( item.url() ) ||
        item.name().endsWith( ".mp3", Qt::CaseInsensitive ) || //for now this is less confusing for the user
        item.name().endsWith( ".aa", Qt::CaseInsensitive ) ||  //for adding to iPod
        item.name().endsWith( ".mp4", Qt::CaseInsensitive ) || //for adding to iPod
        item.name().endsWith( ".m4v", Qt::CaseInsensitive ) || //for adding to iPod
        item.name().endsWith( ".m4b", Qt::CaseInsensitive );   //for adding to iPod
}

#include "MyDirLister.moc"
