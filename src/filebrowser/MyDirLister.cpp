#include "MyDirLister.h"
#include "PlaylistManager.h"

bool
MyDirLister::matchesFilter( const KFileItem &item ) const
{
    return
        ( item.isDir() && !item.isHidden() ) ||
//         EngineController::canDecode( item.url() ) || //FIXME: The way canDecode works in phonon this produces a hell of a lot of false positives.
        item.url().protocol() == "audiocd" ||
        PlaylistManager::isPlaylist( item.url() ) ||
        item.name().endsWith( ".mp3", Qt::CaseInsensitive ) || //for now this is less confusing for the user
        item.name().endsWith( ".aa", Qt::CaseInsensitive ) || //for adding to iPod
        item.name().endsWith( ".mp4", Qt::CaseInsensitive ) || //for adding to iPod
        item.name().endsWith( ".m4v", Qt::CaseInsensitive ) || //for adding to iPod
        item.name().endsWith( ".m4b", Qt::CaseInsensitive ) || //for adding to iPod
        item.name().endsWith( ".ogg", Qt::CaseInsensitive ) ||
        item.name().endsWith( ".oga", Qt::CaseInsensitive ) ||
        item.name().endsWith( ".flac", Qt::CaseInsensitive ) ||
        item.name().endsWith( ".wma", Qt::CaseInsensitive );

}

#include "MyDirLister.moc"
