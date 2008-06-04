/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
*                        Significant parts of this code is inspired       *
*                        and/or copied from KDE Plasma sources, available *
*                        at kdebase/workspace/libs/plasma                 *
*
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef CONTEXT_VIEW_H
#define CONTEXT_VIEW_H

#include "amarok_export.h"
#include "Containment.h"
#include "Context.h"
#include "ContextScene.h"
#include "ContextObserver.h"
#include "EngineController.h"
#include "Svg.h"

#include "plasma/appletbrowser.h"
#include "plasma/view.h"

//TODO: move away, not need anymore
// #include <QGraphicsView>

class QPixmap;

namespace Context
{

class ContextScene;
class ControlBox;

class AMAROK_EXPORT ContextView : public Plasma::View, public EngineObserver, public ContextSubject
{
    Q_OBJECT

public:
     ContextView( Plasma::Containment *containment, QWidget* parent );
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

    /**
        Clear the context scene of all items, discarding any data/widgets currently on the scene.
    */
    void clear();

public slots:
    void zoomIn();
    void zoomOut();

    Plasma::Applet* addApplet(const QString& name, const QStringList& args = QStringList());
    void showAppletBrowser();

protected:
    void engineStateChanged( Phonon::State, Phonon::State = Phonon::StoppedState );

    void resizeEvent(QResizeEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    static ContextView* s_self;

    void connectContainment();

    void loadConfig();

    void showHome();
    void showCurrentTrack();

    typedef QPointer< Context::Applet > AppletPointer;    

    // holds what is currently being shown
    ContextState m_curState;
    Plasma::AppletBrowser *m_appletBrowser;

    //ControlBox* m_controlBox;

private slots:
    void appletBrowserDestroyed();

};

} // Context namespace

#endif
