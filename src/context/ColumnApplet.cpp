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
    : QGraphicsItem( parent )
    , m_padding( 20 )
    , m_defaultColumnSize( 450 )
{
}

// fetches size from scene and creates columns according.
// should only be called *ONCE* by the ContextView once it
// has been added to the scene
void ColumnApplet::init() // SLOT
{
    QGraphicsScene* theScene = scene();
    if( theScene )
    {
        m_geometry = theScene->sceneRect();
        qreal width = m_geometry.width();
        int numColumns = (int)( width - ( 2 * m_padding ) ) / m_defaultColumnSize;
        if( numColumns == 0 ) numColumns = 1;
        debug() << "need to create:" << numColumns << "columns";
        for( int i = 0; i < numColumns; i++ )
            m_layout << new Context::VBoxLayout( this );
        resizeColumns();
    } else
        warning() << "no scene to get data from!!";
    
    // TODO wait until this is completely implemented in plasma
    foreach( VBoxLayout* column, m_layout )
    {
        Plasma::LayoutAnimator* animator = new Plasma::LayoutAnimator();
        QTimeLine* timeLine = new QTimeLine();
        animator->setTimeLine( timeLine );
        animator->setEffect( Plasma::LayoutAnimator::InsertedState, Plasma::LayoutAnimator::FadeInMoveEffect );
        animator->setEffect( Plasma::LayoutAnimator::NormalState, Plasma::LayoutAnimator::MoveEffect );
        animator->setEffect( Plasma::LayoutAnimator::RemovedState ,
                             Plasma::LayoutAnimator::FadeOutMoveEffect );
        column->setAnimator( animator );
    } 
}

void ColumnApplet::saveToConfig( KConfig& conf )
{
    DEBUG_BLOCK
    for( int i = 0; i < m_layout.size(); i++ )
    {
        for( int k = 0; k < m_layout[ i ]->count() ; k++ )
        {
            Applet* applet = dynamic_cast< Applet* >( m_layout[ i ]->itemAt( k ) );
            if( applet != 0 )
            {
                KConfigGroup cg( &conf, QString::number( applet->id() ) );
                debug() << "saving applet" << applet->name();
                cg.writeEntry( "plugin", applet->pluginName() );
                cg.writeEntry( "column", QString::number( i ) );
                cg.writeEntry( "position", QString::number( k ) );
            }
        }
    }
    conf.sync();
}

void ColumnApplet::loadConfig( KConfig& conf )
{
    DEBUG_BLOCK
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
                              QStringList(),
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
    resizeColumns();
}
                    
QRectF ColumnApplet::boundingRect() const {
//     qreal width = 2*m_padding;
//     qreal height = 0;
//     foreach( Plasma::VBoxLayout* column, m_layout )
//     {
//         width += column->sizeHint().width();

//     debug() << "returning geometry:" << m_geometry;
    return m_geometry;
}
// call this when the view changes size: e.g. layout needs to be recalculated
void ColumnApplet::update() // SLOT
{
    if( scene() ) m_geometry = scene()->sceneRect();
    resizeColumns();
}

void ColumnApplet::appletRemoved( QObject* object ) // SLOT
{ 
    Q_UNUSED( object )
    // basically we want to reshuffle the columns since we know something is gone
    resizeColumns();
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
    
    int smallestColumn = 0, min = (int)m_layout[ 0 ]->sizeHint().height();
    for( int i = 1; i < m_layout.size(); i++ ) // find shortest column to put
    {                                           // the applet in
        debug() << "comparing this column, size:" << m_layout[ i ]->sizeHint().height();
        if( m_layout[ i ]->sizeHint().height() < min )
            smallestColumn = i;
    }
    debug() << "smallest column is" << smallestColumn << "(" << min << ")" << "of" << m_layout.size();
    
    debug() << "found" << m_layout.size() << " column, adding applet to column:" << smallestColumn;
    m_layout[ smallestColumn ]->addItem( applet );
        
    connect( applet, SIGNAL( changed() ), this, SLOT( recalculate() ) );
    
    resizeColumns();
    
    return applet;
}

void ColumnApplet::recalculate()
{
    DEBUG_BLOCK
    debug() << "got child item that wants a recalculation";
    foreach( VBoxLayout* column, m_layout )
        column->setGeometry( column->geometry() );
}

