// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright:  See COPYING file that comes with this distribution
//
// Description: a popupmenu to control various features of amaroK
//              also provides amaroK's helpMenu

#ifndef AMAROKMENU_H
#define AMAROKMENU_H

#include "engineobserver.h"
#include <kaction.h>
#include <kpopupmenu.h>

class KActionCollection;
class KHelpMenu;


namespace amaroK {

class Menu : public QPopupMenu
{
Q_OBJECT

public:
    static const int ID_REPEAT_TRACK    = 100;
    static const int ID_REPEAT_PLAYLIST = 101;
    static const int ID_RANDOM_MODE     = 102;
    static const int ID_CONF_DECODER    = 103;

    Menu( QWidget *parent );

    static KPopupMenu *helpMenu( QWidget *parent = 0 );

private slots:
    void slotAboutToShow();
    void slotActivated( int index );

private:
    static KHelpMenu *HelpMenu;
};

//TODO try removing the QObject macro
class MenuAction : public KAction
{
Q_OBJECT

public:
    MenuAction( KActionCollection* );

    virtual int plug( QWidget*, int index = -1 );
/*
protected:
    virtual void virtual_hook( int id, void* data ) { KAction::virtual_hook( id, data ); }
*/
};

class PlayPauseAction : public KAction, public EngineObserver
{
Q_OBJECT

public:
    PlayPauseAction( KActionCollection* );

    virtual void engineStateChanged( EngineBase::EngineState );
};

}

#endif
