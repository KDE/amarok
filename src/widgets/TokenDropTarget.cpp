/****************************************************************************************
 * Copyright (c) 2009 Thomas Lbking <thomas.luebking@web.de>                            *
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

#include <KLocale>

#include <QDropEvent>
#include <QPainter>
#include <QVBoxLayout>


/** TokenDragger - eventfilter that drags a token, designed to be a child of TokenDropTarget
This is necessary, as if TokenDropTarget would QDrag::exec() itself, the eventFilter would be blocked
and thus not be able to handle other events for the parenting widget, like e.g. dragEnter... */

class TokenDragger : public QObject
{
public:
    TokenDragger( const QString &mimeType, TokenDropTarget *parent ) : QObject(parent), m_mimeType( mimeType )
    {}
protected:
    bool eventFilter( QObject *o, QEvent *e )
    {
        if ( e->type() == QEvent::MouseMove )
        {
            if ( static_cast<QMouseEvent*>(e)->buttons() & Qt::LeftButton )
                return drag( qobject_cast<Token*>(o) );
        }
        else if ( e->type() == QEvent::MouseButtonPress )
        {
            if ( static_cast<QMouseEvent*>(e)->buttons() & Qt::LeftButton )
            {
                setCursor( qobject_cast<QWidget*>(o), Qt::ClosedHandCursor );
                return true; // don't propagate to parents
            }
            return false;
        }
        else if ( e->type() == QEvent::MouseButtonRelease )
        {
            if ( !( static_cast<QMouseEvent*>(e)->buttons() & Qt::LeftButton ) )
            {
                setCursor( qobject_cast<QWidget*>(o), Qt::OpenHandCursor );
                emit static_cast<TokenDropTarget*>( parent() )->focusReceived( qobject_cast<QWidget*>(o) );
                return true; // don't propagate to parents
            }
            return false;
        }
        else if ( e->type() == QEvent::FocusIn )
            emit static_cast<TokenDropTarget*>( parent() )->focusReceived( qobject_cast<QWidget*>(o) );
        else if ( e->type() == QEvent::Hide )
        {
            setCursor( qobject_cast<QWidget*>(o), Qt::OpenHandCursor );
            return false;
        }
        return false;
    }

private:
    bool drag( Token *token )
    {
        if ( !token )
            return false;

        bool ret = false;
        bool stacked = token->parentWidget() && qobject_cast<TokenDropTarget*>( token->parentWidget() ); // true if token originated from a TokenDropTarget.
        if (stacked)
            token->hide();

        QPixmap pixmap( token->size() );
        token->render( &pixmap );
        QDrag *drag = new QDrag( token );
        QMimeData *data = new QMimeData;

        QByteArray itemData;
        QDataStream dataStream( &itemData, QIODevice::WriteOnly );
//         dataStream << child->name() << child->iconName() << child->value();

        data->setData( m_mimeType, itemData );
        drag->setMimeData( data );
        drag->setPixmap( pixmap );
        drag->setHotSpot ( pixmap.rect().center() );

        Qt::DropAction dropAction = drag->exec( Qt::CopyAction | Qt::MoveAction, Qt::CopyAction );

        if ( stacked )
        {
            if( dropAction != Qt::MoveAction && dropAction != Qt::CopyAction ) // dragged out and not just dragged to another position.
            {
                // TODO: nice poof animation? ;-)
                delete token;
                ret = true; // THIS IS IMPORTANT
            }
            // anyway, tell daddy to wipe empty rows NOW
            static_cast<TokenDropTarget*>(parent())->deleteEmptyRows();
        }
        return ret;
    }
    inline void setCursor( QWidget *w, Qt::CursorShape shape )
    {
        if ( !w )
            return;
        w->setCursor( shape );
    }
private:
    QString m_mimeType;
    QPoint m_startPos;
};


TokenDropTarget::TokenDropTarget( const QString &mimeType, QWidget *parent )
    : QWidget( parent )
    , m_tokenDragger( new TokenDragger( mimeType, this ) )
    , m_tokenFactory( new TokenFactory() )
    , m_rows( 0 )
    , m_horizontalStretch( false ) // DANGER: m_horizontalStretch is used as int in the following code, assuming that true == 1
{
    m_mimeType = mimeType;
    m_limits[0] = m_limits[1] = 0;
    // let daddy widget be droppable... ;)
    parent->setAcceptDrops(true);
    // ...and handle drop events for him
    parent->removeEventFilter( this );
    parent->installEventFilter( this );

    new QVBoxLayout( this );
    // visual, maybe there should be spacing? however, frames etc. can have contentmargin.
    layout()->setSpacing( 0 );
    layout()->setContentsMargins( 1, 1, 1, 1 );

    setMinimumSize( 60, 16 ); // sloppy. The minimum size depends on the size of the "Drag here" text.
}

