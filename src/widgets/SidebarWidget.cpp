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

#include "App.h"
#include "Debug.h"
#include "amarokconfig.h"

#include "SvgTinter.h"

#include <KIconEffect>

#include <QAbstractItemDelegate>
#include <QAction>
#include <QApplication>
#include <QList>
#include <QPainter>
#include <QTimer>
#include <QWheelEvent>


class SideBarWidget::Private
{
public:
    QList<SideBarButton*> buttons;
    QList<QAction*> actions;
    QList<QAction*> shortcuts;
    QAction *closeShortcut;
    QList<int> visible;
};

SideBarWidget::SideBarWidget( QWidget *parent )
    : super( parent )
    , d( new Private )
{
    setContextMenuPolicy( Qt::ActionsContextMenu );
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );
    d->closeShortcut = new QAction( this );
    d->closeShortcut->setVisible( false );
    d->closeShortcut->setShortcut( QKeySequence( Qt::CTRL | Qt::Key_0 ) );
    addAction( d->closeShortcut );
}

SideBarWidget::~SideBarWidget()
{
    DEBUG_BLOCK

    // Save index of active browser for session management
    int i;
    for( i = 0; i < d->buttons.count() && !d->buttons[i]->isChecked(); i++ ) {}
    AmarokConfig::setActiveBrowser( i );

    // Save list of visible browsers for session management
    AmarokConfig::setVisibleBrowsers( d->visible );

    AmarokConfig::self()->writeConfig();

    delete d;
}

void SideBarWidget::restoreSession()
{
    DEBUG_BLOCK

    QList<QAction*> browserActions;
    foreach( QAction* action, d->actions )
        if( !action->text().isEmpty() )
            browserActions << action;

    // Restore visible browsers
    for( int i = 0; i < d->visible.count(); i++ )
        if( !AmarokConfig::visibleBrowsers().contains( i ) )
            browserActions[i]->toggle();

    // Restore active browser
    const int index = AmarokConfig::activeBrowser();
    if( index < d->buttons.count() && !d->buttons[index]->isChecked() )
        d->buttons[index]->click();
}

int SideBarWidget::addSideBar( const QIcon &icon, const QString &text )
{
    SideBarButton *b = new SideBarButton( icon, text, this );
    connect( b, SIGNAL( clicked( bool ) ), this, SLOT( slotClicked( bool ) ) );
    d->visible.append( d->buttons.count() );
    d->buttons.append( b );

    QAction *a = new QAction( icon, text, this );
    a->setCheckable( true );
    a->setChecked( true );
    connect( a, SIGNAL( toggled( bool ) ), this, SLOT( slotSetVisible( bool ) ) );
    addAction( a );
    d->actions.append( a );

    QAction *s = new QAction( this );
    s->setVisible( false );
    connect( s, SIGNAL( triggered() ), b, SLOT( click() ) );
    s->setShortcut( QKeySequence( Qt::CTRL | ( Qt::Key_0 + d->visible.count() ) ) );
    addAction( s );
    d->shortcuts.append( s );

    if( d->buttons.count() == 1 ) // first one
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
            if( d->buttons[i] == sender() )
                index = i;
            else
                d->buttons[i]->setChecked( false );
        }
        d->closeShortcut->disconnect();
        connect( d->closeShortcut, SIGNAL( triggered() ), d->buttons[index], SLOT( click() ) );
        emit opened( index );
    }
    else
        emit closed();
}

void SideBarWidget::slotSetVisible( bool visible )
{
    DEBUG_BLOCK

    QAction *a = (QAction *)sender();
    const int index = d->actions.indexOf( a );
    if( d->visible.contains( index ) == visible )
        return;
    d->buttons[index]->setVisible( visible );
    if( visible )
        d->visible.append( index );
    else
        d->visible.removeAll( index );
    d->actions[ d->visible.first() ]->setEnabled( d->visible.count() != 1 );
    qSort( d->visible );
    if( !visible && d->buttons[index]->isChecked() )
    {
        d->buttons[index]->click();
        for( int i = 0, n = d->visible.count(); i < n; ++i )
            if( d->visible[i] != index )
            {
                d->buttons[ d->visible[i] ]->click();
                break;
            }
    }
    updateShortcuts();
}

int SideBarWidget::count() const
{
    return d->buttons.count();
}

QString SideBarWidget::text( int index ) const
{
    return d->buttons[index]->text();
}

QIcon SideBarWidget::icon( int index ) const
{
    return d->buttons[index]->icon();
}

void SideBarWidget::open( int index )
{
    d->buttons[ index ]->click();
}

void SideBarWidget::wheelEvent( QWheelEvent *e )
{
    if( !e->delta() ) // *shrug*
        return;

    int index;
    const int n = d->visible.count();
    int current = -1;
    for( int i = 0; i < n; ++i )
        if( d->buttons[ d->visible[i] ]->isChecked() )
        {
            index = d->visible[ qBound( 0, i - e->delta() / 120, n - 1 ) ];
            current = i;
            break;
        }
    if( current == -1 )
        index = d->visible[ qBound( 0, ( e->delta() > 0 ? n : -1 ) - e->delta() / 120, n - 1 ) ];
    if( current == -1 || index != d->visible[ current ] )
        d->buttons[ index ]->click();
}