void ColumnApplet::resizeColumns()
{
    DEBUG_BLOCK
    if( scene() == 0 ) return;
    qreal width = scene()->sceneRect().width() - 2 * m_padding;
    int numColumns = (int)( width - 2 * m_padding ) / m_defaultColumnSize;
    if( numColumns < 1 ) numColumns = 1;
    if( numColumns > m_layout.size() ) // need to make more columns
    {
        for( int i = m_layout.size(); i < numColumns; i++ )
            m_layout << new Context::VBoxLayout( this );
    } if( numColumns < m_layout.size() ) // view was shrunk
    {
        debug() << "gotta shrink!";
        Context::VBoxLayout* column = m_layout[ m_layout.size() - 1 ];
        m_layout.removeAt( m_layout.size() - 1 );
        for( int i = 0; i < column->count() ; i++ )
        {
            debug() << "trying to put away an item";
            LayoutItem* applet = column->takeAt( i );
            int smallestColumn = 0, min = (int)m_layout[ 0 ]->sizeHint().height();
            for( int i = 1; i < m_layout.size(); i++ ) // find shortest column to put
            {                                           // the applet in
                if( m_layout[ i ]->sizeHint().height() < min )
                    smallestColumn = i;
            }
            debug() << "found column for item:" << smallestColumn;
            m_layout[ smallestColumn ]->addItem( applet );
        }
    }
    
    qreal columnWidth = width / numColumns;
    columnWidth -= ( numColumns - 1 ) * m_padding; // make room between columns
    
    for( int i = 0; i < numColumns; i++ ) // lay out columns
    {
        debug() << "setting columns to width:" << columnWidth;
        QPointF pos( ( ( i + 1 ) * m_padding ) + ( i * columnWidth ), m_padding );
        QSizeF size( columnWidth, qMax( m_layout[ i ]->sizeHint().height(),
                                        scene()->sceneRect().height() ) );
        m_layout[ i ]->setGeometry( QRectF( pos, size ) );
    }
//     debug() << "columns laid out, now balancing";
    balanceColumns();
    foreach( Context::VBoxLayout* column, m_layout )
        column->setGeometry( column->geometry() );
    debug() << "result is we have:" << m_layout.size() << "columns:";
    foreach( Context::VBoxLayout* column, m_layout )
        debug() << "column rect:" << column->geometry() <<  "# of children:" << column->count();
}

// even out columns. this checks if any one column can be made shorter by
// moving the last applet to another column
void ColumnApplet::balanceColumns()
{
    DEBUG_BLOCK
    int numColumns = m_layout.size();
    if( numColumns == 1 ) // no balancing to do :)
        return;
    
    bool found = true;
    while( found )
    {
        qreal maxHeight  = -1; int maxColumn = -1;
        for( int i = 0; i < numColumns; i++ )
        {
            if( m_layout[ i ]->sizeHint().height() > maxHeight )
            {
                maxHeight = m_layout[ i ]->sizeHint().height();
                maxColumn = i;
            }
        }
        
        if( maxHeight == 0 ) // no applets
            return;
        if( m_layout[ maxColumn ]->count() == 1 ) // if the largest column only has
            return; // one applet, we can't do anything more
        
        qreal maxAppletHeight = m_layout[ maxColumn ]->itemAt( m_layout[ maxColumn ]->count() - 1 )->sizeHint().height() + 10;
        // HACK!
        // adding 10 is needed to cover the borders/padding... i can't get the exact
        // value from Plasma::VBoxLayout because it's not exposed. arg!
        
        
        debug() << "found maxHeight:" << maxHeight << "and maxColumn:" << maxColumn << "and maxAppletHeight" << maxAppletHeight;
        found = false;
        for( int i = 0; i < numColumns; i++ )
        {
            if( i == maxColumn ) continue; // don't bother
            debug() << "checking for column" << i << "of" << numColumns - 1;
            debug() << "doing math:" << m_layout[ i ]->sizeHint().height() << "+" << maxAppletHeight;
            qreal newColHeight = m_layout[ i ]->sizeHint().height() + maxAppletHeight;
            debug() << "checking if newColHeight:" << newColHeight << "is less than:" << maxHeight;
            if( newColHeight < maxHeight ) // found a new place for this applet
            {
                debug() << "found new place for an applet: column" << i;
                m_layout[ i ]->addItem( m_layout[ maxColumn ]->takeAt( m_layout[ maxColumn ]->count() - 1 ) );
                found = true;
                break;
            }
        }
    }
}

} // Context namespace

#include "ColumnApplet.moc"
