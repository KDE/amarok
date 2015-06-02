/****************************************************************************************
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2008-2009 Seb Ruiz <ruiz@kde.org>                                      *
 * Copyright (c) 2009 Roman Jarosz <kedgedev@gmail.com>                                 *
 * Copyright (c) 2009 Daniel Dewald <Daniel.Dewald@time-shift.de>                       *
 * Copyright (c) 2012 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "Token.h"
#include "TokenDropTarget.h"

#include <KApplication>

#include <QHBoxLayout>
#include <QPainter>
#include <QPen>

#include <QPixmap>
#include <QMimeData>
#include <QMouseEvent>
#include <QDrag>


Token*
TokenFactory::createToken(const QString & text, const QString & iconName, qint64 value, QWidget * parent) const
{
    return new Token( text, iconName, value, parent );
}

Token*
TokenFactory::createTokenFromMime( const QMimeData* mimeData, QWidget* parent ) const
{
    // decode the stream created in Token::mimeData
    QByteArray itemData = mimeData->data( Token::mimeType() );
    QDataStream dataStream(&itemData, QIODevice::ReadOnly);

    QString tokenName;
    QString tokenIconName;
    qint64 tokenValue;
    QColor tokenTextColor;

    dataStream >> tokenName;
    dataStream >> tokenIconName;
    dataStream >> tokenValue;
    dataStream >> tokenTextColor;

    Token* token = createToken( tokenName, tokenIconName, tokenValue, parent );
    token->setTextColor( tokenTextColor );

    return token;
}


Token::Token( const QString &name, const QString &iconName, qint64 value, QWidget *parent )
    : QWidget( parent )
    , m_name( name )
    , m_icon( QIcon::fromTheme( iconName ) )
    , m_iconName( iconName )
    , m_value( value )
    , m_customColor( false )
{
    setFocusPolicy( Qt::StrongFocus );

    // Note: We set all the margins because we need a quite small size.
    // Vertically for the EditPlaylistLayoutDialog and horizontally for
    // the OrganizeTracksDialog

    m_label = new QLabel( this );
    m_label->setAlignment( Qt::AlignCenter );
    m_label->setContentsMargins( 0, 0, 0, 0 );
    m_label->setMargin( 0 );
    m_label->setText( name );

    m_iconContainer = new QLabel( this );
    m_iconContainer->setContentsMargins( 0, 0, 0, 0 );
    m_iconContainer->setMargin( 0 );
    QPixmap pixmap = QPixmap( icon().pixmap( 16, 16 ) );
    m_iconContainer->setPixmap( pixmap );

    QHBoxLayout *hlayout = new QHBoxLayout( this );
    hlayout->setSpacing( 3 );
    hlayout->setContentsMargins( 3, 0, 3, 0 ); // to allow the label to overwrite the border if space get's tight
    hlayout->addWidget( m_iconContainer );
    hlayout->addWidget( m_label );
    setLayout( hlayout );

    updateCursor();
}

QString
Token::name() const
{
    return m_name;
}

qint64
Token::value() const
{
    return m_value;
}

QIcon
Token::icon() const
{
    return m_icon;
}

QString Token::iconName() const
{
    return m_iconName;
}

QColor Token::textColor() const
{
    return m_label->palette().color( QPalette::WindowText );
}

void
Token::setTextColor( QColor textColor )
{
    m_customColor = true;
    if( textColor == this->textColor() )
        return;
    QPalette myPalette( m_label->palette() );
    myPalette.setColor( QPalette::WindowText, textColor );
    m_label->setPalette( myPalette );
}

QMimeData*
Token::mimeData() const
{
    QByteArray itemData;

    QDataStream dataStream( &itemData, QIODevice::WriteOnly );
    dataStream << name() << iconName() << value() << textColor();

    QMimeData *mimeData = new QMimeData;
    mimeData->setData( mimeType(), itemData );

    return mimeData;
}

QString
Token::mimeType()
{
    return QLatin1String( "application/x-amarok-tag-token" );
}

QSize
Token::sizeHint() const
{
    QSize result = QWidget::sizeHint();
    result += QSize( 6, 6 ); // the border

    return result;
}

QSize
Token::minimumSizeHint() const
{
    QSize result = QWidget::minimumSizeHint();

    return result;
}

void
Token::changeEvent( QEvent *event )
{
    QWidget::changeEvent( event );
    if( !event || event->type() == QEvent::EnabledChange )
        updateCursor();
}

void
Token::focusInEvent( QFocusEvent* event )
{
    QWidget::focusInEvent( event );
    emit gotFocus( this );
}

void
Token::updateCursor()
{
    if( isEnabled() )
        setCursor( Qt::OpenHandCursor );
    else
        unsetCursor();
}

void
Token::mousePressEvent( QMouseEvent* event )
{
    if( event->button() == Qt::LeftButton )
        m_startPos = event->pos();            //store the start position
    QWidget::mousePressEvent( event );    //feed it to parent's event
}

void
Token::mouseMoveEvent( QMouseEvent* event )
{
    if( isEnabled() &&
        event->buttons() & Qt::LeftButton )
    {
        int distance = ( event->pos() - m_startPos ).manhattanLength();
        if ( distance >= KApplication::startDragDistance() )
            performDrag();
    }
    QWidget::mouseMoveEvent( event );
}

//Handles the creation of a QDrag object that carries the (text-only) QDataStream from an item in TokenPool
void
Token::performDrag()
{
    bool stacked = parentWidget() && qobject_cast<TokenDropTarget*>( parentWidget() ); // true if token originated from a TokenDropTarget.
    if( stacked )
        hide();

    Token *token = this;

    QDrag *drag = new QDrag( this );
    drag->setMimeData( mimeData() );

    // icon for pointer
    QPixmap pixmap( token->size() );
    token->render( &pixmap );
    drag->setPixmap( pixmap );
    drag->setHotSpot ( pixmap.rect().center() );

    Qt::DropAction dropAction = drag->exec( Qt::MoveAction | Qt::CopyAction, Qt::CopyAction );

    if( dropAction != Qt::MoveAction && dropAction != Qt::CopyAction ) // dragged out and not just dragged to another position.
    {
        // TODO: nice poof animation? ;-)
        token->deleteLater();
    }

}

void Token::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED( pe )

    QPainter p( this );
    p.setBrush( Qt::NoBrush );
    p.setRenderHint( QPainter::Antialiasing );
    QColor c;
    if( isEnabled() && hasFocus() )
    {
        c = palette().color( QPalette::Highlight );
    }
    else if( isEnabled() )
    {
        c = palette().color( foregroundRole() );
        c.setAlpha( c.alpha() * 0.5 );
    }
    else
    {
        c = palette().color( foregroundRole() );
        c.setAlpha( c.alpha() * 0.3 );
    }
    p.setPen( QPen( c, 2 ) );
    p.drawRoundedRect( rect().adjusted(1,1,-1,-1), 4, 4 );
    p.end();
}


