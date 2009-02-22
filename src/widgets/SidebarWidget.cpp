/*
  Copyright (c) 2006 Gábor Lehel <illissius@gmail.com>
  Copyright (c) 2007-2008 Mark Kretschmann <kretschmann@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "SidebarWidget.h"

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "amarokconfig.h"

#include "SvgTinter.h"

#include <QAbstractItemDelegate>
#include <QApplication>
#include <QPainter>
#include <QTimer>
#include <QWheelEvent>


SideBarButton::SideBarButton( const QIcon &icon, const QString &text, QWidget *parent )
    : QAbstractButton( parent )
    , m_animCount( 0 )
    , m_animTimer( new QTimer( this ) )
    , m_autoOpenTimer( new QTimer( this ) )
{
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );
    setCheckable( true );
    setAcceptDrops( true );
    setIcon( icon );
    setText( text );

    setForegroundRole( QPalette::HighlightedText );

    m_autoOpenTimer->setSingleShot( true );

    connect( m_animTimer, SIGNAL( timeout() ), this, SLOT( slotAnimTimer() ) );
    connect( m_autoOpenTimer, SIGNAL( timeout() ), this, SLOT( click() ) );
}

QSize SideBarButton::sizeHint() const
{
    return QSize( widthHint(), heightHint() ).expandedTo( QApplication::globalStrut() );
}

int SideBarButton::widthHint() const
{
    int width = 0;
    if( !icon().isNull() )
        width = iconSize().width();
    if( !width && text().isEmpty() )
        width = fontMetrics().size( Qt::TextShowMnemonic, "TEXT" ).height();
    else
        width = qMax( width, fontMetrics().size( Qt::TextShowMnemonic, text() ).height() );
    return width + 8;
}

int SideBarButton::heightHint() const
{
    int height = 0;
    if( !icon().isNull() )
        height = iconSize().height();
    if( !height && text().isEmpty() )
        height = fontMetrics().size( Qt::TextShowMnemonic, "Text" ).width();
    else
        height += fontMetrics().size( Qt::TextShowMnemonic, text() ).width();
    return height + 12;
}

void SideBarButton::dragEnterEvent( QDragEnterEvent* event )
{
    event->accept(); // so we can get a possible QDragLeaveEvent
    if( !isChecked() )
        m_autoOpenTimer->start( AUTO_OPEN_TIME );
}

void SideBarButton::dragLeaveEvent( QDragLeaveEvent* )
{
    m_autoOpenTimer->stop();
}

void SideBarButton::enterEvent( QEvent* )
{
    m_animEnter = true;
    m_animCount = 0;

    m_animTimer->start( ANIM_INTERVAL );
}

void SideBarButton::leaveEvent( QEvent* )
{
    // This can happen if you enter and leave the tab quickly
    if ( m_animCount == 0 )
        m_animCount = 1;

    m_animEnter = false;
    m_animTimer->start( ANIM_INTERVAL );
}

void SideBarButton::slotAnimTimer()
{
    if ( m_animEnter ) {
        m_animCount += 1;
        repaint();
        if ( m_animCount >= ANIM_MAX )
            m_animTimer->stop();
    } else {
        m_animCount -= 1;
        repaint();
        if ( m_animCount <= 0 )
            m_animTimer->stop();
    }
}

void SideBarButton::paintEvent( QPaintEvent* )
{
    QPainter p( this );
    p.setRenderHint ( QPainter::Antialiasing, false );

    static const int borderWidth = 8;
    static const int borderHeight = 8;
    static const int halfWidth = borderWidth / 2;
    static const int halfHeight = borderHeight / 2;

    const QString prefix = "sidebar_button_";
    QString shadow = "_shadow";
    if ( isDown() || isChecked() )
        shadow = "_shadow_down";

    QPixmap topLeft_shadow = The::svgHandler()->renderSvg( prefix + "topleft" + shadow, borderWidth, borderHeight, prefix + "topleft" + shadow);
    QPixmap topLeft = The::svgHandler()->renderSvg( prefix + "topleft", halfWidth, halfHeight, prefix + "topleft" );
    p.drawPixmap( 0, 0, topLeft_shadow );
    p.drawPixmap( halfWidth, halfHeight, topLeft );

    QPixmap top_shadow = The::svgHandler()->renderSvg( prefix + "top" + shadow, width() - ( 2 * borderWidth ), borderHeight, prefix + "top" + shadow);
    QPixmap top = The::svgHandler()->renderSvg( prefix + "top", width() - ( 2 * borderWidth ), halfHeight, prefix + "top" );
    p.drawPixmap( borderWidth, 0, top_shadow );
    p.drawPixmap( borderWidth, halfHeight, top );

    QPixmap topRight_shadow = The::svgHandler()->renderSvg( prefix + "topright" + shadow, borderWidth, borderHeight, prefix + "topright" + shadow);
    QPixmap topRight = The::svgHandler()->renderSvg( prefix + "topright", halfWidth, halfHeight, prefix + "topright" );
    p.drawPixmap( width() - borderWidth, 0, topRight_shadow );
    p.drawPixmap( width() - borderWidth, halfHeight, topRight );

    QPixmap right_shadow = The::svgHandler()->renderSvg( prefix + "right" + shadow, borderWidth, height() - ( 2 * borderHeight ), prefix + "right" + shadow);
    QPixmap right = The::svgHandler()->renderSvg( prefix + "right", halfWidth, height() - ( 2 * borderHeight ), prefix + "right" );
    p.drawPixmap( width() - borderWidth, borderHeight, right_shadow );
    p.drawPixmap( width() - borderWidth, borderHeight, right);

    QPixmap bottomRight_shadow = The::svgHandler()->renderSvg( prefix + "bottomright" + shadow, borderWidth, borderHeight, prefix + "bottomright" + shadow);
    QPixmap bottomRight = The::svgHandler()->renderSvg( prefix + "bottomright", halfWidth, halfHeight, prefix + "bottomright" );
    p.drawPixmap( width() - borderWidth, height() - borderHeight, bottomRight_shadow );
    p.drawPixmap( width() - borderWidth, height() - borderHeight, bottomRight );

    QPixmap bottom_shadow = The::svgHandler()->renderSvg( prefix + "bottom" + shadow, width() - 2 * borderWidth, borderHeight, prefix + "bottom" + shadow);
    QPixmap bottom = The::svgHandler()->renderSvg( prefix + "bottom", width() - 2 * borderWidth, halfHeight, prefix + "bottom" );
    p.drawPixmap( borderWidth, height() - borderHeight, bottom_shadow );
    p.drawPixmap( borderWidth, height() - borderHeight, bottom );

    QPixmap bottomLeft_shadow = The::svgHandler()->renderSvg( prefix + "bottomleft" + shadow, borderWidth, borderHeight, prefix + "bottomleft" + shadow);
    QPixmap bottomLeft = The::svgHandler()->renderSvg( prefix + "bottomleft", halfWidth, halfHeight, prefix + "bottomleft" );
    p.drawPixmap( 0, height() - borderHeight, bottomLeft_shadow );
    p.drawPixmap( halfWidth, height() - borderHeight , bottomLeft );

    QPixmap left_shadow = The::svgHandler()->renderSvg( prefix + "left" + shadow, borderWidth, height() - 2 * borderHeight, prefix + "left" + shadow);
    QPixmap left = The::svgHandler()->renderSvg( prefix + "left", halfWidth, height() - 2 * borderHeight, prefix + "left" );
    p.drawPixmap( 0, borderHeight, left_shadow );
    p.drawPixmap( halfWidth, borderHeight, left );

    
    QPixmap center = The::svgHandler()->renderSvg( prefix + "center", width() - 2 * borderWidth, height() - 2 * borderHeight, prefix + "center" );
    p.drawPixmap( borderWidth, borderHeight, center );

    const int pos = qMin( height(), height() / 2 + heightHint() / 2 ) - iconSize().height();
    const QString txt = text().remove( '&' );

    p.translate( 0, pos );
    p.drawPixmap( width() / 2 - iconSize().width() / 2, 0, icon().pixmap( iconSize() ) );
    p.translate( fontMetrics().size( Qt::TextShowMnemonic, txt ).height() - 2, 0 );
    p.rotate( -90 );

    p.setPen( QPen( App::instance()->palette().text().color() ) );
    p.drawText( 10, 0, QAbstractItemDelegate::elidedText( fontMetrics(), pos - 10, Qt::ElideRight, txt ) );
}

QColor SideBarButton::blendColors( const QColor& color1, const QColor& color2, int percent ) //static
{
    const float factor1 = ( 100 - ( float ) percent ) / 100;
    const float factor2 = ( float ) percent / 100;

    const int r = static_cast<int>( color1.red() * factor1 + color2.red() * factor2 );
    const int g = static_cast<int>( color1.green() * factor1 + color2.green() * factor2 );
    const int b = static_cast<int>( color1.blue() * factor1 + color2.blue() * factor2 );

    QColor result;
    result.setRgb( r, g, b );

    return result;
}

void SideBarButton::paletteChange(const QPalette & /*oldPalette*/)
{
    The::svgHandler()->reTint();
    repaint( 0, 0, -1,-1 );
}


