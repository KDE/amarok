/****************************************************************************************
 * Copyright (c) 2007-2008 Leo Franchi <lfranchi@gmail.com>                             *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "ColumnContainment.h"

#include "Amarok.h"
#include "Debug.h"
#include "MainWindow.h"
#include "SvgHandler.h"

#include <QDesktopWidget>
#include <QFontMetrics>
#include <QPainter>

#include <limits.h>

namespace Context
{

static const int BORDER_PADDING = 30;
static const int OFFSET_Y = 40;

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
    , m_zoomInIcon( 0 )
    , m_zoomOutIcon( 0 )
    , m_addAppletsIcon( 0 )
    , m_removeAppletsIcon( 0 )
    , m_view( 0 )
    , m_selectionLayer( 0 )
{
    setContainmentType( CustomContainment );
    setDrawWallpaper( false );
    
    m_grid = new QGraphicsGridLayout();
    setLayout( m_grid );
    m_grid->setSpacing( 3 );
    m_grid->setContentsMargins( 0, 0, 0, 0 );
    setContentsMargins( 20, 60, 0, 0 );

    setMaximumSize( QSizeF( INT_MAX, INT_MAX ) );

    setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));

    loadInitialConfig();

    for( int i = 0; i < m_maxRows; i++ )
        for( int j = 0; j < m_maxColumns; j++ )
            m_gridFreePositions[i][j] = true;

    m_header = new Context::Svg( this );
    m_header->setImagePath( "widgets/amarok-containment-header" );
    m_header->setContainsMultipleImages( false );

    m_header->resize();

    m_title = new QGraphicsSimpleTextItem( this );

    m_title->hide();
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 5 );
    m_title->setFont( labelFont );

    m_footer = new QGraphicsSimpleTextItem( this );
    m_footer->setZValue( zValue() + 1000 );
    m_footer->setBrush( palette().brush( QPalette::Disabled, QPalette::Text ) );
    QFont footerFont = m_footer->font();
    footerFont.setPointSize( footerFont.pointSize() + 2 );
    footerFont.setWeight( 99 ); // Weight is on a scale from 0 (superlight) to 99 (superdark)
    m_footer->setFont( footerFont );

    connect( this, SIGNAL( appletAdded( Plasma::Applet*, const QPointF & ) ),
             this, SLOT( addApplet( Plasma::Applet*, const QPointF & ) ) );

    QAction *appletBrowserAction = new QAction( KIcon( "list-add-amarok" ), i18n( "Add Applet..." ), this );
    connect( appletBrowserAction, SIGNAL( triggered( bool ) ), this, SLOT( showAddAppletsMenu() ) );
    // set up default context menu actions
    m_actions = new QList<QAction*>();
    m_actions->append( appletBrowserAction );

    setupControlButtons();
    setupRemoveButton();
    
    connect( this, SIGNAL( appletRemoved( Plasma::Applet* ) ), this, SLOT( appletRemoved( Plasma::Applet* ) ) );

    m_selectionLayer = new ContainmentSelectionLayer( this );
    m_selectionLayer->hide();
    m_selectionLayer->setZValue( zValue() - 1000 ); //keep it down all elements

    connect( m_selectionLayer, SIGNAL( zoomRequested( Plasma::Containment *, Plasma::ZoomDirection ) ),
             this, SIGNAL( zoomRequested( Plasma::Containment *, Plasma::ZoomDirection ) ) );
}

ColumnContainment::~ColumnContainment()
{
    clearApplets();
    m_appletsPositions.clear();    
    Amarok::config( "ContextView" ).writeEntry( "ContextView size", size() );
    
    for( int i = 0; i < m_maxRows; i++ )
        delete [] m_gridFreePositions[i];
    delete [] m_gridFreePositions;
}

void
ColumnContainment::loadInitialConfig()
{
    DEBUG_BLOCK

    QSize cvSize = Amarok::config( "ContextView" ).readEntry( "ContextView size", QSize() );
    debug() << cvSize;

    // Get the CV size based on an extimation using the mainwindow size
    if( !cvSize.isValid() )
        cvSize = QSize( The::mainWindow()->size().width() * 0.4, The::mainWindow()->size().height() * 0.8 );
    
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
        m_currentColumns = qMax( 1, cvSize.width() / m_minColumnWidth );
        m_currentRows = qMax( 1, cvSize.height() / m_rowHeight );
        m_width = cvSize.width();
    }
    
    QRect screenRect = QApplication::desktop()->screenGeometry( screen() );
    QRect availableScreen = QApplication::desktop()->availableGeometry( screen() );

    // these are max boundaries
    m_maxRows = availableScreen.height() / m_rowHeight;
    m_maxColumns = availableScreen.width() / m_minColumnWidth;
    
    m_gridFreePositions = new PositionsRow[m_maxRows];
    
    for( int i = 0; i < m_maxRows; i++ )
        m_gridFreePositions[i] = new bool[m_maxColumns];
    
    debug() << "current columns: " << m_currentColumns;
    debug() << "current rows: " << m_currentRows;
    debug() << "max columns: " << m_maxColumns;
    debug() << "max rows: " << m_maxRows;
}

void
ColumnContainment::setupRemoveButton()
{
    QAction* listRemove = new QAction( i18n( "Remove Widgets..." ), this );
    listRemove->setIcon( KIcon( "list-remove" ) );
    listRemove->setVisible( true );
    listRemove->setEnabled( m_grid->count() > 0 );
    
    m_removeAppletsIcon = addAction( listRemove );

    connect( listRemove, SIGNAL( triggered() ), this, SLOT( showRemoveAppletsMenu() ) );
}

void
ColumnContainment::setupControlButtons()
{

    m_addAppletsMenu = new AmarokToolBoxMenu( this, false );
//     m_addAppletsMenu->setZValue( zValue() + 10000 ); // show over applets

    m_removeAppletsMenu = new AmarokToolBoxMenu( this, true );
//     m_removeAppletsMenu->setZValue( zValue() + 10000 ); // show over applets

    QAction* zoomInAction = new QAction( i18n( "Zoom In" ), this );
    zoomInAction->setIcon( KIcon( "zoom-in" ) );
    zoomInAction->setVisible( true );
    zoomInAction->setEnabled( true );

    QAction* zoomOutAction = new QAction( i18n( "Zoom Out" ), this );
    zoomOutAction->setIcon( KIcon( "zoom-out" ) );
    zoomOutAction->setVisible( true );
    zoomOutAction->setEnabled( true );

    QAction* listAdd = new QAction( i18n( "Add Widgets..." ), this );
    listAdd->setIcon( KIcon( "list-add" ) );
    listAdd->setVisible( true );
    listAdd->setEnabled( true );

    QAction *switchRight = new QAction( i18n( "Next Group" ), this );
    switchRight->setIcon( KIcon( "arrow-right" ) );
    switchRight->setVisible( true );
    switchRight->setEnabled( true );

    QAction *switchLeft = new QAction( i18n( "Previous Group" ), this );
    switchLeft->setIcon( KIcon( "arrow-left" ) );
    switchLeft->setVisible( true );
    switchLeft->setEnabled( true );
    
    m_zoomInIcon = addAction( zoomInAction  );
    m_zoomOutIcon = addAction( zoomOutAction );
    m_addAppletsIcon = addAction( listAdd );
    m_switchLeftIcon = addAction( switchLeft );
    m_switchRightIcon = addAction( switchRight );

    connect( listAdd, SIGNAL( triggered() ), this, SLOT( showAddAppletsMenu() ) );

    connect( m_zoomInIcon, SIGNAL( clicked() ), this, SLOT( zoomInRequested() ) );
    connect( m_zoomOutIcon, SIGNAL( clicked() ), this, SLOT( zoomOutRequested() ) );
}

void
ColumnContainment::constraintsEvent( Plasma::Constraints constraints )
{
    //setContainment must be done when the containment is completelly initiated
    //otherwise the menus that receive this pointer would have problems.
    if( constraints & Plasma::StartupCompletedConstraint )
    {
        m_addAppletsMenu->setContainment( this );
        m_removeAppletsMenu->setContainment( this );
        m_grid->setGeometry( contentsRect() );
    }

    if( m_rowHeight < m_preferredRowHeight && rect().height() / m_preferredRowHeight >= 4 )
    {
        int numRows = rect().height() / m_preferredRowHeight;
        m_rowHeight = rect().height() / numRows;
    }

    m_currentRows = ( int )( rect().height() ) / m_rowHeight;

    int columns = qMax( 1, ( int )( rect().width() ) / m_minColumnWidth );

    if( columns != m_currentColumns )
    {
        const int rowCount = m_grid->rowCount();
        const bool hide = columns  < m_currentColumns;
        const int columnsToUpdate = qAbs( m_currentColumns - columns );
        const int col = hide ? m_currentColumns - columnsToUpdate : m_currentColumns;
        const int lastColumn = col + columnsToUpdate;
        
        for( int j = col; j < lastColumn; j++ )
        {
            for( int i = 0; i < rowCount; )
            {
                Plasma::Applet* applet = static_cast< Plasma::Applet* >( m_grid->itemAt( i, j ) );

                if( !applet ) //probably there are no applets left in the column
                    break;

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
    
    correctControlButtonPositions();
}

QList<QAction*>
ColumnContainment::contextualActions()
{
    return *m_actions;
}

void
ColumnContainment::paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect& contentsRect)
{
    Q_UNUSED( option ); Q_UNUSED( contentsRect );
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
        if( m_zoomLevel == Plasma::GroupZoom )
            emit zoomRequested( this, Plasma::ZoomIn ); //only zoom in when zoomed out
    }

    m_addAppletsMenu->hide();
    m_removeAppletsMenu->hide();

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

        if( applet != 0 && applet->pluginName() != "currenttrack" )
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
        m_appletsFromConfigCount++;
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
    setupRemoveButton();
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
ColumnContainment::setTitle( const QString& text )
{
    m_title->setText( text );
}

void
ColumnContainment::setFooter( const QString& text )
{
    m_footer->setText( text );
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
ColumnContainment::showAddAppletsMenu()
{
    if( m_removeAppletsMenu->showing() )
        m_removeAppletsMenu->hide();
    if( m_addAppletsMenu->showing() )
    {   // hide again on double-click
        m_addAppletsMenu->hide();
        return;
    }
    const qreal xpos = BORDER_PADDING;
    const qreal ypos = contentsRect().height() - m_addAppletsMenu->boundingRect().height() + OFFSET_Y;

    m_addAppletsMenu->setPos( xpos, ypos );
    m_addAppletsMenu->show();
}

void
ColumnContainment::showRemoveAppletsMenu()
{
    if( m_addAppletsMenu->showing() )
        m_addAppletsMenu->hide();
    if( m_removeAppletsMenu->showing() )
    {   // hide again on double-click
        m_removeAppletsMenu->hide();
        return;
    }
    const qreal xpos = BORDER_PADDING;
    const qreal ypos = contentsRect().height() - m_removeAppletsMenu->boundingRect().height();

    m_removeAppletsMenu->setPos( xpos, ypos );
    m_removeAppletsMenu->show();
}

void
ColumnContainment::correctControlButtonPositions()
{
    if( m_addAppletsIcon && m_removeAppletsIcon )
    {
        qreal xpos = BORDER_PADDING;
        qreal ypos = boundingRect().height() - m_addAppletsIcon->size().height();

        m_addAppletsIcon->setPos( xpos, ypos );

        xpos += m_addAppletsIcon->size().width();

        m_removeAppletsIcon->setPos( xpos, ypos );

        m_addAppletsIcon->show();
        m_removeAppletsIcon->show();
    }
    if( m_switchRightIcon && m_switchLeftIcon )
    {
        // icon for switching to right side
        qreal xpos = boundingRect().width() / 2 + m_switchRightIcon->size().width() - 5;
        qreal ypos = boundingRect().height() - m_switchRightIcon->size().height();
        m_switchRightIcon->setPos( xpos, ypos );

        // icon for switching to left side
        xpos = boundingRect().width() / 2 - m_switchLeftIcon->size().width() + 5;
        m_switchLeftIcon->setPos( xpos, ypos );

        m_switchLeftIcon->show();
        m_switchRightIcon->show();
    }
    if( m_zoomInIcon && m_zoomOutIcon )
    {
        // we don;t know which icon is shown,
        // but we know only one is currently visible,
        // so put them in the same place
        // location is to the left of the switching arrows
        
        const qreal xpos = boundingRect().width() - m_zoomOutIcon->size().width();
        const qreal ypos = m_switchLeftIcon->pos().y();

        m_zoomOutIcon->setPos( xpos, ypos );
        m_zoomInIcon->setPos( xpos, ypos );

        if( view()->zoomLevel() == Plasma::DesktopZoom )
            m_zoomOutIcon->show();
        else if( view()->zoomLevel() == Plasma::GroupZoom )
           m_zoomInIcon->show();
    }
    const QFontMetrics fm( m_footer->font() );
    const QRect footerRect = fm.boundingRect( m_footer->text() );
    
    const int footerX = m_switchRightIcon->pos().x() - ( ( m_switchRightIcon->pos().x() - ( m_switchLeftIcon->pos().x() + m_switchLeftIcon->size().width() ) ) / 2 )
              - footerRect.width() / 2;
    
    const int footerY = m_switchLeftIcon->pos().y() + m_switchLeftIcon->size().height() / 2 - footerRect.height() / 2;
    m_footer->setPos( footerX , footerY );
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

Plasma::IconWidget*
ColumnContainment::addAction( QAction *action )
{
    if ( !action ) {
        debug() << "ERROR!!! PASSED INVALID ACTION";
        return 0;
    }

    Plasma::IconWidget *tool = new Plasma::IconWidget( this );

    tool->setAction( action );
    tool->setText( QString() );
    tool->setToolTip( action->text() );
    tool->setDrawBackground( false );
    tool->setOrientation( Qt::Horizontal );
    QSizeF iconSize = tool->sizeFromIconSize( 22 );
    tool->setMinimumSize( iconSize );
    tool->setMaximumSize( iconSize );
    tool->resize( iconSize );

    tool->hide();
    tool->setZValue( zValue() + 1 );

    return tool;
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
        debug() << "using false width of" << width << "instead of real:" << m_currentColumns << m_maxColumnWidth / m_currentColumns << m_maxColumnWidth;
    }
    else
        width = m_currentColumns ? m_maxColumnWidth / m_currentColumns : m_maxColumnWidth;
    int height = applet->effectiveSizeHint( Qt::PreferredSize, QSizeF( width, -1 ) ).height();
//     int rowSpan = qMin( qMax( ( int )( height )/ m_rowHeight , 1 ), 2 );

    //get the rowspan based on the aspect ratio and a constant.
    qreal aspectRatio = height / (qreal)width;
    int rowSpan = aspectRatio / 0.3;

    /*
    debug() << "calculating rowspan, aspectratio = height / width";
    debug() << aspectRatio << " = " << height << " / " << width;
    debug() << "rowspan = aspectRatio / 0.3";
    debug() << rowSpan << " = " << aspectRatio << " / 3"; */
    int colSpan = 1;

    if( rowSpan == 0 || ( aspectRatio * 100 > 30 && ( int )( aspectRatio * 100 ) % 30 > 15 ) )
        rowSpan++;

    rowSpan = qMin( rowSpan, m_currentRows );
    /*
    debug() << "current columns: " << m_currentColumns;
    debug() << "current rows: " << m_currentRows;
    debug() << "applet: " << applet->pluginName();
    debug() << "width: " << width;
    debug() << "height: " << height;
    debug() << "rowspan: " << rowSpan;
    */
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
        //most applets should work with rowSpan of at least two rows.
        if( consec == rowSpan || consec >= 2 )
        {
            positionFound = true;
            rowSpan = consec;
            row = i - rowSpan; //get the first of the consecutive rows
            col = j;
        }
        j++;
    }

    if( positionFound )
    {
        const QRectF rect = geometry();
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
        m_grid->setColumnMaximumWidth( col, INT_MAX );
        
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
    if( m_grid->count() == 1 )
        m_removeAppletsIcon->action()->setEnabled( true );

    return applet;
}

