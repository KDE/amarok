/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
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

/*
  Significant parts of this code is inspired and/or copied from KDE Plasma sources,
  available at kdebase/workspace/libs/plasma
*/

#ifndef AMAROK_CONTEXT_VIEW_H
#define AMAROK_CONTEXT_VIEW_H

#include "Context.h"
#include "ContextObserver.h"
#include "ContextScene.h"
#include "EngineController.h"
#include "Svg.h"
#include "amarok_export.h"
#include "widgets/ContainmentArrow.h"
#include "widgets/appletexplorer/AppletExplorer.h"

#include <Plasma/Containment>

#include <QMouseEvent>
#include <QGraphicsView>
#include <QQueue>
#include <QAbstractAnimation>

class QPixmap;
class ContextUrlRunner;
class QParallelAnimationGroup;

namespace Context
{

class ContextScene;
class ControlBox;

class AMAROK_EXPORT ContextView : public QGraphicsView, public ContextSubject
{
    Q_OBJECT

public:
     ContextView( Plasma::Containment *containment, Plasma::Corona *corona, QWidget* parent = 0 );
    ~ContextView();

    /**
     * Singleton pattern accessor. May return 0 if the view was not yet constructed.
     */
    static ContextView *self() { return s_self; }

    /**
        Returns the context scene that this view is attached to.
    */
    ContextScene* contextScene();

    /**
        Clears the context scene of all items, but first saves the current state of the scene into the
        config file using as a key the string parameter.
    */
    void clear( const ContextState& name );

    void clearNoSave();

    /**
        Shows the home state. Loads applets from config file.
    */
    void showHome();

    /**
        Get the plugin names, in order, of the applets currently in the contextView.
    */
    QStringList currentApplets();

    /**
        Get the user visible applet names, in order, of the applets currently in the contextView.
    */
    QStringList currentAppletNames();

    /**
        Adds a collapse animation
        This object will take ownership of the animation.
    */
    void addCollapseAnimation( QAbstractAnimation *anim );

public Q_SLOTS:
    /**
     * Convenience methods to show and hide the applet explorer.
     */
    void hideAppletExplorer();
    void showAppletExplorer();

Q_SIGNALS:
    void appletExplorerHid();

protected:
    void resizeEvent(QResizeEvent *event);
    void wheelEvent(QWheelEvent *event);

private Q_SLOTS:
    void slotTrackChanged( Meta::TrackPtr track );
    void slotMetadataChanged( Meta::TrackPtr track );
    void slotPositionAppletExplorer();
    void slotStartCollapseAnimations();
    void slotCollapseAnimationsFinished();

private:
    static ContextView* s_self;

    void loadConfig();

    // holds what is currently being shown
    ContextState m_curState;

    ContextUrlRunner * m_urlRunner;

    AppletExplorer *m_appletExplorer;
    QParallelAnimationGroup *m_collapseAnimations;
    QAnimationGroup *m_queuedAnimations;
    QTimer *m_collapseGroupTimer;
};

} // Context namespace

#endif
