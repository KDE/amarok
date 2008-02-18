/***************************************************************************
copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License or (at your option) version 3 or any later version
accepted by the membership of KDE e.V. (or its successor approved
by the membership of KDE e.V.), which shall act as a proxy
defined in Section 14 of version 3 of the license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
class SvgTinter;

namespace Amarok {
    class ContextStatusBar;
}

namespace The {
    AMAROK_EXPORT Playlist::Model*   playlistModel();       //defined in playlist/PlaylistModel.cpp
    Playlist::GraphicsView*          playlistView();
    AMAROK_EXPORT PlaylistManager*   playlistManager();
    QueueManagerNS::Model*           queueModel();          //defined in queuemanager/QueueModel.cpp
    ServiceInfoProxy *               serviceInfoProxy();
    AMAROK_EXPORT SvgTinter *        svgTinter();
    AMAROK_EXPORT Amarok::ContextStatusBar* statusBar();
}

#endif
