// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright:  See COPYING file that comes with this distribution
//
// Description: a popupmenu to control various features of amaroK
//              also provides amaroK's helpMenu

#ifndef AMAROKMENU_H
#define AMAROKMENU_H

#include <kpopupmenu.h>

class KHelpMenu;

namespace amaroK {

class Menu : public QPopupMenu {
public:

    static const int ID_REPEAT_TRACK    = 100;
    static const int ID_REPEAT_PLAYLIST = 101;
    static const int ID_RANDOM_MODE     = 102;
    static const int ID_CONF_DECODER    = 103;

    Menu( QWidget *parent );
    int exec( const QPoint&, int indexAtPoint = -1 );

    static KPopupMenu *helpMenu( QWidget *parent = 0 );

private:
    static KHelpMenu *HelpMenu;
};

}

#endif