bool
TokenDropTarget::accept( QDropEvent *de )
{
    if ( !de->mimeData()->hasFormat( m_mimeType ) )
    {
        de->ignore();
        return false;
    }

    if ( de->source() && parentWidget() && de->source()->parentWidget() == parentWidget() )
    {   // move
        de->setDropAction(Qt::MoveAction);
        de->accept();
    }
    else
        de->acceptProposedAction();
    return true;
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
    QLayoutItem *row, *col;
    while( m_rows )
    {
        row = layout()->takeAt( 0 );
        if ( QLayout *layout = row->layout() )
        {
            while( ( col = layout->takeAt( 0 ) ) )
            {
                delete col->widget();
                delete col;
            }
        }
        delete row;
        m_rows--;
    }
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
TokenDropTarget::deleteEmptyRows()
{
    for( int row = rows() - 1; row >= 0; --row )
    {
        QBoxLayout *box = qobject_cast<QBoxLayout*>( layout()->itemAt( row )->layout() );
        if( box && box->count() < ( 1 + m_horizontalStretch ) ) // sic! last is spacer
        {
            delete layout()->takeAt( row );
            m_rows--;
        }
    }

    update(); // this removes empty layouts somehow for deleted tokens
    emit changed();
}

QList< Token *>
TokenDropTarget::drags( int row )
{
    int lower = 0, upper = rows();
    if ( row > -1 && row < rows() )
    {
        lower = row;
        upper = row + 1;
    }

    QList< Token *> list;
    Token *token;
    for ( row = lower; row < upper; ++row )
        if ( QHBoxLayout *rowBox = qobject_cast<QHBoxLayout*>( layout()->itemAt( row )->layout() ) )
        {
            for( int col = 0; col < rowBox->count() - m_horizontalStretch; ++col )
                if ( ( token = qobject_cast<Token*>( rowBox->itemAt( col )->widget() ) ) )
                    list << token;
        }

    return list;
}

void
TokenDropTarget::drop( Token *token, const QPoint &pos )
{
    if ( !token )
        return;

    // unlayout in case of move
    if ( QBoxLayout *box = rowBox( token ) )
        box->removeWidget( token );
    token->setParent( parentWidget() );

    // find the token at the position.
    QWidget *child = childAt( pos );
    Token *sibling = qobject_cast<Token*>( child );
    if( !sibling && child && child->parent() ) // sometimes we get the label of the token.
        sibling = qobject_cast<Token*>( child->parent() );

    QBoxLayout *box = 0;
    if( sibling )
    {   // we hit a sibling, -> prepend
        QPoint idx;
        box = rowBox( sibling, &idx );
        if( pos.x() > sibling->geometry().x() + 2 * sibling->width() / 3 )
            box->insertWidget( idx.x() + 1, token );
        else
            box->insertWidget( idx.x(), token );
    }
    else
    {
        if ( rowLimit() && rows() >= (int)rowLimit() ) // we usually don't want more rows
            box = qobject_cast<QBoxLayout*>( layout()->itemAt( rows() - 1 )->layout() );

        if ( !box )
        {
            box = rowBox( pos ); // maybe this is on an existing row
            if ( !box )
                box = appendRow();
        }
        int col = ( box->count() > m_horizontalStretch && box->itemAt(0)->widget() &&
                    pos.x() < box->itemAt(0)->widget()->geometry().x() ) ? 0 : box->count() - m_horizontalStretch;
        box->insertWidget( col, token ); // append to existing row
    }
    token->show();
    // update(); // count changed
    emit changed();

    token->setFocus( Qt::OtherFocusReason ); // select the new token right away
}

bool
TokenDropTarget::eventFilter( QObject *o, QEvent *ev )
{
    Q_UNUSED( o )

    if ( ev->type() == QEvent::DragMove ||
         ev->type() == QEvent::DragEnter )
    {
        accept( static_cast<QDropEvent*>( ev ) );
        return false; // TODO: return accept boolean ?
    }

//     if ( ev->type() == QEvent::DragLeave )
    if ( ev->type() == QEvent::Drop )
    {
        QDropEvent *de = static_cast<QDropEvent*>( ev );
        if ( accept( de ) )
        {
            Token *token = qobject_cast<Token*>( de->source() );
            if ( !token )
            {
                // decode the stream created in TokenPool::dropEvent
                QByteArray itemData = de->mimeData()->data( m_mimeType );
                QDataStream dataStream(&itemData, QIODevice::ReadOnly);

                QString tokenName;
                QString tokenIconName;
                qint64 tokenValue;
                QColor tokenTextColor;
                dataStream >> tokenName;
                dataStream >> tokenIconName;
                dataStream >> tokenValue;
                dataStream >> tokenTextColor;

                token = m_tokenFactory->createToken( tokenName, tokenIconName, tokenValue, this );
                token->setTextColor( tokenTextColor );
                token->removeEventFilter( m_tokenDragger );
                token->installEventFilter( m_tokenDragger );
                token->setCursor( Qt::OpenHandCursor );
            }
            drop( token, de->pos() );
        }
        return false;
    }
    return false;
}

void
TokenDropTarget::insertToken( Token *token, int row, int col )
{
    // - validate row
    if ( row < 0 && rowLimit() && rows() >= (int)rowLimit() )
        row = rowLimit() - 1; // want to append, but we can't so use the last row instead

    QBoxLayout *box;
    if( row < 0 || row >= rows() )
        box = appendRow();
    else
        box = qobject_cast<QBoxLayout*>( layout()->itemAt( row )->layout() );

    // - validate col
    if( col < 0 || col > box->count() - ( 1 + m_horizontalStretch ) )
        col = box->count() - m_horizontalStretch;

    token->setParent( parentWidget() );
    box->insertWidget( col, token );
    token->removeEventFilter( m_tokenDragger );
    token->installEventFilter( m_tokenDragger );
    token->setCursor( Qt::OpenHandCursor );
    emit changed();
    // update(); // count changed
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
    for ( int row = 0; row <= rows(); ++row )
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
    for ( int row = 0; row < rows(); ++row )
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
    for ( int row = 0; row < rows(); ++row )
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

