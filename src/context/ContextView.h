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
#include "ColumnApplet.h"
#include "Context.h"
#include "ContextScene.h"
#include "ContextObserver.h"
#include "enginecontroller.h"
#include "Svg.h"

#include <QGraphicsView>

class QPixmap;

namespace Context
{

class ContextScene;
class ControlBox;

class AMAROK_EXPORT ContextView : public QGraphicsView, public EngineObserver, public ContextSubject
{
    Q_OBJECT
    
public:
     ContextView( QWidget* parent );
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
    
    Applet* addApplet(const QString& name, const QStringList& args = QStringList());
    
protected:        
    void engineNewMetaData( const MetaBundle&, bool );
    void engineStateChanged( Engine::State, Engine::State = Engine::Empty );
    
    void drawBackground(QPainter *painter, const QRectF &);
    void resizeEvent(QResizeEvent *event);
    void wheelEvent(QWheelEvent *event);
    
    void contextMenuEvent(QContextMenuEvent *event);
    
private:
    static ContextView* s_self;

    void loadConfig();
    
    void showHome();
    void showCurrentTrack();
    
    typedef QPointer< Context::Applet > AppletPointer;
    // internal representation of the columns visible
    ColumnApplet* m_columns;
    
    // holds what is currently being shown
    ContextState m_curState;
    
    ControlBox* m_controlBox;
    Svg *m_background;
    Svg *m_logo;
    
};

} // Context namespace

#endif
