/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/
#ifndef AMAROK_THE_INSTANCES
#define AMAROK_THE_INSTANCES

#include "amarok_export.h"

namespace Playlist {
    class Model;
    class GraphicsView;
}

class PlaylistManager;

namespace QueueManagerNS {
   class Model;
}

class ServiceInfoProxy;

namespace The {
    AMAROK_EXPORT Playlist::Model*   playlistModel();       //defined in playlist/PlaylistModel.cpp
    Playlist::GraphicsView*          playlistView();
    PlaylistManager*                 playlistManager();
    QueueManagerNS::Model*           queueModel();          //defined in queuemanager/QueueModel.cpp
    ServiceInfoProxy *               serviceInfoProxy();
}

#endif
