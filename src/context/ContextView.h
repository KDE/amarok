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

#ifndef CONTEXT_VIEW_H
#define CONTEXT_VIEW_H

#include "Context.h"
#include "ContextObserver.h"
#include "ContextScene.h"
#include "EngineController.h"
#include "Svg.h"
#include "amarok_export.h"
#include "widgets/ContainmentArrow.h"

#include "plasma/containment.h"

#include <QMouseEvent>

#include <QGraphicsView>

class QPixmap;

namespace Context
{

static const int INTER_CONTAINMENT_MARGIN = 6;
static const int CONTAINMENT_COLUMNS = 2;
    
class ContextScene;
class ControlBox;

class AMAROK_EXPORT ContextView : public QGraphicsView, public EngineObserver, public ContextSubject
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

    Plasma::ZoomLevel zoomLevel() const;

    Plasma::Containment *containment();

public slots:
    void zoom( Plasma::Containment* containment, Plasma::ZoomDirection direction );
    void zoomIn( Plasma::Containment* containment );
    void zoomOut( Plasma::Containment* containment );

    Plasma::Applet* addApplet(const QString& name, const QStringList& args = QStringList());

    void setContainment( Plasma::Containment* containment );
    void setContainment( Plasma::Containment* fromCont, int toDirection );
    
    void nextContainment();
    void previousContainment();
    
    // void arrowChangeContainment( Plasma::Containment* from, ArrowDirection to );

    /**
    * Find a containment with @p rowSpan free rows to add the applet with @p pluginName.
    * If there is no availiable containment create a new one and add the applet there.
    * The focus is changed to the containment where the applet has been added.
    * @arg pluginName the applet pluginName to add.
    * @arg rowSpan the applet row span
    */
    void findContainmentForApplet( QString pluginName, int rowSpan );

protected:
    void engineStateChanged( Phonon::State, Phonon::State = Phonon::StoppedState );

    void resizeEvent(QResizeEvent *event);
    void wheelEvent(QWheelEvent *event);
//     void mousePressEvent( QMouseEvent* event );

private:
    static ContextView* s_self;

    /**
    * Add a new context containment to the view
    */
    void addContainment( const QVariantList &args = QVariantList() );

    /**
    * Connect all needed signals to a containment
    * @arg containment the containment to connect the signals to
    */
    void connectContainment( Plasma::Containment* containment );    

    /**
    * Disconnect all signals set in connectContainment
    * @arg containment the containment to disconnect the signals
    */
    void disconnectContainment( Plasma::Containment* containment );
    
    /**
    * Set the current containment or all containments geometry in the scene with the same geometry as the
    * Context View widget.
    * @arg updateAll if true updates all containments else only the current containment.
    */
    void updateContainmentsGeometry( bool updateAll = false );
    
    void centerOnZoom( qreal factor, Plasma::ZoomDirection direction );
    
    void loadConfig();

    void showHome();

    typedef QPointer< Context::Applet > AppletPointer;    

    // holds what is currently being shown
    ContextState m_curState;
    
    Plasma::ZoomLevel m_zoomLevel;

    bool m_startupFinished;

    QRectF m_startPos;
    QRectF m_destinationPos;

    Plasma::Containment *m_containment;
    int m_numContainments;

    //ControlBox* m_controlBox;

private slots:
    void animateZoomIn( qreal progress, int id );
    void animateZoomOut( qreal progress, int id );
    void zoomInFinished( int id );
    void zoomOutFinished( int id );
    void animateContainmentChange( qreal progress, int id );

    /**
    * Correct containments geometries after containment change animation has finished
    * @arg id the id of the custom animation
    */
    void animateContainmentChangeFinished( int id );
};

} // Context namespace

#endif
