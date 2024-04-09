/****************************************************************************************
 * Copyright (c) 2003-2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2005-2013 Mark Kretschmann <kretschmann@kde.org>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "BlockAnalyzer.h"

#include "AnalyzerWorker.h"
#include "BlockRenderer.h"
#include "BlockWorker.h"
#include "PaletteHandler.h"
#include "core/support/Debug.h"

#include <cmath>

#include <QPainter>
#include <QQuickWindow>
#include <QScreen>
#include <QSGTexture>


BlockAnalyzer::BlockAnalyzer( QQuickItem *parent )
    : Analyzer::Base( parent )
    , m_columns( 0 )         //int
    , m_rows( 0 )            //int
    , m_fadeBarsPixmaps( FADE_SIZE ) //vector<QPixmap>
{
    setTextureFollowsItemSize( true );
    setObjectName( "Blocky" );

    m_columnWidth = config().readEntry( "columnWidth", 4 );
    m_fallSpeed = (FallSpeed) config().readEntry( "fallSpeed", (int) Medium );
    m_showFadebars = config().readEntry( "showFadebars", true );

    paletteChange( The::paletteHandler()->palette() );
    connect( The::paletteHandler(), &PaletteHandler::newPalette, this, &BlockAnalyzer::paletteChange );
    connect( this, &QQuickItem::windowChanged, this, &BlockAnalyzer::newWindow );
}

QQuickFramebufferObject::Renderer*
BlockAnalyzer::createRenderer() const
{
    return new BlockRenderer;
}

Analyzer::Worker * BlockAnalyzer::createWorker() const
{
    auto worker = new BlockWorker( m_rows, m_columns, m_step, m_showFadebars );
    if( window() )
        worker->setRefreshRate( window()->screen()->refreshRate() );
    connect( worker, &BlockWorker::finished, this, &QQuickFramebufferObject::update, Qt::QueuedConnection );
    connect( this, &BlockAnalyzer::stepChanged, worker, &BlockWorker::setStep, Qt::QueuedConnection );
    connect( this, &BlockAnalyzer::rowsChanged, worker, &BlockWorker::setRows, Qt::QueuedConnection );
    connect( this, &BlockAnalyzer::columnsChanged, worker, &BlockWorker::setColumns, Qt::QueuedConnection );
    connect( this, &BlockAnalyzer::refreshRateChanged, worker, &BlockWorker::setRefreshRate, Qt::QueuedConnection );
    connect( this, &BlockAnalyzer::showFadebarsChanged, worker, &BlockWorker::setShowFadebars, Qt::QueuedConnection );
    return worker;
}

void
BlockAnalyzer::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickFramebufferObject::geometryChanged( newGeometry, oldGeometry );

    if( !newGeometry.isValid() )
        return;

    const int oldRows = m_rows;

    // Rounded up so that the last column/line is covered if partially visible
    m_columns = std::lround( std::ceil( (double)newGeometry.width() / ( m_columnWidth + 1 ) ) );
    Q_EMIT columnsChanged( m_columns );
    m_rows    = std::ceil( (double)newGeometry.height() / ( BLOCK_HEIGHT + 1 ) );
    Q_EMIT rowsChanged( m_rows );

    setScopeSize( m_columns );

    if( m_rows != oldRows )
    {
        m_barPixmap = QPixmap( m_columnWidth, m_rows * ( BLOCK_HEIGHT + 1 ) );

        determineStep();
        paletteChange( The::paletteHandler()->palette() );
    }
    else
        drawBackground( The::paletteHandler()->palette() );
}

void
BlockAnalyzer::determineStep()
{
    // falltime is dependent on rowcount due to our digital resolution (ie we have boxes/blocks of pixels)

    const qreal fallTime = 1.0 / pow( 1.5, m_fallSpeed );  // time to fall from top to bottom
    m_step = qreal( m_rows ) / fallTime;  // the amount of rows to fall per second
    Q_EMIT stepChanged( m_step );
}

void
BlockAnalyzer::paletteChange( const QPalette& palette ) //virtual
{
    const QColor bg = palette.color( QPalette::Active, QPalette::Base );
    const QColor abg = palette.color( QPalette::Active, QPalette::AlternateBase );
    const QColor highlight = palette.color( QPalette::Active, QPalette::Highlight );

    m_topBarPixmap = QPixmap( m_columnWidth, BLOCK_HEIGHT );
    m_topBarPixmap.fill( highlight );

    m_barPixmap.fill( QColor( ( highlight.red() + bg.red() ) / 2, ( highlight.green() + bg.green() ) / 2, ( highlight.blue() + bg.blue() ) / 2 ) );

    int h, s, v;
    palette.color( QPalette::Active, QPalette::Dark ).getHsv( &h, &s, &v );
    const QColor fade = QColor::fromHsv( h + 30, s, v );

    const double dr = fade.red() - abg.red();
    const double dg = fade.green() - abg.green();
    const double db = fade.blue() - abg.blue();
    const int r = abg.red(), g = abg.green(), b = abg.blue();

    if( m_rows == 0 )
        return; // Nothing to draw yet, avoid some QPainter warnings from empty surfaces

    // Precalculate all fade-bar pixmaps
    for( int y = 0; y < FADE_SIZE; ++y )
    {
        m_fadeBarsPixmaps[y] = QPixmap( m_columnWidth, m_rows * ( BLOCK_HEIGHT + 1 ) );

        m_fadeBarsPixmaps[y].fill( palette.color( QPalette::Active, QPalette::Base ) );
        const double Y = 1.0 - ( log10( ( FADE_SIZE ) - y ) / log10( ( FADE_SIZE ) ) );
        QPainter f( &m_fadeBarsPixmaps[y] );
        for( int z = 0; z < m_rows; ++z )
            f.fillRect( 0,
                        z * ( BLOCK_HEIGHT + 1 ),
                        m_columnWidth, BLOCK_HEIGHT,
                        QColor( r + int( dr * Y ), g + int( dg * Y ), b + int( db * Y ) ) );
    }

    m_pixmapsChanged = true;
    drawBackground( palette );
}

void
BlockAnalyzer::drawBackground( const QPalette &palette )
{
    const QColor bg = palette.color( QPalette::Active, QPalette::Base );
    const QColor abg = palette.color( QPalette::Active, QPalette::AlternateBase );

    // background gets stretched if it is too big
    m_backgroundPixmap = QPixmap( width(), height() );
    m_backgroundPixmap.fill( bg );

    QPainter p( &m_backgroundPixmap );
    p.scale( 1/QGuiApplication::primaryScreen()->devicePixelRatio(), 1/QGuiApplication::primaryScreen()->devicePixelRatio() );
    for( int x = 0; x < m_columns; ++x )
        for( int y = 0; y < m_rows; ++y )
            p.fillRect( x * ( m_columnWidth + 1 ), y * ( BLOCK_HEIGHT + 1 ), m_columnWidth, BLOCK_HEIGHT, abg );

    m_pixmapsChanged = true;

    update();
}

void
BlockAnalyzer::setFallSpeed( FallSpeed fallSpeed )
{
    DEBUG_BLOCK

    debug() << "Fall speed set to:" << fallSpeed;

    if( m_fallSpeed == fallSpeed )
        return;

    m_fallSpeed = fallSpeed;
    config().writeEntry( "fallSpeed", (int) m_fallSpeed );
    Q_EMIT fallSpeedChanged();

    determineStep();
}

void
BlockAnalyzer::setColumnWidth( int columnWidth )
{
    DEBUG_BLOCK

    debug() << "Column width set to:" << columnWidth;

    if( columnWidth < 1 )
    {
        warning() << "Column width can not be smaller than one!";
        columnWidth = 1;
    }

    if( m_columnWidth == columnWidth )
        return;

    m_columnWidth = columnWidth;
    config().writeEntry( "columnWidth", m_columnWidth );
    Q_EMIT columnWidthChanged();

    m_columns = std::lround( std::ceil( (double)width() / ( m_columnWidth + 1 ) ) );
    Q_EMIT columnsChanged( m_columns );
    setScopeSize( m_columns );
    m_barPixmap = QPixmap( m_columnWidth, m_rows * ( BLOCK_HEIGHT + 1 ) );
    paletteChange( The::paletteHandler()->palette() );
}

void
BlockAnalyzer::setShowFadebars( bool showFadebars )
{
    DEBUG_BLOCK

    debug() << "Show fadebars:" << showFadebars;

    if( m_showFadebars == showFadebars )
        return;

    m_showFadebars = showFadebars;
    Q_EMIT showFadebarsChanged( m_showFadebars );
}

void
BlockAnalyzer::newWindow( QQuickWindow* window )
{
    if( window )
        Q_EMIT refreshRateChanged( window->screen()->refreshRate() );
}
