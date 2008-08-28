/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef MAINTOOLBAR_H
#define MAINTOOLBAR_H

#include "EngineObserver.h" //baseclass

#include <KHBox>

class KToolBar;
class MainControlsWidget;
class PopupDropperAction;
class VolumeWidget;

/**
A KHBox based toolbar with a nice svg background and takes care of adding any additional controls needed by individual tracks

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class MainToolbar : public KHBox, public EngineObserver
{
public:
    MainToolbar( QWidget * parent );

    ~MainToolbar();

    virtual void engineStateChanged( Phonon::State state, Phonon::State oldState = Phonon::StoppedState );
    virtual void engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged );

    void reRender();

protected:
      virtual void paintEvent( QPaintEvent * );
      virtual void resizeEvent( QResizeEvent * event );
      virtual bool eventFilter( QObject*, QEvent* );
      virtual void paletteChange( const QPalette & oldPalette );
      void handleAddActions();
      //void centerAddActions();

private:
    QWidget            *m_insideBox;
    KToolBar           *m_addControlsToolbar;
    VolumeWidget       *m_volumeWidget;
    MainControlsWidget *m_mainControlsWidget;

    bool m_renderAddControls;
    int m_addActionsOffsetX;

    bool m_ignoreCache;

    QList<PopupDropperAction *> m_additionalActions;
};

#endif