void
ColumnContainment::appletRemoved( Plasma::Applet* applet )
{
    DEBUG_BLOCK

    if( m_appletsPositions.contains( applet ) )
    {
        const QList<int> pos = m_appletsPositions[applet];
        const int row = pos[0];
        const int col = pos[1];
        const int rowSpan = pos[2];

        for( int i = 0; i < rowSpan; i++ )
            m_gridFreePositions[row + i][col] = true;

        m_appletsPositions.remove( applet );
        m_appletsIndexes.remove( applet );

        //keep the indexes updated
        const int idx = m_appletsIndexes[applet];
        foreach( const Plasma::Applet *a, m_appletsIndexes.keys() )
            if( m_appletsIndexes[a] > idx )
                m_appletsIndexes[a]--;

        if( m_grid->count() > 0 )
            rearrangeApplets( row + rowSpan, col );
        else
            m_removeAppletsIcon->action()->setEnabled( false );
    }
}

void
ColumnContainment::rearrangeApplets( int startRow, int startColumn )
{
    DEBUG_BLOCK

    int row = startRow;
    int columnCount = m_grid->columnCount();
    int columnsBeforeRearrange = columnCount;
    int rowCount = m_grid->rowCount();

    //Make applets ocuppy the space left by a removed applet
    for( int col = startColumn; col < columnCount; col++ )
    {        
        while( row < rowCount )
        {
            Plasma::Applet *applet = static_cast< Plasma::Applet* >( m_grid->itemAt( row, col ) );
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
                m_gridFreePositions[row + k][col] = true;

            foreach( const Plasma::Applet* a, m_appletsIndexes.keys() )
                if( m_appletsIndexes[a] > idx )
                    m_appletsIndexes[a]--;

            insertInGrid( applet );
            row += rowSpan;
        }
        row = 0;
    }

    //Expand applets width when a complete column has been removed
    columnCount = m_grid->columnCount();
    rowCount = m_grid->rowCount();
    if( ( startColumn == columnCount && startRow == rowCount - 1 ) || columnsBeforeRearrange > columnCount )
    {
        m_grid->setColumnMaximumWidth( columnCount, 0 );
    }
}

