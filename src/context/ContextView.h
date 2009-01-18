/*****************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>           *
*                      : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
*                        Significant parts of this code is inspired          *
*                        and/or copied from KDE Plasma sources, available    *
*                        at kdebase/workspace/libs/plasma                    *
*                                                                            *
******************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef AMAROK_CONTEXT_VIEW_H
#define AMAROK_CONTEXT_VIEW_H

#include "Context.h"
#include "ContextObserver.h"
#include "ContextScene.h"
#include "EngineController.h"
#include "Svg.h"
#include "amarok_export.h"
#include "widgets/ContainmentArrow.h"

#include "plasma/containment.h"
#include "plasma/view.h"
#include <QMouseEvent>

#include <QGraphicsView>

class QPixmap;

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

    /**
        Clear the context scene of all items, discarding any data/widgets currently on the scene.
    */
    void clear();

public slots:

    Plasma::Applet* addApplet(const QString& name, const QStringList& args = QStringList());

protected:
    void engineStateChanged( Phonon::State, Phonon::State = Phonon::StoppedState );

    void resizeEvent(QResizeEvent *event);
    void wheelEvent(QWheelEvent *event);
//     void mousePressEvent( QMouseEvent* event );

private:
    static ContextView* s_self;

    /**
    * Set all containments geometry in the scene with the same geometry as the Context View widget 
    */
    void updateContainmentsGeometry();
      
    void loadConfig();

    void showHome();

    typedef QPointer< Context::Applet > AppletPointer;    

    // holds what is currently being shown
    ContextState m_curState;
};

} // Context namespace

#endif
