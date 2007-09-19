/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ColumnApplet.h"

#include "ContextScene.h"
#include "debug.h"

#include "plasma/widgets/layoutanimator.h"

#include <QGraphicsScene>
#include <QTimeLine>

namespace Context
{

ColumnApplet::ColumnApplet( QGraphicsItem * parent )
    : Plasma::Applet( parent )
    , m_defaultColumnSize( 450 )
{
    m_columns = new Plasma::FlowLayout( this );
    m_columns->setColumnWidth( m_defaultColumnSize );
    
}

// fetches size from scene and creates columns according.
// should only be called *ONCE* by the ContextView once it
// has been added to the scene
// void ColumnApplet::init() // SLOT
// {
    // TODO wait until this is completely implemented in plasma
// and for it to not crash....
//     foreach( Plasma::VBoxLayout* column, m_layout )
//     {
// 	
//         Plasma::LayoutAnimator* animator = new Plasma::LayoutAnimator;
// 	QTimeLine* timeLine = new QTimeLine;
// 	animator->setTimeLine(timeLine);
//         animator->setEffect( Plasma::LayoutAnimator::InsertedState , Plasma::LayoutAnimator::FadeInMoveEffect );
//         animator->setEffect( Plasma::LayoutAnimator::StandardState , Plasma::LayoutAnimator::MoveEffect );
//         animator->setEffect( Plasma::LayoutAnimator::RemovedState , Plasma::LayoutAnimator::FadeOutMoveEffect );
// 	column->setAnimator(animator);    
//     }
// }

void ColumnApplet::saveToConfig( KConfig& conf )
{
    DEBUG_BLOCK
        // TODO port
//     for( int i = 0; i < m_layout.size(); i++ )
//     {
//         for( int k = 0; k < m_layout[ i ]->count() ; k++ )
//         {
//             Applet* applet = dynamic_cast< Applet* >( m_layout[ i ]->itemAt( k ) );
//             if( applet != 0 )
//             {
//                 KConfigGroup cg( &conf, QString::number( applet->id() ) );
//                 debug() << "saving applet" << applet->name();
//                 cg.writeEntry( "plugin", applet->pluginName() );
//                 cg.writeEntry( "column", QString::number( i ) );
//                 cg.writeEntry( "position", QString::number( k ) );
//             }
//         }
//     }
//     conf.sync();
}

void ColumnApplet::loadConfig( KConfig& conf )
{
    DEBUG_BLOCK
        // TODO port
    /*
    m_layout.clear();
    init();
    foreach( const QString& group, conf.groupList() )
    {
        KConfigGroup cg( &conf, group );
        debug() << "loading applet:" << cg.readEntry( "plugin", QString() )
            << QStringList() << group.toUInt() << cg.readEntry( "column", QString() ) << cg.readEntry( "position", QString() );

        ContextScene* scene = qobject_cast< ContextScene* >( this->scene() );
        if( scene != 0 )
        {
            Applet* applet = scene->addApplet( cg.readEntry( "plugin", QString() ),
                              QVariantList(),
                              group.toUInt(),
                              QRectF() );
            int column = cg.readEntry( "column", 1000 );
            int pos = cg.readEntry( "position", 1000 );
            debug() << "restoring applet to column:" << column << "and position:" << pos;
            if( column >= m_layout.size() ) // was saved in a column that is not available now
                addApplet( applet );
            else
            {
                if( pos < m_layout[ column ]->count() ) // there is a position for it
                    m_layout[ column ]->insertItem( pos, applet );
                else // just append it, there is no position for it
                    m_layout[ column ]->addItem( applet );
            }
        }
    }
    resizeColumns();*/
}

QRectF ColumnApplet::boundingRect() const {
//     qreal width = 2*m_padding;
//     qreal height = 0;
//     foreach( Plasma::VBoxLayout* column, m_layout )
//     {
//         width += column->sizeHint().width();

//     debug() << "returning geometry:" << m_geometry;
    return m_columns->geometry();
}
// call this when the view changes size: e.g. layout needs to be recalculated
void ColumnApplet::update() // SLOT
{
    if( scene() ) m_columns->setGeometry( scene()->sceneRect() );
}

void ColumnApplet::appletRemoved( QObject* object ) // SLOT
{
    Q_UNUSED( object )
    // basically we want to reshuffle the columns since we know something is gone
//     resizeColumns();
}

// parts of this code come from Qt 4.3, src/gui/graphicsview/qgraphicsitem.cpp
void ColumnApplet::mousePressEvent( QGraphicsSceneMouseEvent* event )
{
    DEBUG_BLOCK
    if (event->button() == Qt::LeftButton && (flags() & ItemIsSelectable))
    {
        bool multiSelect = (event->modifiers() & Qt::ControlModifier) != 0;
        if (!multiSelect) {
            if (!isSelected()) {
                if (scene())
                    scene()->clearSelection();
                setSelected(true);
            }
        }
    } else if (!(flags() & ItemIsMovable)) {
        event->ignore();
    }
}

// parts of this code come from Qt 4.3, src/gui/graphicsview/qgraphicsitem.cpp
void ColumnApplet::mouseMoveEvent( QGraphicsSceneMouseEvent * event )
{
    debug() << "layout manager got a mouse event";
    if ( ( event->buttons() & Qt::LeftButton ) )
    {

        // Find the active view.
        QGraphicsView *view = 0;
        if (event->widget())
            view = qobject_cast<QGraphicsView *>(event->widget()->parentWidget());

        if ((flags() & ItemIsMovable) && (!parentItem() || !parentItem()->isSelected())) {
            QPointF diff;
            if (flags() & ItemIgnoresTransformations) {
                    // Root items that ignore transformations need to
                    // calculate their diff by mapping viewport coordinates to
                    // parent coordinates. Items whose ancestors ignore
                    // transformations can ignore this problem; their events
                    // are already mapped correctly.
                QTransform viewToParentTransform = (sceneTransform() * view->viewportTransform()).inverted();

                QTransform myTransform = transform().translate(pos().x(), pos().y());
                viewToParentTransform = myTransform * viewToParentTransform;

                diff = viewToParentTransform.map(QPointF(view->mapFromGlobal(event->screenPos())))
                    - viewToParentTransform.map(QPointF(view->mapFromGlobal(event->lastScreenPos())));
            } else
            {
                diff = mapToParent(event->pos()) - mapToParent(event->lastPos());
            }

            moveBy(diff.x(), diff.y());

            if (flags() & ItemIsSelectable)
                setSelected(true);
        }
    }
}



AppletPointer ColumnApplet::addApplet( AppletPointer applet )
{
    debug() << "m_columns:" << m_columns;
    m_columns->addItem( applet );

    connect( applet, SIGNAL( changed() ), this, SLOT( recalculate() ) );
    return applet;
}

void ColumnApplet::recalculate()
{
    DEBUG_BLOCK
    debug() << "got child item that wants a recalculation";
    m_columns->setGeometry( m_columns->geometry() );
}


} // Context namespace

#include "ColumnApplet.moc"
