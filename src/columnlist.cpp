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

#include <config.h>

#include <qheader.h>
#include <qpainter.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <klistview.h>
#include <kpushbutton.h>
#include "amarokconfig.h"
#include "metabundle.h"
#include "playlist.h"

#include "columnlist.h"

class MyCheckListItem: public QCheckListItem
{
    typedef QCheckListItem super;
    ColumnList *m_list;
public:
    int column;
    MyCheckListItem( int c, QListView *v, const QString &s, Type t, ColumnList *list ):
        super( v, s, t ), m_list( list ), column( c ) { }
    virtual void paintCell( QPainter * p, const QColorGroup &cg, int c, int w, int a )
    {
        QFont f = p->font();
        if( isOn() )
            f.setBold( !f.bold() );
        p->setFont( f );
        super::paintCell( p, cg, c, w, a );
    }
    virtual void stateChange( bool b )
    {
        super::stateChange( b );
        m_list->setChanged();
    }
    MyCheckListItem *itemAbove() { return static_cast<MyCheckListItem*>( QCheckListItem::itemAbove() ); }
    MyCheckListItem *itemBelow() { return static_cast<MyCheckListItem*>( QCheckListItem::itemBelow() ); }
};

ColumnList::ColumnList( QWidget *parent, const char *name )
    : QHBox( parent, name ), m_changed( true )
{
    setSpacing( 5 );

    QVBox *buttonbox = new QVBox( this );

    m_up = new KPushButton( KGuiItem( QString::null, "up" ), buttonbox );
    QToolTip::add( m_up, i18n( "Move column up" ) );
    connect( m_up, SIGNAL( clicked() ), this, SLOT( moveUp() ) );

    m_down = new KPushButton( KGuiItem( QString::null, "down" ), buttonbox );
    QToolTip::add( m_down, i18n( "Move column down" ) );
    connect( m_down, SIGNAL( clicked() ), this, SLOT( moveDown() ) );

    m_list = new KListView( this );
    m_list->addColumn("");
    m_list->header()->hide();
    m_list->setSelectionMode( QListView::Single );
    m_list->setResizeMode( QListView::LastColumn );
    m_list->setSorting( -1 );
    m_list->setAcceptDrops( true );
    m_list->setDragEnabled( true );
    m_list->setDropVisualizer( true );
    m_list->setDropVisualizerWidth( 3 );
    connect( m_list, SIGNAL( moved() ), this, SLOT( updateUI() ) );
    connect( m_list, SIGNAL( moved() ), this, SLOT( setChanged() ) );
    connect( m_list, SIGNAL( currentChanged( QListViewItem* ) ), this, SLOT( updateUI() ) );

    QHeader* const h = Playlist::instance()->header();
    for( int i = h->count() - 1; i >= 0; --i )
    {
        const int s = h->mapToSection( i );
        if( ( s != MetaBundle::Rating || AmarokConfig::useRatings() ) &&
            ( s != MetaBundle::Mood   || AmarokConfig::showMoodbar() ) &&
            ( s != MetaBundle::Score  || AmarokConfig::useScores() ) )
        {
            ( new MyCheckListItem( s, m_list, MetaBundle::prettyColumnName( s ), QCheckListItem::CheckBox, this ) )
                ->setOn( h->sectionSize( s ) );
        }
    }

    m_list->setCurrentItem( m_list->firstChild() );
    updateUI();
    resetChanged();
}

QValueList<int> ColumnList::visibleColumns() const
{
    QValueList<int> v;
    for( MyCheckListItem *item = static_cast<MyCheckListItem*>( m_list->firstChild() ); item; item = item->itemBelow() )
        if( item->isOn() )
            v.append( item->column );
    return v;
}

QValueList<int> ColumnList::columnOrder() const
{
    QValueList<int> v;
    for( MyCheckListItem *item = static_cast<MyCheckListItem*>( m_list->firstChild() ); item; item = item->itemBelow() )
        v.append( item->column );
    return v;
}

bool ColumnList::isChanged() const
{
    return m_changed;
}

void ColumnList::resetChanged()
{
    m_changed = false;
}

void ColumnList::moveUp()
{
    if( QListViewItem *item = m_list->currentItem() )
        if( item->itemAbove() )
        {
            m_list->moveItem( item, 0, item->itemAbove()->itemAbove() );
            m_list->setCurrentItem( item );
            m_list->ensureItemVisible( item );
            updateUI();
            setChanged();
        }
}

void ColumnList::moveDown()
{
    if( QListViewItem *item = m_list->currentItem() )
    {
        m_list->moveItem( item, 0, item->itemBelow() );
        m_list->setCurrentItem( item );
        m_list->ensureItemVisible( item );
        updateUI();
        setChanged();
    }
}

void ColumnList::updateUI()
{
    m_up->setEnabled( m_list->currentItem() && m_list->currentItem()->itemAbove() );
    m_down->setEnabled( m_list->currentItem() && m_list->currentItem()->itemBelow() );
}

void ColumnList::setChanged() //slot
{
    if( !m_changed )
    {
        m_changed = true;
        emit changed();
    }
}

ColumnsDialog::ColumnsDialog()
    : KDialogBase( PlaylistWindow::self(), 0, false, i18n( "Playlist Columns" ) ),
      m_list( new ColumnList( this ) )
{
    setMainWidget( m_list );
    enableButtonApply( false );
    connect( m_list, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
}

ColumnsDialog::~ColumnsDialog()
{
    s_instance = 0;
}

void ColumnsDialog::slotApply()
{
    apply();
    KDialogBase::slotApply();
}

void ColumnsDialog::slotOk()
{
    apply();
    KDialogBase::slotOk();
}

void ColumnsDialog::hide()
{
    KDialogBase::hide();
    delete this;
}

void ColumnsDialog::apply()
{
    Playlist::instance()->setColumns( m_list->columnOrder(), m_list->visibleColumns() );
    m_list->resetChanged();
    enableButtonApply( false );
}

void ColumnsDialog::display() //static
{
    if( !s_instance )
        s_instance = new ColumnsDialog;
    s_instance->show();
}

void ColumnsDialog::slotChanged() //slot
{
    enableButtonApply( true );
}

ColumnsDialog *ColumnsDialog::s_instance = 0;

#include "columnlist.moc"