//////////////////////////////////////////////////////////////////////
// Class SideBarWidget
//////////////////////////////////////////////////////////////////////

SideBarWidget::SideBarWidget( QWidget *parent )
    : super( parent )
{
    setContextMenuPolicy( Qt::ActionsContextMenu );
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );
    m_closeShortcut = new QAction( this );
    m_closeShortcut->setVisible( false );
    m_closeShortcut->setShortcut( QKeySequence( Qt::CTRL | Qt::Key_0 ) );
    addAction( m_closeShortcut );
}

SideBarWidget::~SideBarWidget()
{
    DEBUG_BLOCK

    // Save index of active browser for session management
    int i;
    for( i = 0; i < m_buttons.count() && !m_buttons.at( i )->isChecked(); i++ ) {}
    AmarokConfig::setActiveBrowser( i );

    // Save list of visible browsers for session management
    AmarokConfig::setVisibleBrowsers( m_visible );

    AmarokConfig::self()->writeConfig();
}

void SideBarWidget::restoreSession()
{
    DEBUG_BLOCK

    QList<QAction*> browserActions;
    foreach( QAction* action, m_actions )
        if( !action->text().isEmpty() )
            browserActions << action;

    // Restore visible browsers
    if( !AmarokConfig::visibleBrowsers().isEmpty() )
        for( int i = 0; i < m_visible.count(); i++ )
            if( !AmarokConfig::visibleBrowsers().contains( i ) )
                browserActions[i]->toggle();

    // Restore active browser
    const int index = AmarokConfig::activeBrowser();
    if( index < m_buttons.count() && !m_buttons.at(index)->isChecked() )
        m_buttons.at(index)->click();
}

