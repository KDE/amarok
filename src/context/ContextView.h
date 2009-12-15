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

#include <plasma/containment.h>
#include <plasma/view.h>
#include <QMouseEvent>

#include <QGraphicsView>

class QPixmap;
class ContextUrlRunner;

namespace Context
{

class ContextScene;
class ControlBox;

class AMAROK_EXPORT ContextView : public Plasma::View, public EngineObserver, public ContextSubject
{
    Q_OBJECT

public:
     ContextView( Plasma::Containment *containment, Plasma::Corona *corona, QWidget* parent = 0 );
    ~ContextView();

     /**
         * Singleton pattern accessor.
     */
    static ContextView* self() { return s_self; }

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


public slots:

    Plasma::Applet* addApplet(const QString& name, const QStringList& args = QStringList());
    void hideAppletExplorer();
    void showAppletExplorer();

signals:
    void appletExplorerHid();
        
protected:
    
    virtual void enginePlaybackEnded( qint64 finalPosition, qint64 trackLength, PlaybackEndedReason reason );
    void engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged ); // for stream scrobbling
    virtual void engineNewTrackPlaying();
    
    void resizeEvent(QResizeEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    static ContextView* s_self;

    /**
    * Set all containments geometry in the scene with the same geometry as the Context View widget 
    */
    void updateContainmentsGeometry();
      
    void loadConfig();

    typedef QPointer< Context::Applet > AppletPointer;    

    // holds what is currently being shown
    ContextState m_curState;

    //it seems we get a Phonon::PausedState before we start to play anything.
    //Because we generally don't want to update the context view when moving from
    //Paused to Playing state, this causes the CV to not get updated when starting Amarok
    //with a track being resumed (Resume playback enabled in the options). To avoid this,
     //we always kick the cv on the first play state we receive, irregardless if the
     //previous state was Paused.
    bool m_firstPlayingState;

    ContextUrlRunner * m_urlRunner;

    AppletExplorer* m_appletExplorer;
};

} // Context namespace

#endif
