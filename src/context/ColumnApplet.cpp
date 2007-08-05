/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
*                                                                         *
*                        Significant parts of this code is inspired       *
*                        and/or copied from KDE Plasma sources, available *
*                        at kdebase/workspace/plasma                      *
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

#include "ColumnApplet.h"

#include "debug.h"

#include <QGraphicsScene>

namespace Context
{

ColumnApplet::ColumnApplet( QGraphicsItem * parent )
    : QGraphicsItem( parent )
    , m_padding( 3 )
    , m_defaultColumnSize( 300 )
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
            m_layout << new Plasma::VBoxLayout( this );
        resizeColumns();
    } else
        warning() << "no scene to get data from!!";
}

QRectF ColumnApplet::boundingRect() const {
//     qreal width = 2*m_padding;
//     qreal height = 0;
//     foreach( Plasma::VBoxLayout* column, m_layout )
//     {
//         width += column->sizeHint().width();

    return m_geometry;
}
// call this when the view changes size: e.g. layout needs to be recalculated
void ColumnApplet::update() // SLOT
{
    if( scene() ) m_geometry = scene()->sceneRect();
    resizeColumns();
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
    
    return applet;
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
            m_layout << new Plasma::VBoxLayout( this );
    } if( numColumns < m_layout.size() ) // view was shrunk
    {
        debug() << "gotta shrink!";
        Plasma::VBoxLayout* column = m_layout[ m_layout.size() - 1 ];
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
//         m_layout[ i ]->setPos( pos );
    }
    debug() << "columns laid out, now balancing";
    balanceColumns();
    foreach( Plasma::VBoxLayout* column, m_layout )
        column->setGeometry( column->geometry() );
    debug() << "result is we have:" << m_layout.size() << "columns:";
    foreach( Plasma::VBoxLayout* column, m_layout )
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
    
    while( true )
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
        
        qreal maxAppletHeight = m_layout[ maxColumn ]->itemAt( m_layout[ maxColumn ]->count() - 1 )->geometry().size().height();
        
//         debug() << "found maxHeight:" << maxHeight << "and maxColumn:" << maxColumn << "and maxAppletHeight" << maxAppletHeight;
        bool found = false;
        for( int i = 0; i < numColumns; i++ )
        {
            if( i == maxColumn || m_layout[ i ]->count() < 2 ) continue; // don't bother
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
        if( !found ) break;
    }
}

} // Context namespace