void SideBarWidget::updateShortcuts()
{
    for( int i = 0, n = d->shortcuts.count(); i < n; ++i )
        d->shortcuts[i]->setShortcut( QKeySequence() );
    for( int i = 0, n = d->visible.count(); i < n; ++i )
        d->shortcuts[ d->visible[i] ]->setShortcut( QKeySequence( Qt::CTRL | ( Qt::Key_1 + i ) ) );
}


//////////////////////////////////////////////////////////////////////
// Class SideBarButton
//////////////////////////////////////////////////////////////////////

SideBarButton::SideBarButton( const QIcon &icon, const QString &text, QWidget *parent )
    : QAbstractButton( parent )
    , m_animCount( 0 )
    , m_animTimer( new QTimer( this ) )
{
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );
    setCheckable( true );
    setIcon( icon );
    setText( text );

    setForegroundRole( QPalette::HighlightedText );

    connect( m_animTimer, SIGNAL( timeout() ), this, SLOT( slotAnimTimer() ) );
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

    const int borderWidth = 8;
    const int borderHeight = 8;
    const int halfWidth = borderWidth / 2;
    const int halfHeight = borderHeight / 2;

    QString prefix = "sidebar_button_";
    QString shadow = "_shadow";
    if ( isDown() || isChecked() ) shadow = "_shadow_down";

    QImage topLeft_shadow = The::svgHandler()->renderSvg( prefix + "topleft" + shadow, borderWidth, borderHeight, prefix + "topleft" + shadow).toImage();
    QImage topLeft = The::svgHandler()->renderSvg( prefix + "topleft", halfWidth, halfHeight, prefix + "topleft" ).toImage();
    p.drawImage( 0, 0, topLeft_shadow );
    p.drawImage( halfWidth, halfHeight, topLeft );

    QImage top_shadow = The::svgHandler()->renderSvg( prefix + "top" + shadow, width() - ( 2 * borderWidth ), borderHeight, prefix + "top" + shadow).toImage();
    QImage top = The::svgHandler()->renderSvg( prefix + "top", width() - ( 2 * borderWidth ), halfHeight, prefix + "top" ).toImage();
    p.drawImage( borderWidth, 0, top_shadow );
    p.drawImage( borderWidth, halfHeight, top );

    QImage topRight_shadow = The::svgHandler()->renderSvg( prefix + "topright" + shadow, borderWidth, borderHeight, prefix + "topright" + shadow).toImage();
    QImage topRight = The::svgHandler()->renderSvg( prefix + "topright", halfWidth, halfHeight, prefix + "topright" ).toImage();
    p.drawImage( width() - borderWidth, 0, topRight_shadow );
    p.drawImage( width() - borderWidth, halfHeight, topRight );

    QImage right_shadow = The::svgHandler()->renderSvg( prefix + "right" + shadow, borderWidth, height() - ( 2 * borderHeight ), prefix + "right" + shadow).toImage();
    QImage right = The::svgHandler()->renderSvg( prefix + "right", halfWidth, height() - ( 2 * borderHeight ), prefix + "right" ).toImage();
    p.drawImage( width() - borderWidth, borderHeight, right_shadow );
    p.drawImage( width() - borderWidth, borderHeight, right);

    QImage bottomRight_shadow = The::svgHandler()->renderSvg( prefix + "bottomright" + shadow, borderWidth, borderHeight, prefix + "bottomright" + shadow).toImage();
    QImage bottomRight = The::svgHandler()->renderSvg( prefix + "bottomright", halfWidth, halfHeight, prefix + "bottomright" ).toImage();
    p.drawImage( width() - borderWidth, height() - borderHeight, bottomRight_shadow );
    p.drawImage( width() - borderWidth, height() - borderHeight, bottomRight );

    QImage bottom_shadow = The::svgHandler()->renderSvg( prefix + "bottom" + shadow, width() - 2 * borderWidth, borderHeight, prefix + "bottom" + shadow).toImage();
    QImage bottom = The::svgHandler()->renderSvg( prefix + "bottom", width() - 2 * borderWidth, halfHeight, prefix + "bottom" ).toImage();
    p.drawImage( borderWidth, height() - borderHeight, bottom_shadow );
    p.drawImage( borderWidth, height() - borderHeight, bottom );

    QImage bottomLeft_shadow = The::svgHandler()->renderSvg( prefix + "bottomleft" + shadow, borderWidth, borderHeight, prefix + "bottomleft" + shadow).toImage();
    QImage bottomLeft = The::svgHandler()->renderSvg( prefix + "bottomleft", halfWidth, halfHeight, prefix + "bottomleft" ).toImage();
    p.drawImage( 0, height() - borderHeight, bottomLeft_shadow );
    p.drawImage( halfWidth, height() - borderHeight , bottomLeft );

    QImage left_shadow = The::svgHandler()->renderSvg( prefix + "left" + shadow, borderWidth, height() - 2 * borderHeight, prefix + "left" + shadow).toImage();
    QImage left = The::svgHandler()->renderSvg( prefix + "left", halfWidth, height() - 2 * borderHeight, prefix + "left" ).toImage();
    p.drawImage( 0, borderHeight, left_shadow );
    p.drawImage( halfWidth, borderHeight, left );

    
    QImage center = The::svgHandler()->renderSvg( prefix + "center", width() - 2 * borderWidth, height() - 2 * borderHeight, prefix + "center" ).toImage();
    p.drawImage( borderWidth, borderHeight, center );

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


#include "SidebarWidget.moc"
