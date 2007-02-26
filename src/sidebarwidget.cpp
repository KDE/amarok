/*
  Copyright (c) 2006 Gábor Lehel <illissius@gmail.com>

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

#include <QAbstractButton>
#include <QAbstractItemDelegate>
#include <QAction>
#include <QApplication>
#include <QList>
#include <QPainter>
#include <QWheelEvent>
#include <QtDebug>

#include "sidebarwidget.h"

class SideBarButton: public QAbstractButton
{
    typedef QAbstractButton super;
    public:
        SideBarButton( const QIcon &icon, const QString &text, QWidget *parent )
            : super( parent )
        {
            setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );
            setCheckable( true );
            setIcon( icon );
            setText( text );
        }

        virtual QSize sizeHint() const;

    protected:
        virtual void paintEvent( QPaintEvent *e );
        virtual void enterEvent( QEvent* ) { update(); }
        virtual void leaveEvent( QEvent* ) { update(); }

    private:
        int widthHint() const;
        int heightHint() const;
};


struct SideBarWidget::Private
{
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
    delete d;
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

QSize SideBarButton::sizeHint() const
{
    return QSize( widthHint(), heightHint() ).expandedTo( QApplication::globalStrut() );
}

int SideBarButton::widthHint() const
{
    int width;
    if( !icon().isNull() )
        width = iconSize().width();
    if( !width && text().isEmpty() )
        width = fontMetrics().size( Qt::TextShowMnemonic, "TEXT" ).height();
    else
        width = qMax( width, fontMetrics().size( Qt::TextShowMnemonic, text() ).height() );
    return width + 6;
}

int SideBarButton::heightHint() const
{
    int height = 0;
    if( !icon().isNull() )
        height = iconSize().height();
    if( !height && text().isEmpty() )
        height = fontMetrics().size( Qt::TextShowMnemonic, "TEXT" ).width();
    else
        height += fontMetrics().size( Qt::TextShowMnemonic, text() ).width();
    return height + 12;
}

void SideBarButton::paintEvent( QPaintEvent* )
{
    QPainter p( this );
    p.initFrom( this );

    QColor c;
    if( isDown() )
        c = palette().highlight().color().dark( 150 );
    else if( isChecked() && underMouse() )
        c = palette().highlight().color().light( 110 );
    else if( isChecked() )
        c = palette().highlight().color();
    else if( underMouse() )
        c = palette().highlight().color().light();
    else
        c = palette().window().color();
    p.setBrush( c );
    p.drawRect( rect() );

    const QString txt = text().replace( "&", "" );

    const int pos = qMin( height(), height() / 2 + heightHint() / 2 ) - iconSize().height();

    p.translate( 0, pos );
    p.drawPixmap( width() / 2 - iconSize().width() / 2, 0, icon().pixmap( iconSize() ) );
    p.translate( fontMetrics().size( Qt::TextShowMnemonic, txt ).height(), 0 );
    p.rotate( -90 );
    p.drawText( 10, 0, QAbstractItemDelegate::elidedText( fontMetrics(), pos - 10, Qt::ElideRight, txt ) );
}

#include "sidebarwidget.moc"
