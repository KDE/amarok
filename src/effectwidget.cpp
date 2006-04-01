/***************************************************************************
                          effectwidget.cpp  -  description
                             -------------------
    begin                : Mar 6 2003
    copyright            : (C) 2003 by Mark Kretschmann
    email                : markey@web.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "effectwidget.h"
#include "engine/enginebase.h"
#include "enginecontroller.h"

#include <vector>

#include <qgroupbox.h>
#include <qheader.h>
#include <qlayout.h>
#include <qsizepolicy.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qvbox.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <klocale.h>
#include <kpushbutton.h>


EffectWidget* EffectWidget::self = 0;
QRect         EffectWidget::saveGeometry;


EffectWidget::EffectWidget( QWidget* parent )
        : KDialogBase( parent, "EffectWidget", false, i18n("Effects"), User1, User1, false, KStdGuiItem::close() )
{
    if( saveGeometry.isValid() )
    {
        setGeometry( saveGeometry );
    }
    else resize( 300, 400 );

    Engine::Effects &effects = EngineController::engine()->effects();
    setWFlags( Qt::WDestructiveClose );

    QVBox *pFrame = makeVBoxMainWidget();
    QWidget *w, *box1, *box2;
    QVBox   *box3;
    QSizePolicy policy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    box1 = new QGroupBox( 2, Qt::Horizontal, i18n("Available Effects"), pFrame );
    box2 = new QGroupBox( 2, Qt::Horizontal, i18n("Active Effects"), pFrame );

    pFrame->setStretchFactor( box1, 1 );
    pFrame->setStretchFactor( box2, 10 );

    m_pComboBox = new KComboBox( box1 );
    m_pComboBox->insertStringList( effects.availableEffects() );

    w = new KPushButton( KGuiItem( "", "down" ), box1 );
    w->setSizePolicy( policy );
    QToolTip::add( w, i18n("Add") );
    connect( w, SIGNAL( clicked() ), SLOT( slotAdd() ) );


    m_pListView = new KListView( box2 );
    m_pListView->header()->hide();
    m_pListView->addColumn( "void" );
    m_pListView->setResizeMode( QListView::LastColumn );
    connect( m_pListView, SIGNAL(selectionChanged( QListViewItem* )), SLOT(slotChanged( QListViewItem* )) );

    box3 = new QVBox( box2 );

    w = new KPushButton( KGuiItem( QString::null, "configure" ), box3 );
    w->setSizePolicy( policy );
    w->setEnabled( false );
    QToolTip::add( w, i18n( "Configure" ) );
    connect( w, SIGNAL( clicked() ), SLOT( slotConfigure() ) );

    m_pConfigureButton = w;

    w = new KPushButton( KGuiItem( QString::null, "editdelete" ), box3 );
    w->setSizePolicy( policy );
    QToolTip::add( w, i18n("Remove") );
    connect( w, SIGNAL( clicked() ), SLOT( slotRemove() ) );

    box3->layout()->addItem( new QSpacerItem( 0, 10 ) );

    { //fill listview with restored effect entries
        std::vector<long> vec = effects.activeEffects();
        for ( uint i = 0; i < vec.size(); i++ )
                new EffectListItem( m_pListView, effects.effectNameForId( vec[i] ), vec[i] );
    }

    connect( this, SIGNAL( user1Clicked() ), this, SLOT( accept() ) );

    show();
}


EffectWidget::~EffectWidget()
{
    self = 0;
    saveGeometry = geometry();
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
////////////////////////////////////////////////////////////////////////////////

void EffectWidget::slotAdd()
{
    m_pListView->setCurrentItem( new EffectListItem( m_pListView, m_pComboBox->currentText() ) );
}


void EffectWidget::slotConfigure()
{
    EffectListItem *item = static_cast<EffectListItem*>( m_pListView->currentItem() );
    item->configure();
}


void EffectWidget::slotRemove()
{
    if ( EffectListItem *item = static_cast<EffectListItem*>( m_pListView->currentItem() ) )
    {
        EngineController::engine()->effects().removeEffect( item->m_id );
        delete item;
    }
}


void EffectWidget::slotChanged( QListViewItem *selectedItem ) //SLOT
{
    const EffectListItem* const item = static_cast<EffectListItem*>( selectedItem );
    const bool enabled = item && EngineController::engine()->effects().effectConfigurable( item->m_id );

    m_pConfigureButton->setEnabled( enabled );
}



////////////////////////////////////////////////////////////////////////////////
// CLASS EffectListItem
////////////////////////////////////////////////////////////////////////////////

EffectListItem::EffectListItem( KListView *parent, const QString &label, long id )
        : KListViewItem( parent, label )
        , m_id( id != -1 ? id : EngineController::engine()->effects().createEffect( label ) )
{}


void EffectListItem::configure()
{
    EngineController::engine()->effects().configureEffect( m_id );
}


#include "effectwidget.moc"