int SideBarWidget::addSideBar( const QIcon &icon, const QString &text )
{
    SideBarButton *b = new SideBarButton( icon, text, this );
    connect( b, SIGNAL( clicked( bool ) ), this, SLOT( slotClicked( bool ) ) );
    m_visible.append( m_buttons.count() );
    m_buttons.append( b );

    QAction *a = new QAction( icon, text, this );
    a->setCheckable( true );
    a->setChecked( true );
    connect( a, SIGNAL( toggled( bool ) ), this, SLOT( slotSetVisible( bool ) ) );
    addAction( a );
    m_actions.append( a );

    QAction *s = new QAction( this );
    s->setVisible( false );
    connect( s, SIGNAL( triggered() ), b, SLOT( click() ) );
    s->setShortcut( QKeySequence( Qt::CTRL | ( Qt::Key_0 + m_visible.count() ) ) );
    addAction( s );
    m_shortcuts.append( s );

    if( m_buttons.count() == 1 ) // first one
        b->click();               // open it

    return count() - 1;
}

void SideBarWidget::slotClicked( bool checked )
{
    if( checked )
    {
        int index = -1;
        for( int i = 0, n = count(); i < n; ++i )
        {
            if( m_buttons[i] == sender() )
                index = i;
            else
                m_buttons[i]->setChecked( false );
        }
        m_closeShortcut->disconnect();
        connect( m_closeShortcut, SIGNAL( triggered() ), m_buttons[index], SLOT( click() ) );
        emit opened( index );
    }
    else
        emit closed();
}

void SideBarWidget::slotSetVisible( bool visible )
{
    DEBUG_BLOCK

    QAction *a = (QAction *)sender();
    const int index = m_actions.indexOf( a );
    if( m_visible.contains( index ) == visible )
        return;
    m_buttons[index]->setVisible( visible );
    if( visible )
        m_visible.append( index );
    else
        m_visible.removeAll( index );
    m_actions[ m_visible.first() ]->setEnabled( m_visible.count() != 1 );
    qSort( m_visible );
    if( !visible && m_buttons[index]->isChecked() )
    {
        m_buttons[index]->click();
        for( int i = 0, n = m_visible.count(); i < n; ++i )
            if( m_visible[i] != index )
            {
                m_buttons[ m_visible[i] ]->click();
                break;
            }
    }
    updateShortcuts();
}

int SideBarWidget::count() const
{
    return m_buttons.count();
}

QString SideBarWidget::text( int index ) const
{
    return m_buttons[index]->text();
}

QIcon SideBarWidget::icon( int index ) const
{
    return m_buttons[index]->icon();
}

void SideBarWidget::open( int index )
{
    m_buttons[ index ]->click();
}

void SideBarWidget::wheelEvent( QWheelEvent *e )
{
    if( !e->delta() ) // *shrug*
        return;

    int index;
    const int n = m_visible.count();
    int current = -1;
    for( int i = 0; i < n; ++i )
        if( m_buttons[ m_visible[i] ]->isChecked() )
        {
            index = m_visible[ qBound( 0, i - e->delta() / 120, n - 1 ) ];
            current = i;
            break;
        }
    if( current == -1 )
        index = m_visible[ qBound( 0, ( e->delta() > 0 ? n : -1 ) - e->delta() / 120, n - 1 ) ];
    if( current == -1 || index != m_visible[ current ] )
        m_buttons[ index ]->click();
}

void SideBarWidget::updateShortcuts()
{
    for( int i = 0, n = m_shortcuts.count(); i < n; ++i )
        m_shortcuts[i]->setShortcut( QKeySequence() );
    for( int i = 0, n = m_visible.count(); i < n; ++i )
        m_shortcuts[ m_visible[i] ]->setShortcut( QKeySequence( Qt::CTRL | ( Qt::Key_1 + i ) ) );
}


#include "SidebarWidget.moc"

