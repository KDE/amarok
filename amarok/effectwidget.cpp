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
#include "playerapp.h"
#include "playerwidget.h"

#include <string>
#include <vector>

#include <qcstring.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qheader.h>
#include <qiconset.h>
#include <qlayout.h>
#include <qlistview.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qpushbutton.h>
#include <qsizepolicy.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qvbox.h>

#include <kcombobox.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstandarddirs.h>


// CLASS EffectListItem --------------------------------------------------------

EffectListItem::EffectListItem( QListView *parent, const QString &label )
        : QListViewItem( parent, label )
        , m_id( pApp->m_pEngine->createEffect( label ) )
        
{}


EffectListItem::EffectListItem( QListView *parent, const QString &label, long id )
        : QListViewItem( parent, label )
        , m_id( id )
{}


EffectListItem::~EffectListItem()
{}


void EffectListItem::configure()
{
    pApp->m_pEngine->configureEffect( m_id );
}


// CLASS EffectWidget --------------------------------------------------------

static const int BUTTON_WIDTH = 30;


EffectWidget::EffectWidget()
        : KDialogBase( 0, "EffectWidget", false, kapp->makeStdCaption( i18n("Effects") ) )
{
    showButtonApply( false );
    showButtonCancel( false );
    setButtonText( Ok, i18n("Close") );

    QVBox *pFrame = makeVBoxMainWidget();
    pFrame->layout()->setResizeMode( QLayout::FreeResize );
    KIconLoader iconLoader;

    m_pGroupBoxTop = new QGroupBox( 2, Qt::Horizontal, i18n("Available Effects"), pFrame );
    m_pGroupBoxBot = new QGroupBox( 2, Qt::Horizontal, i18n("Active Effects"), pFrame );

    pFrame->setStretchFactor( m_pGroupBoxTop, 1 );
    pFrame->setStretchFactor( m_pGroupBoxBot, 10 );

    m_pComboBox = new KComboBox( m_pGroupBoxTop );
    m_pComboBox->insertStringList( pApp->m_pEngine->availableEffects() );

    m_pButtonTopDown = new QPushButton( iconLoader.loadIconSet( "down", KIcon::Toolbar, KIcon::SizeSmall ),
                                        0, m_pGroupBoxTop );
    m_pButtonTopDown->setMaximumWidth ( BUTTON_WIDTH );
    QToolTip::add( m_pButtonTopDown, i18n("Add") );
    connect( m_pButtonTopDown, SIGNAL( clicked() ), this, SLOT( slotButtonTop() ) );

    m_pListView = new QListView( m_pGroupBoxBot );
    m_pListView->header()->hide();
    m_pListView->addColumn( "void" );
    m_pListView->setSorting( -1 );
    connect( m_pListView, SIGNAL( currentChanged( QListViewItem * ) ),
             this, SLOT( slotItemClicked( QListViewItem * ) ) );

    QFrame *pContainerBotButtons = new QFrame( m_pGroupBoxBot );

    m_pButtonBotConf = new QPushButton( iconLoader.loadIconSet( "configure", KIcon::Toolbar, KIcon::SizeSmall ),
                                        0, pContainerBotButtons );
    m_pButtonBotConf->setMaximumWidth ( BUTTON_WIDTH );
    m_pButtonBotConf->setEnabled      ( false );
    QToolTip::add( m_pButtonBotConf, i18n( "Configure" ) );
    connect( m_pButtonBotConf, SIGNAL( clicked() ), this, SLOT( slotButtonBotConf() ) );

    m_pButtonBotRem = new QPushButton( iconLoader.loadIconSet( "remove", KIcon::Toolbar, KIcon::SizeSmall ),
                                       0, pContainerBotButtons );
    m_pButtonBotRem->setMaximumWidth ( BUTTON_WIDTH );
    QToolTip::add( m_pButtonBotRem, i18n("Remove") );
    connect( m_pButtonBotRem, SIGNAL( clicked() ), this, SLOT( slotButtonBotRem() ) );

    QBoxLayout *pLayoutBotButtons = new QVBoxLayout( pContainerBotButtons );
    pLayoutBotButtons->setResizeMode( QLayout::FreeResize );
    pLayoutBotButtons->addWidget    ( m_pButtonBotConf );
    pLayoutBotButtons->addWidget    ( m_pButtonBotRem );
    pLayoutBotButtons->addItem      ( new QSpacerItem( 0, 10 ) );

    { //fill listview with restored effect entries
        std::vector<long> vec = pApp->m_pEngine->activeEffects();
        for ( int i = 0; i < vec.size(); i++ )
                new EffectListItem( m_pListView, pApp->m_pEngine->effectNameForId( vec[i] ), vec[i] );
    }
                
    resize( 300, 400 );
}


EffectWidget::~EffectWidget()
{}


// SLOTS ----------------------------------------------------------------------------

void EffectWidget::slotButtonTop()
{
    new EffectListItem( m_pListView, m_pComboBox->currentText() );
    slotItemClicked( m_pListView->currentItem() );
}


void EffectWidget::slotButtonBotConf()
{
    EffectListItem *pItem = static_cast<EffectListItem*>( m_pListView->currentItem() );
    pItem->configure();
    
    m_pButtonBotConf->setEnabled( false );
}


void EffectWidget::slotButtonBotRem()
{
    pApp->m_pEngine->removeEffect( static_cast<EffectListItem*>( m_pListView->currentItem() )->m_id );
    delete m_pListView->currentItem();
    
    m_pButtonBotConf->setEnabled( false );
}


void EffectWidget::slotItemClicked( QListViewItem *pCurrentItem )
{
    if ( pCurrentItem )
    {
        EffectListItem *pEffect = static_cast<EffectListItem *>( pCurrentItem );
        m_pButtonBotConf->setEnabled( pApp->m_pEngine->effectConfigurable( pEffect->m_id ) );
    }

    else
    {
        m_pButtonBotConf->setEnabled( false );
    }
}

#include "effectwidget.moc"
