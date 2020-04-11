/****************************************************************************************
 * Copyright (c) 2009 Thomas Luebking <thomas.luebking@web.de>                          *
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

#include "TokenDropTarget.h"

#include "Token.h"
#include "TokenPool.h"
#include "core/support/Debug.h"

#include <KLocalizedString>

#include <QDropEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QMimeData>

TokenDropTarget::TokenDropTarget( QWidget *parent )
    : QWidget( parent )
    , m_rowLimit( 0 )
    , m_rows( 0 )
    , m_horizontalStretch( false ) // DANGER: m_horizontalStretch is used as int in the following code, assuming that true == 1
    , m_verticalStretch( true )
    , m_tokenFactory( new TokenFactory() )
{
    setAcceptDrops( true );

    QBoxLayout* mainLayout = new QVBoxLayout( this );
    mainLayout->setSpacing( 0 );
    mainLayout->addStretch( 1 ); // the vertical stretch

    mainLayout->setContentsMargins( 0, 0, 0, 0 );
}

TokenDropTarget::~TokenDropTarget()
{
    delete m_tokenFactory;
}

QSize
TokenDropTarget::sizeHint() const
{
    QSize result = QWidget::sizeHint();

     // we need at least space for the "empty" text.
    int h = fontMetrics().height();
    result = result.expandedTo( QSize( 36 * h, 2 * h ) );

    return result;
}

QSize
TokenDropTarget::minimumSizeHint() const
{
    QSize result = QWidget::minimumSizeHint();

     // we need at least space for the "empty" text.
    int h = fontMetrics().height();
    result = result.expandedTo( QSize( 36 * h, 2 * h ) );

    return result;
}


QHBoxLayout *
TokenDropTarget::appendRow()
{
    QHBoxLayout *box = new QHBoxLayout;
    box->setSpacing( 0 );
    if( m_horizontalStretch )
        box->addStretch();
    static_cast<QVBoxLayout*>(layout())->insertLayout( rows(), box );
    m_rows++;
    return box;
}

void
TokenDropTarget::clear()
{
    QList< Token *> allTokens = tokensAtRow();
    foreach( Token* token, allTokens )
        delete token;

    Q_EMIT changed();
}

int
TokenDropTarget::count() const
{
    int c = 0;
    for( int row = rows() - 1; row >= 0; --row )
        if( QBoxLayout *box = qobject_cast<QBoxLayout*>( layout()->itemAt( row )->layout() ) )
            c += box->count() - m_horizontalStretch;

    return c;
}

void
TokenDropTarget::setRowLimit( uint r )
{
    // if we have more than one row we have a stretch at the end.
    QBoxLayout *mainLayout = qobject_cast<QBoxLayout*>( layout() );
    if( ( r == 1 ) && (m_rowLimit != 1 ) )
        mainLayout->takeAt( mainLayout->count() - 1 );
    else if( ( r != 1 ) && (m_rowLimit == 1 ) )
        mainLayout->addStretch( 1 ); // the vertical stretch

    m_rowLimit = r;
}

void
TokenDropTarget::deleteEmptyRows()
{
    DEBUG_BLOCK;

    for( int row = rows() - 1; row >= 0; --row )
    {
        QBoxLayout *box = qobject_cast<QBoxLayout*>( layout()->itemAt( row )->layout() );
        if( box && box->count() < ( 1 + m_horizontalStretch ) ) // sic! last is spacer
        {
            delete layout()->takeAt( row );
            m_rows--;
        }
    }
    update(); // this removes empty layouts somehow for deleted tokens. don't remove
}

QList< Token *>
TokenDropTarget::tokensAtRow( int row )
{
    DEBUG_BLOCK;

    int lower = 0;
    int upper = (int)rows();
    if( row > -1 && row < (int)rows() )
    {
        lower = row;
        upper = row + 1;
    }

    QList< Token *> list;
    Token *token;
    for( row = lower; row < upper; ++row )
        if ( QHBoxLayout *rowBox = qobject_cast<QHBoxLayout*>( layout()->itemAt( row )->layout() ) )
        {
            for( int col = 0; col < rowBox->count() - m_horizontalStretch; ++col )
                if ( ( token = qobject_cast<Token*>( rowBox->itemAt( col )->widget() ) ) )
                    list << token;
        }

    debug() << "Row:"<<row<<"items:"<<list.count();

    return list;
}

void
TokenDropTarget::insertToken( Token *token, int row, int col )
{
    // - copy the token if it belongs to a token pool (fix BR 296136)
    if( qobject_cast<TokenPool*>(token->parent() ) ) {
        debug() << "Copying token" << token->name();
        token = m_tokenFactory->createToken( token->name(),
                                             token->iconName(),
                                             token->value() );
    }

    token->setParent( this );

    // - validate row
    if ( row < 0 && rowLimit() && rows() >= rowLimit() )
        row = rowLimit() - 1; // want to append, but we can't so use the last row instead

    QBoxLayout *box;
    if( row < 0 || row >= (int)rows() )
        box = appendRow();
    else
        box = qobject_cast<QBoxLayout*>( layout()->itemAt( row )->layout() );

    // - validate col
    if( col < 0 || col > box->count() - ( 1 + m_horizontalStretch ) )
        col = box->count() - m_horizontalStretch;

    // - copy the token if it belongs to a token pool (fix BR 296136)
    if( qobject_cast<TokenPool*>(token->parent() ) ) {
        debug() << "Copying token" << token->name();
        token = m_tokenFactory->createToken( token->name(),
                                             token->iconName(),
                                             token->value() );
    }

    box->insertWidget( col, token );
    token->show();

    connect( token, &Token::changed, this, &TokenDropTarget::changed );
    connect( token, &Token::gotFocus, this, &TokenDropTarget::tokenSelected );
    connect( token, &Token::changed, this, &TokenDropTarget::deleteEmptyRows );

    Q_EMIT changed();
}


Token*
TokenDropTarget::tokenAt( const QPoint &pos ) const
{
    for( uint row = 0; row < rows(); ++row )
        if( QBoxLayout *rowBox = qobject_cast<QBoxLayout*>( layout()->itemAt( row )->layout() ) )
            for( int col = 0; col < rowBox->count(); ++col )
                if( QWidget *kid = rowBox->itemAt( col )->widget() )
                {
                    if( kid->geometry().contains( pos ) )
                        return qobject_cast<Token*>(kid);
                }
    return 0;
}

void
TokenDropTarget::drop( Token *token, const QPoint &pos )
{
    DEBUG_BLOCK;

    if ( !token )
        return;

    // find the token at the position.
    QWidget *child = childAt( pos );
    Token *targetToken = qobject_cast<Token*>(child); // tokenAt( pos );
    if( !targetToken && child && child->parent() ) // sometimes we get the label of the token.
        targetToken = qobject_cast<Token*>( child->parent() );

    // unlayout in case of move
    if( QBoxLayout *box = rowBox( token ) )
    {
        box->removeWidget( token );
        deleteEmptyRows(); // a row could now be empty due to a move
    }

    if( targetToken )
    {   // we hit a sibling, -> prepend
        QPoint idx;
        rowBox( targetToken, &idx );

        if( rowLimit() != 1 && rowLimit() < m_rows && idx.y() == (int)m_rows - 1 &&
            pos.y() > targetToken->geometry().y() + ( targetToken->height() * 2 / 3 ) )
            insertToken( token, idx.y() + 1, idx.x());
        else if( pos.x() > targetToken->geometry().x() + targetToken->width() / 2 )
            insertToken( token, idx.y(), idx.x() + 1);
        else
            insertToken( token, idx.y(), idx.x() );
    }
    else
    {
        appendToken( token );
    }

    token->setFocus( Qt::OtherFocusReason ); // select the new token right away
}

void
TokenDropTarget::dragEnterEvent( QDragEnterEvent *event )
{
    if( event->mimeData()->hasFormat( Token::mimeType() ) )
        event->acceptProposedAction();
}

void
TokenDropTarget::dropEvent( QDropEvent *event )
{
    if( event->mimeData()->hasFormat( Token::mimeType() ) )
    {
        event->acceptProposedAction();

        Token *token = qobject_cast<Token*>( event->source() );

        if ( !token ) // decode the stream created in TokenPool::dropEvent
            token = m_tokenFactory->createTokenFromMime( event->mimeData(), this );

    // - copy the token if it belongs to a token pool (fix BR 296136)
    if( qobject_cast<TokenPool*>(token->parent() ) ) {
        token = m_tokenFactory->createToken( token->name(),
                                             token->iconName(),
                                             token->value() );
    }

        if( token )
            drop( token, event->pos() );
    }
}

void
TokenDropTarget::paintEvent(QPaintEvent *pe)
{
    QWidget::paintEvent(pe);
    if (count())
        return;
    QPainter p(this);
    QColor c = palette().color(foregroundRole());
    c.setAlpha(c.alpha()*64/255);
    p.setPen(c);
    p.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap, i18n("Drag in and out items from above."));
    p.end();
}

int
TokenDropTarget::row( Token *token ) const
{
    for( uint row = 0; row <= rows(); ++row )
    {
        QBoxLayout *box = qobject_cast<QBoxLayout*>( layout()->itemAt( row )->layout() );
        if ( box && ( box->indexOf( token ) ) > -1 )
            return row;
    }
    return -1;
}

QBoxLayout *
TokenDropTarget::rowBox( QWidget *w, QPoint *idx ) const
{
    QBoxLayout *box = 0;
    int col;
    for( uint row = 0; row < rows(); ++row )
    {
        box = qobject_cast<QBoxLayout*>( layout()->itemAt( row )->layout() );
        if ( box && ( col = box->indexOf( w ) ) > -1 )
        {
            if ( idx )
            {
                idx->setX( col );
                idx->setY( row );
            }
            return box;
        }
    }
    return NULL;
}

QBoxLayout *
TokenDropTarget::rowBox( const QPoint &pt ) const
{
    QBoxLayout *box = 0;
    for( uint row = 0; row < rows(); ++row )
    {
        box = qobject_cast<QBoxLayout*>( layout()->itemAt( row )->layout() );
        if ( !box )
            continue;
        for ( int col = 0; col < box->count(); ++col )
        {
            if ( QWidget *w = box->itemAt( col )->widget() )
            {
                const QRect &geo = w->geometry();
                if ( geo.y() <= pt.y() && geo.bottom() >= pt.y() )
                    return box;
                break; // yes - all widgets are supposed of equal height. we checked on, we checked all
            }
        }
    }
    return NULL;
}

void
TokenDropTarget::setCustomTokenFactory( TokenFactory * factory )
{
    delete m_tokenFactory;
    m_tokenFactory = factory;
}

void
TokenDropTarget::setVerticalStretch( bool value )
{
    if( value == m_verticalStretch )
        return;

    m_verticalStretch = value;

    if( m_verticalStretch )
        qobject_cast<QBoxLayout*>( layout() )->addStretch( 1 );
    else
        delete layout()->takeAt( layout()->count() - 1 );
}


