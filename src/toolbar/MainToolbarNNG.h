/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MainToolbarNNG_H
#define MainToolbarNNG_H

#include "EngineObserver.h" //baseclass
#include "SmartPointerList.h"

#include <KHBox>

#include <QToolBar>

class KToolBar;
class MainControlsWidget;
class QAction;
class VolumePopupButton;

/**
    A KHBox based toolbar with a nice svg background and takes care of 
    adding any additional controls needed by individual tracks
*/
class MainToolbarNNG : public QToolBar, public EngineObserver
{

public:
    MainToolbarNNG( QWidget * parent );

    ~MainToolbarNNG();

    virtual void engineStateChanged( Phonon::State state, Phonon::State oldState = Phonon::StoppedState );
    virtual void engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged );

    void reRender();

protected:
      virtual void resizeEvent( QResizeEvent * event );
      void handleAddActions();
      void centerAddActions();

private:
    QWidget            *m_insideBox;
    KToolBar           *m_addControlsToolbar;
    VolumePopupButton  *m_volumePopupButton;
    MainControlsWidget *m_mainControlsWidget;

    int m_addActionsOffsetX;
    bool m_ignoreCache;

    SmartPointerList<QAction> m_additionalActions;
};

#endif
