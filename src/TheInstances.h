/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/
#ifndef AMAROK_THE_INSTANCES
#define AMAROK_THE_INSTANCES

namespace Playlist {
    class Model;
    class GraphicsView;
}

namespace PopupDropperNS {
    class PopupDropper;
}

namespace PortableDevicesNS {
    class SolidHandler;
}


class PlaylistManager;

namespace QueueManagerNS {
   class Model;
}

class ServiceInfoProxy;

namespace The {
    Playlist::Model*                 playlistModel();       //defined in playlist/PlaylistModel.cpp
    Playlist::GraphicsView*          playlistView();
    PopupDropperNS::PopupDropper*    PopupDropper();
    PortableDevicesNS::SolidHandler* SolidHandler();
    PlaylistManager*                 playlistManager();
    QueueManagerNS::Model*           queueModel();          //defined in queuemanager/QueueModel.cpp
    ServiceInfoProxy *               serviceInfoProxy();
}

#endif
