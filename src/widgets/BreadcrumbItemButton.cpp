/****************************************************************************************
 * Copyright (c) 2006 Peter Penz <peter.penz@gmx.at>                                    *
 * Copyright (c) 2006 Aaron Seigo <aseigo@kde.org>                                      *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "BreadcrumbItemButton.h"

#include "amarokurls/AmarokUrlAction.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "core/support/Amarok.h"

#include <KColorScheme>
#include <QIcon>
#include <KLocalizedString>
#include <QMenu>

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QMargins>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionFocusRect>

BreadcrumbItemButton::BreadcrumbItemButton( QWidget *parent )
    : Amarok::ElidingButton( parent )
    , m_displayHint( 0 )
{
    init();
}

BreadcrumbItemButton::BreadcrumbItemButton( const QString &text, QWidget *parent )
    : Amarok::ElidingButton( text, parent )
    , m_displayHint( 0 )
{
    init();
}

BreadcrumbItemButton::BreadcrumbItemButton( const QIcon &icon, const QString &text, QWidget *parent )
    : Amarok::ElidingButton( icon, text, parent )
    , m_displayHint( 0 )
{
    init();
}

void
BreadcrumbItemButton::init()
{
    setFocusPolicy( Qt::NoFocus );
    setDisplayHintEnabled( HoverHint, false );
}

BreadcrumbItemButton::~BreadcrumbItemButton()
{
}

void
BreadcrumbItemButton::setActive( const bool active )
{
    setDisplayHintEnabled( ActiveHint, active );

    QFont f = font();
    f.setBold( active );
    setFont( f );
}

void
BreadcrumbItemButton::setDisplayHintEnabled( DisplayHint hint, bool enable )
{
    if( enable )
        m_displayHint = m_displayHint | hint;
    else
        m_displayHint = m_displayHint & ~hint;

    update();
}

bool
BreadcrumbItemButton::isDisplayHintEnabled( DisplayHint hint ) const
{
    return (m_displayHint & hint) > 0;
}

void
BreadcrumbItemButton::enterEvent( QEvent* event )
{
    QPushButton::enterEvent( event );
    setDisplayHintEnabled( HoverHint, true );
    update();
}

void
BreadcrumbItemButton::leaveEvent( QEvent* event )
{
    QPushButton::leaveEvent( event );
    setDisplayHintEnabled( HoverHint, false );
    update();
}

void
BreadcrumbItemButton::paintEvent( QPaintEvent* event )
{
    Q_UNUSED(event);

    QPainter painter(this);

    const int buttonHeight = height();
    int buttonWidth = width();
    int preferredWidth = sizeHint().width();
    if (preferredWidth < minimumWidth()) {
        preferredWidth = minimumWidth();
    }
    if (buttonWidth > preferredWidth) {
        buttonWidth = preferredWidth;
    }
    drawHoverBackground(&painter);

    QMargins margins = contentsMargins();
    const int padding = 2;
    int xoffset;

    if( !icon().isNull() )
    {
        const int iconWidth = iconSize().width();
        const int iconHeight = iconSize().height();
        const int iconTop = ( (buttonHeight - margins.top() - margins.bottom()) - iconHeight ) / 2;
        const QRect iconRect( margins.left() + padding, iconTop, iconWidth, iconHeight );
        painter.drawPixmap( iconRect, icon().pixmap( iconSize() ) );
        xoffset = margins.left() + (padding * 2) + iconWidth;
    }
    else
        xoffset = margins.left() + (padding * 2);

    const QRect textRect( xoffset, margins.top(), buttonWidth, buttonHeight);
    painter.drawText(textRect, Qt::AlignVCenter, text());
}


void
BreadcrumbItemButton::drawHoverBackground(QPainter* painter)
{
    const bool isHovered = isDisplayHintEnabled( HoverHint );

    if( isHovered )
    {
        // QColor backgroundColor = palette().color(QPalette::Highlight);
        // TODO: the backgroundColor should be applied to the style
        QStyleOptionViewItem option;
        option.initFrom(this);
        option.state = QStyle::State_Enabled | QStyle::State_MouseOver;
        option.viewItemPosition = QStyleOptionViewItem::OnlyOne;
        style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter, this );
    }
}

QColor
BreadcrumbItemButton::foregroundColor() const
{
    const bool isHighlighted = isDisplayHintEnabled( HoverHint );
    const bool isActive = isDisplayHintEnabled( ActiveHint );

    QColor foregroundColor = palette().color( foregroundRole() );
    if( !isActive && !isHighlighted )
        foregroundColor.setAlpha( 180 );

    return foregroundColor;
}

QSize
BreadcrumbItemButton::sizeHint() const
{
    QSize size = Amarok::ElidingButton::sizeHint();
    int width = 8;
    if( !icon().isNull() )
    {
        width += iconSize().width();
    }
    if( !text().isEmpty() )
    {
        QFontMetrics fm( font() );
        width += fm.horizontalAdvance( text() );
    }
    size.setWidth( width );
    return size;
}


BreadcrumbItemMenuButton::BreadcrumbItemMenuButton( QWidget* parent )
    : BreadcrumbItemButton( parent )
{
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
}

void
BreadcrumbItemMenuButton::paintEvent( QPaintEvent* event )
{
    Q_UNUSED(event);

    QPainter painter(this);
    drawHoverBackground(&painter);

    const QColor fgColor = foregroundColor();

    QStyleOption option;
    option.initFrom(this);
    option.rect = QRect(0, 0, width(), height());
    option.palette = palette();
    option.palette.setColor(QPalette::Text, fgColor);
    option.palette.setColor(QPalette::WindowText, fgColor);
    option.palette.setColor(QPalette::ButtonText, fgColor);

    if (layoutDirection() == Qt::LeftToRight) {
        style()->drawPrimitive(QStyle::PE_IndicatorArrowRight, &option, &painter, this);
    } else {
        style()->drawPrimitive(QStyle::PE_IndicatorArrowLeft, &option, &painter, this);
    }
}



BreadcrumbUrlMenuButton::BreadcrumbUrlMenuButton( const QString &urlsCommand, QWidget *parent )
    : BreadcrumbItemButton( QIcon::fromTheme( "bookmark-new-list" ), QString(), parent )
    , m_urlsCommand( urlsCommand )
    , m_copyToClipboardAction( nullptr )
{
    setToolTip( i18n( "List and run bookmarks, or create new ones" ) );

    connect( this, &QAbstractButton::clicked, this, &BreadcrumbUrlMenuButton::showMenu );
}

BreadcrumbUrlMenuButton::~BreadcrumbUrlMenuButton()
{
}

void
BreadcrumbUrlMenuButton::generateMenu( const QPoint &pos )
{

    DEBUG_BLOCK

    BookmarkList list = The::amarokUrlHandler()->urlsByCommand( m_urlsCommand );

    QMenu * menu = new QMenu();
    menu->setTitle( i18n("Amarok Bookmarks" ) );

    if( m_urlsCommand == "navigate" )
        menu->addAction( Amarok::actionCollection()->action( "bookmark_browser" ) );
    else if( m_urlsCommand == "playlist" )
    {
        menu->addAction( Amarok::actionCollection()->action( "bookmark_playlistview" ) );
        debug()<<"Adding bookmark playlist action";
    }
    else if( m_urlsCommand == "context" )
    {
        menu->addAction( Amarok::actionCollection()->action( "bookmark_contextview" ) );
        debug()<<"Adding bookmark context view action";
    }
    else
        warning()<<"Bad URL command.";

    if( !m_copyToClipboardAction )
    {
        m_copyToClipboardAction = new QAction( QIcon::fromTheme( "klipper" ), i18n( "Copy Current View Bookmark to Clipboard" ), this );
        connect( m_copyToClipboardAction, &QAction::triggered, this, &BreadcrumbUrlMenuButton::copyCurrentToClipboard );
    }

    menu->addAction( m_copyToClipboardAction );

    menu->addAction( Amarok::actionCollection()->action( "bookmark_manager" ) );

    menu->addSeparator();

    for( AmarokUrlPtr url : list )
    {
        menu->addAction( new AmarokUrlAction( url, menu ) );
    }

    debug() << "showing menu at " << pos;
    menu->exec( pos );
    delete menu;

}

void
BreadcrumbUrlMenuButton::showMenu()
{
    QPoint pos( 0, height() );
    generateMenu( mapToGlobal( pos ) );
}

void
BreadcrumbUrlMenuButton::copyCurrentToClipboard()
{

    QString urlString;

    if( m_urlsCommand == "navigate" )
    {
        AmarokUrl url = The::amarokUrlHandler()->createBrowserViewBookmark();
        urlString = url.url();
    }
    else if( m_urlsCommand == "playlist" )
    {
        AmarokUrl url = The::amarokUrlHandler()->createPlaylistViewBookmark();
        urlString = url.url();
    }
    else if( m_urlsCommand == "context" )
    {
        AmarokUrl url = The::amarokUrlHandler()->createContextViewBookmark();
        urlString = url.url();
    }

    QApplication::clipboard()->setText( urlString );

}

