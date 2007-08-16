/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/
#ifndef AMAROK_THE_INSTANCES
#define AMAROK_THE_INSTANCES

namespace PlaylistNS {
    class Model;
}

namespace PopupDropperNS {
    class PopupDropper;
}

namespace PortableDevicesNS {
    class SolidHandler;
}

// namespace Podcasts {
    class PodcastCollection;
// }

namespace QueueManagerNS {
   class Model;
}

class ServiceInfoProxy;

namespace The {
    PlaylistNS::Model*               playlistModel();       //defined in playlist/PlaylistModel.cpp
    PopupDropperNS::PopupDropper*    PopupDropper();
    PortableDevicesNS::SolidHandler* SolidHandler();
    /*Podcast::*/PodcastCollection*  podcastCollection();
    QueueManagerNS::Model*           queueModel();          //defined in queuemanager/QueueModel.cpp
    ServiceInfoProxy *               serviceInfoProxy(); 

}

#endif