void
ColumnContainment::zoomInRequested() // SLOT
{
    emit zoomIn( this );
}

void ColumnContainment::zoomOutRequested() // SLOT
{
    emit zoomOut( this );
}

void
ColumnContainment::setView( ContextView *newView )
{
    m_view = newView;
    // all view-dependent signals, so we do it here
    connect( this, SIGNAL( changeContainment( Plasma::Containment*, int ) ), m_view, SLOT( setContainment( Plasma::Containment*, int ) ) );

    connect( this, SIGNAL( zoomOut( Plasma::Containment* ) ), m_view, SLOT( zoomOut( Plasma::Containment* ) ) );
    connect( this, SIGNAL( zoomIn( Plasma::Containment* ) ), m_view, SLOT( zoomIn( Plasma::Containment* ) ) );

    connect( m_addAppletsMenu, SIGNAL( changeContainment( Plasma::Containment * ) ),
             m_view, SLOT( setContainment( Plasma::Containment * ) ) );
    connect( m_removeAppletsMenu, SIGNAL( changeContainment( Plasma::Containment * ) ),
             m_view, SLOT( setContainment( Plasma::Containment * ) ) );

    connect( m_switchRightIcon, SIGNAL( clicked() ), m_view, SLOT( nextContainment() ) );
    connect( m_switchLeftIcon, SIGNAL( clicked() ), m_view, SLOT( previousContainment() ) );
}

void ColumnContainment::setZoomLevel( Plasma::ZoomLevel level )
{
    m_zoomLevel = level;
    correctControlButtonPositions();
    if( level == Plasma::DesktopZoom ) // zoomed in
    {
        // only option is to zoom out
        if( m_zoomInIcon && m_zoomOutIcon )
        {
            m_zoomInIcon->hide();
            m_zoomOutIcon->show();
        }
        m_switchRightIcon->show();
        m_switchLeftIcon->show();
        
        m_selectionLayer->hide();
        m_selectionLayer->setZValue( zValue() - 10000 ); //move down to avoid dead zones
        
            
    } else if( level == Plasma::GroupZoom )
    {
         // only option is to zoom in
         if( m_zoomInIcon && m_zoomOutIcon )
         {
            m_zoomOutIcon->hide();
            m_zoomInIcon->show();
         }
         // don't show switching arrows
         m_switchRightIcon->hide();
         m_switchLeftIcon->hide();
         
        m_selectionLayer->show();
        m_selectionLayer->setZValue( zValue() + 10000 ); //draw over applets  
    }
}

} // Context namespace


#include "ColumnContainment.moc"

