/***************************************************************************
* copyright            : (C) 2007-2008 Leo Franchi <lfranchi@gmail.com>         *
* copyright            : (C) 2008 Mark Kretschmann <kretschmann@kde.org>   *
* copyright            : (C) 2008 William Viana Soares <vianasw@gmail.com> *
****************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "Amarok.h"
#include "ColumnContainment.h"
#include "Debug.h"
#include "SvgHandler.h"

#include <QPainter>

#include <limits.h>

namespace Context
{
ColumnContainment::ColumnContainment( QObject *parent, const QVariantList &args )
    : Context::Containment( parent, args )
    , m_actions( 0 )
    , m_minColumnWidth( 370 )
    , m_maxColumnWidth( 500 )
    , m_rowHeight( 150 )
    , m_preferredRowHeight( 150 )
    , m_paintTitle( 0 )
    , m_manageCurrentTrack( 0 )
    , m_appletsFromConfigCount( 0 )
    , m_toolBox( 0 )
    , m_view( 0 )
{
    setContainmentType( CustomContainment );

    m_grid = new QGraphicsGridLayout();
    setLayout( m_grid );
    m_grid->setSpacing( 3 );
    m_grid->setContentsMargins( 0, 0, 0, 0 );
    setContentsMargins( 20, 60, 0, 0 );

    setMaximumSize( QSizeF( INT_MAX, INT_MAX ) );

    for( int i = 0; i < MAX_ROWS; i++ )
        for( int j = 0; j < MAX_COLUMNS; j++ )
            m_gridFreePositions[i][j] = true;

    setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));

    loadInitialConfig();

    m_header = new Context::Svg( this );
    m_header->setImagePath( "widgets/amarok-containment-header" );
    m_header->setContainsMultipleImages( false );

    m_header->resize();

    m_title = new QGraphicsSimpleTextItem( this );

    m_title->hide();
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 5 );
    m_title->setFont( labelFont );

    connect( this, SIGNAL( appletAdded( Plasma::Applet*, const QPointF & ) ),
             this, SLOT( addApplet( Plasma::Applet*, const QPointF & ) ) );
    
    // 
    // foreach( QVariant arrow, args )
    // {
    //     if( arrow.canConvert< int >() )
    //     {
    //         debug() << "CREATING CONTAINMENT ARROW: " << arrow.value< int >();
    //         addContainmentArrow( arrow.value< int >() );
    //     }
    // }
    
    // for now, just do left/right. once we get nuno's mockups we'll see
    // what else to do.
    // DISABLING ARROWS FOR BETA, AND UNTIL WE HAVE AN ARTIST's OPINION
    // addContainmentArrow( LEFT );
    // addContainmentArrow( RIGHT );

    connect( this, SIGNAL( appletRemoved( Plasma::Applet* ) ), this, SLOT( appletRemoved( Plasma::Applet* ) ) );
}

ColumnContainment::~ColumnContainment()
{
    clearApplets();
    m_appletsPositions.clear();    
    Amarok::config( "ContextView" ).writeEntry( "ContextView size", size() );
}


void
ColumnContainment::loadInitialConfig()
{
    DEBUG_BLOCK

    QSize cvSize = Amarok::config( "ContextView" ).readEntry( "ContextView size", QSize() );
    debug() << cvSize;
    if( cvSize.isValid() )
    {
        if( cvSize.height() / m_preferredRowHeight < 4 )
        {
            if( cvSize.height() / 3 < m_preferredRowHeight / 0.7 )
                m_rowHeight = m_preferredRowHeight;
            else
                m_rowHeight = cvSize.height() / 3;
        }
        else
        {
            int numRows = cvSize.height() / m_preferredRowHeight;
            m_rowHeight = cvSize.height() / numRows;
        }
        m_currentColumns = cvSize.width() / m_minColumnWidth;
        m_currentRows = cvSize.height() / m_rowHeight;
        m_width = cvSize.width();
    }
    else
    {
        //FIXME: get these values based on the current application size
        m_currentColumns = 1;
        m_currentRows = 4;
        m_width = 500.0;
    }
    debug() << "current columns: " << m_currentColumns;
    debug() << "current rows: " << m_currentRows;
}


void
ColumnContainment::constraintsEvent( Plasma::Constraints constraints )
{
    DEBUG_BLOCK
    m_grid->setGeometry( contentsRect() );
    
    if( m_rowHeight < m_preferredRowHeight && rect().height() / m_preferredRowHeight >= 4 )
    {
        int numRows = rect().height() / m_preferredRowHeight;
        m_rowHeight = rect().height() / numRows;
    }
    
    m_currentRows = ( int )( rect().height() ) / m_rowHeight;

    const int columns = ( int )( rect().width() ) / m_minColumnWidth;
    debug() << "rect(): " << rect();
    debug() << "size(): " << size();
    debug() << "geometry():" << geometry();
    debug() << "boundingRect(): " << boundingRect();
    debug() << "rect.width(): " << rect().width();
    debug() << "m_minColumnWidth(): " << m_minColumnWidth;
    debug() << "columns: " << columns;


    if( columns != m_currentColumns )
    {
        const int rowCount = m_grid->rowCount();
        const bool hide = columns  < m_currentColumns;
        const int columnsToUpdate = qAbs( m_currentColumns - columns );
        const int col = hide ? m_currentColumns - columnsToUpdate : m_currentColumns;

        for( int j = col; j < col + columnsToUpdate; j++ )
        {
            for( int i = 0; i < rowCount; )
            {
                debug() << "Column: " << j << " Row: " << i;
                Plasma::Applet* applet = static_cast< Plasma::Applet* >( m_grid->itemAt( i, j ) );

                if( !applet ) //probably there are no applets left in the column
                {
                    debug() << "no applet";
                    break;
                }

                if( hide )
                    applet->hide();
                else
                    applet->show();

                QList<int> pos = m_appletsPositions[applet];
                const int rowSpan = pos[2];
                i += rowSpan;
            }
        }
    }

    m_currentColumns = columns;

    if( !m_pendingApplets.isEmpty() &&  m_currentColumns > 0 && m_currentRows > 0 )
    {
        while( !m_pendingApplets.isEmpty() )
        {
            Plasma::Containment::addApplet( m_pendingApplets.dequeue() );
        }
    }

    m_grid->updateGeometry();
    m_maxColumnWidth = rect().width();

    if( m_toolBox )
    {
        m_toolBox->setPos( geometry().width() / 2 - m_toolBox->size().width() / 2,
                            geometry().height() - m_toolBox->size().height() );
    }
    
    if( m_arrows[ LEFT ] ) 
        m_arrows[ LEFT ]->resize( geometry().size() );
    if( m_arrows[ RIGHT ] ) 
        m_arrows[ RIGHT ]->resize( geometry().size() );
    if( m_arrows[ DOWN ] ) 
        m_arrows[ DOWN ]->resize( geometry().size() );
    if( m_arrows[ UP ] ) 
        m_arrows[ UP ]->resize( geometry().size() );
    correctArrowPositions();
    
    //debug() << "ColumnContainment updating size to:" << geometry() << "sceneRect is:" << scene()->sceneRect() << "max size is:" << maximumSize();
}

QList<QAction*>
ColumnContainment::contextualActions()
{
    QAction *appletBrowserAction = new QAction(KIcon("list-add-amarok"), i18n("Add Applet..."), this);
    QList<QAction*> actions;
    actions.append( appletBrowserAction );
    return actions;
}

void
ColumnContainment::paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect& contentsRect)
{
    Q_UNUSED( option );
    painter->save();

    if( m_paintTitle )
    {
        int width = rect().width() * 0.95;
        int offsetX = ( rect().width() - width ) / 2;
        QRectF headerRect( rect().topLeft().x() + offsetX,
                           rect().topLeft().y(),
                           width,
                           55 );
        m_header->paint( painter, headerRect );
    }

    painter->restore();

    painter->save();
    //paint divider
    int dividerOffset = rect().width() / 20;
    painter->drawPixmap( dividerOffset, geometry().height() - 1, The::svgHandler()->renderSvg( "divider_bottom", rect().width() - 2 * dividerOffset,  1, "divider_bottom" ) );
    painter->drawPixmap( dividerOffset, geometry().height(), The::svgHandler()->renderSvg( "divider_top", rect().width() - 2 * dividerOffset, 1, "divider_top" ) );

    painter->restore();
}

void
ColumnContainment::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    DEBUG_BLOCK
    debug() << event->pos();
    if( event->button() == Qt::LeftButton )
    {
        debug() << "Focus requested by containment";
        emit focusRequested( this );
    }
    Context::Containment::mousePressEvent( event );
}

void
ColumnContainment::saveToConfig( KConfigGroup &conf )
{
    QStringList plugins;
    for( int i = 0; i < m_grid->count(); i++ )
    {
        Applet *applet = 0;
        QGraphicsLayoutItem *item = m_grid->itemAt( i );
        applet = dynamic_cast<Plasma::Applet*>( item );
        debug() << "trying to save an applet";
        if( applet != 0 && applet->pluginName() != "currenttrack" ) //don't save the current track applet
        {
            debug() << "saving applet" << applet->name();
            plugins << applet->pluginName();
        }
        conf.writeEntry( "plugins", plugins );
    }
    
}


void
ColumnContainment::loadConfig( const KConfigGroup &conf )
{
    DEBUG_BLOCK
    if( m_manageCurrentTrack )
    {
        debug() << "adding current track";
        Plasma::Containment::addApplet( "currenttrack" );
    }
    QStringList plugins = conf.readEntry( "plugins", QStringList() );
    m_appletsFromConfigCount = 1 + plugins.size();
    debug() << "plugins.size(): " << plugins.size();
    foreach( const QString& plugin, plugins )
    {
        debug() << "Adding applet: " << plugin;
        if( m_currentColumns == 0 || m_currentRows == 0 )
            m_pendingApplets.enqueue( plugin );
        else
            Plasma::Containment::addApplet( plugin );
    }
}


bool
ColumnContainment::hasPlaceForApplet( int rowSpan )
{
    bool positionFound = false;
    int j = 0;
    //traverse the grid matrix to find rowSpan consecutive positions in the same column

    while( !positionFound && j < m_currentColumns   )
    {
        int consec = 0;
        int i = 0;
        while( i < m_currentRows && consec < rowSpan)
        {

            if( m_gridFreePositions[i][j] )
                consec++;
            else
                consec = 0;
            i++;
        }
        if( consec == rowSpan )
        {
            positionFound = true;
        }
        j++;
    }
    return positionFound;
}

void
ColumnContainment::setTitle( QString title )
{
    m_title->setText( title );
}

void
ColumnContainment::showTitle()
{
    m_paintTitle = true;
    int offSetX = ( rect().width() - 100 ) / 2;
    int offSetY = rect().topLeft().y() + 10;
    m_title->setPos( offSetX , offSetY );
    m_title->show();
}

void
ColumnContainment::hideTitle()
{
    m_title->hide();
    m_paintTitle = false;
}

void
ColumnContainment::addCurrentTrack()
{
    m_manageCurrentTrack = true;
}

void
ColumnContainment::addToolBox()
{
    DEBUG_BLOCK
    if( m_toolBox )
        return;
    m_toolBox = new AmarokToolBox( this );
    if( view() )
    {
        DEBUG_LINE_INFO
        connect( m_toolBox, SIGNAL( changeContainment( Plasma::Containment * ) ),
                 view(), SLOT( setContainment( Plasma::Containment * ) ) );
        connect( m_toolBox, SIGNAL( correctToolBoxPos() ), this, SLOT( correctToolBoxPos() ) );
    }
    m_toolBox->setPos( geometry().width() / 2 - m_toolBox->size().width() / 2,
                       geometry().height() - m_toolBox->size().height() - 30 );
    m_toolBox->show();
    m_toolBox->addAction( action( "zoom in" ) );
    m_toolBox->addAction( action( "zoom out" ) );
}

void
ColumnContainment::correctToolBoxPos()
{
    DEBUG_BLOCK

    if( !view() )
        return;

    Plasma::ZoomLevel zoomLevel = view()->zoomLevel();

    if( m_toolBox->showingMenu() )
    {
        DEBUG_LINE_INFO
        if( zoomLevel == Plasma::GroupZoom )
        {
            m_toolBox->setPos( geometry().width() / 2 - m_toolBox->size().width(),
                                geometry().height() - m_toolBox->size().height() * 2 - 30 );
        }
        else
        {
            m_toolBox->setPos( geometry().width() / 2 - m_toolBox->size().width() / 2,
                                geometry().height() - m_toolBox->size().height() );
        }
    }
    else
    {
        if( zoomLevel == Plasma::GroupZoom )
        {
            m_toolBox->setPos( geometry().width() / 2 - m_toolBox->size().width(),
                            geometry().height() - m_toolBox->size().height() * 2 - 25 );
        }
        else if( zoomLevel == Plasma::DesktopZoom )
        {
            m_toolBox->setPos( geometry().width() / 2 - m_toolBox->size().width() / 2,
                            geometry().height() - m_toolBox->size().height() );
        }
    }
}

ContextView *
ColumnContainment::view()
{
    return m_view;
}


bool
ColumnContainment::insertInGrid( Plasma::Applet* applet )
{
    DEBUG_BLOCK
    int width;

    // this is done beucause of the CV small startup width. We use the width read from config file, may be not safe
    // in some cases.
    if( m_appletsFromConfigCount )
    {
        DEBUG_LINE_INFO
        width = m_currentColumns ? m_width / m_currentColumns : m_width;
        m_appletsFromConfigCount--;
    }
    else
        width = m_currentColumns ? m_maxColumnWidth / m_currentColumns : m_maxColumnWidth;
    int height = applet->effectiveSizeHint( Qt::PreferredSize, QSizeF( width, -1 ) ).height();
//     int rowSpan = qMin( qMax( ( int )( height )/ m_rowHeight , 1 ), 2 );

    //get the rowspan based on the aspect ratio and a constant.
    qreal aspectRatio = height / (qreal)width;
    int rowSpan = aspectRatio / 0.3;

    int colSpan = 1;

    if( rowSpan == 0 || aspectRatio * 100 > 30 && ( int )( aspectRatio * 100 ) % 30 > 15 )
        rowSpan += 1;

    rowSpan = qMin( rowSpan, m_currentRows );

    debug() << "current columns: " << m_currentColumns;
    debug() << "current rows: " << m_currentRows;
    debug() << "applet: " << applet->pluginName();
    debug() << "width: " << width;
    debug() << "height: " << height;
    debug() << "rowspan: " << rowSpan;

    int col = 0;
    int row = 0;
    bool positionFound = false;
    int j = 0;

    //traverse the grid matrix to find rowSpan consecutive positions in the same column

    while( !positionFound && j < m_currentColumns   )
    {
        int consec = 0;
        int i = 0;
        while( i < m_currentRows && consec < rowSpan)
        {
            if( m_gridFreePositions[i][j] )
                consec++;
            else
                consec = 0;
            i++;
        }
        if( consec == rowSpan )
        {
            positionFound = true;
            row = i - rowSpan; //get the first of the consecutive rows
            col = j;
        }
        j++;
    }

    if( positionFound )
    {

        QRectF rect = geometry();
        debug() << "applet height: " << height;
        debug() << "applet inserted at: " << row << col;
        debug() << "applet rowSpan :" << rowSpan;

        QList<int> pos;
        pos << row << col << rowSpan;
        m_appletsPositions[applet] = pos;
        m_appletsIndexes[applet] = m_grid->count();
//         m_grid->setColumnMaximumWidth( col, m_maxColumnWidth/m_currentColumns );
        m_grid->setColumnMinimumWidth( col, m_minColumnWidth );
        
        for( int i = 0; i < rowSpan; i++ )
        {
            m_gridFreePositions[row + i][col] = false;            
            m_grid->setRowMaximumHeight( row + i, m_rowHeight );
            m_grid->setRowPreferredHeight( row + i, height );
        }
        
        m_grid->setRowMinimumHeight( row, m_rowHeight * qMax( 1, rowSpan - 1 ) );
        
        
        if( m_grid->columnCount() > 0 )
            applet->resize( m_maxColumnWidth / m_grid->columnCount(), rowSpan * m_rowHeight );
        else
            applet->resize( m_maxColumnWidth, rowSpan * m_rowHeight );
        m_grid->addItem( applet, row, col, rowSpan, colSpan );

        updateConstraints( Plasma::SizeConstraint );
        m_grid->updateGeometry();
    }
    return positionFound;
}

Plasma::Applet*
ColumnContainment::addApplet( Plasma::Applet* applet, const QPointF & )
{
    DEBUG_BLOCK

    if( !insertInGrid( applet ) )
    {
        debug() << "Send applet to the next free containment";

        int height = applet->effectiveSizeHint( Qt::PreferredSize,
                                            QSizeF( m_maxColumnWidth, -1 ) ).height();
        int rowSpan = height / m_rowHeight;
        if( rowSpan == 0 || height % m_rowHeight >  m_rowHeight / 2.0  )
            rowSpan += 1;
        debug() << "emit appletRejected at containment";
        emit appletRejected( applet->pluginName(), rowSpan );
        applet->destroy();
        return 0;
    }
    return applet;
}

void
ColumnContainment::appletRemoved( Plasma::Applet* applet )
{
    DEBUG_BLOCK
    if( m_appletsPositions.contains( applet ) )
    {
        QList<int> pos = m_appletsPositions[applet];
        int row = pos[0];
        int col = pos[1];
        int rowSpan = pos[2];
        for( int i = 0; i < rowSpan; i++ )
            m_gridFreePositions[row + i][col] = true;
        m_appletsPositions.remove( applet );
        m_appletsIndexes.remove( applet );

        //keep the indexes updated
        int idx = m_appletsIndexes[applet];
        foreach( Plasma::Applet *a, m_appletsIndexes.keys() )
            if( m_appletsIndexes[a] > idx )
                m_appletsIndexes[a]--;

        if( m_grid->count() > 0 )
            rearrangeApplets( row + rowSpan, col );

    }

}

void
ColumnContainment::rearrangeApplets( int startRow, int startColumn )
{
    DEBUG_BLOCK
    int i = startRow;

    int lastColumn = m_grid->columnCount();
    debug() << "start row: " << startRow;
    debug() << "row count: " << m_grid->rowCount();

    for( int j = startColumn; j < lastColumn; j++ )
    {
        int lastRow = m_grid->rowCount();

        while( i < lastRow )
        {
            Plasma::Applet *applet = static_cast< Plasma::Applet* >( m_grid->itemAt( i, j ) );
            if( !applet )
            {
                debug() << "bad cast";
                break;
            }

            QList<int> pos = m_appletsPositions[applet];
            int rowSpan = pos[2];
            int idx = m_appletsIndexes[applet];

            debug() << "remove applet with index: " << idx;
            m_grid->removeAt( idx );

            for( int k = 0; k < rowSpan; k++ )
                m_gridFreePositions[i + k][j] = true;

            foreach( Plasma::Applet* a, m_appletsIndexes.keys() )
                if( m_appletsIndexes[a] > idx )
                    m_appletsIndexes[a]--;

            insertInGrid( applet );
            i += rowSpan;
        }
        i = 0;
    }

}

void 
ColumnContainment::addContainmentArrow( int direction )
{
    DEBUG_BLOCK

    if( m_arrows[ direction ] != 0 )
    {
        debug() << "GOT BAD DIRECTION";
        return;
    }    

    m_arrows[ direction ] = new ContainmentArrow( this, direction );
    m_arrows[ direction ]->setZValue( 100000 );
    debug() << "CONNECTING ARROW SIGNAL!";
    connect( m_arrows[ direction ], SIGNAL( changeContainment( int ) ), this, SLOT( slotArrowChangeContainment( int ) ) );

    correctArrowPositions();
} 

void
ColumnContainment::slotArrowChangeContainment( int to )
{
    debug() << "EMITTING CHANGECONTAINMENT";
    emit changeContainment( this, to );
}

void
ColumnContainment::correctArrowPositions()
{
    if( m_arrows[ UP ] ) {
        debug() << "setting UP arrow pos to 0,0";
        m_arrows[ UP ]->setPos( 0, 0 );  // easy case, just position in top left
    } if( m_arrows[ DOWN ] ) {
        debug() << "setting DOWN arrow pos to 0," << geometry().height() - m_arrows[ DOWN ]->size().height();
        m_arrows[ DOWN ]->setPos( 0, geometry().height() - m_arrows[ DOWN ]->size().height() ); // offset correctly to give it room to show
    } if( m_arrows[ LEFT ] ) {
        debug() << "setting LEFT arrow pos to " <<  m_arrows[ LEFT ]->size().width() << ",0";
        m_arrows[ LEFT ]->setPos( m_arrows[ LEFT ]->size().width() , 0 );
    } if( m_arrows[ RIGHT ] ) { 
        debug() << "setting RIGHT arrow pos to "<< geometry().width() - m_arrows[ RIGHT ]->size().width() << ",0";
        m_arrows[ RIGHT ]->setPos( geometry().width() - m_arrows[ RIGHT ]->size().width() , 0 );
    }
}

void
ColumnContainment::setView( ContextView *newView )
{
    m_view = newView;
    // all view-dependent signals, so we do it here
    debug() << "connecting to view";
    connect( this, SIGNAL( changeContainment( Plasma::Containment*, int ) ), m_view, SLOT( setContainment( Plasma::Containment*, int ) ) );
}

void ColumnContainment::setZoomLevel( Plasma::ZoomLevel level )
{
    if( level == Plasma::DesktopZoom ) // zoomed in
    {
        foreach( ContainmentArrow* arrow, m_arrows.values() )
        {
            if( arrow )
                arrow->enable();
        }
            
    } else if( level == Plasma::GroupZoom )
    {
        foreach( ContainmentArrow* arrow, m_arrows.values() )
        {
            if( arrow )
                arrow->disable();
        }
    }
}

} // Context namespace



#include "ColumnContainment.moc"
