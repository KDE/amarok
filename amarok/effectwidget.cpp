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

EffectListItem::EffectListItem( QListView *parent, const QString &label ) :
        QListViewItem( parent, label )
{
/*    m_pFX = new Arts::StereoEffect;
    *m_pFX = Arts::DynamicCast( pApp->m_Server.createObject( std::string( label.ascii() ) ) );
    m_pFX->start();

    m_ID = pApp->m_effectStack.insertBottom( *m_pFX, std::string( label.ascii() ) );

    if ( !m_ID )
    {
        kdDebug() << "insertBottom failed" << endl;
        m_pFX->stop();
        return;
    }*/
}


EffectListItem::~EffectListItem()
{
/*    m_pFX->stop();
    pApp->m_effectStack.remove( m_ID );

    delete m_pFX;*/
}


void EffectListItem::configure()
{
/*    ArtsConfigWidget *pWidget = new ArtsConfigWidget( *m_pFX, pApp->m_pPlayerWidget );
    pWidget->show();*/
}


// CLASS EffectConfigWidget --------------------------------------------------------

// ArtsConfigWidget::ArtsConfigWidget( Arts::Object object, QWidget *parent )
//         : QWidget( parent, 0, Qt::WType_TopLevel | Qt::WDestructiveClose )
// {
//     setCaption( kapp->makeStdCaption( QString( object._interfaceName().c_str() ) ) );
// 
//     Arts::GenericGuiFactory factory;
//     m_gui = factory.createGui( object );
// 
//     if ( m_gui.isNull() )
//     {
//         kdDebug() << "Arts::Widget gui == NULL! Returning.." << endl;
//         return;
//     }
// 
//     else
//     {
//         m_pArtsWidget = new KArtsWidget( m_gui, this );
// 
//         QBoxLayout *lay = new QHBoxLayout( this );
//         lay->add( m_pArtsWidget );
//     }
// }
// 
// 
// ArtsConfigWidget::~ArtsConfigWidget()
// {
//     delete m_pArtsWidget;
//     m_gui = Arts::Widget::null();
// }


// CLASS EffectWidget -----------------------------------------------------------

#define BUTTON_WIDTH 30

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
    connect( m_pListView, SIGNAL( clicked( QListViewItem * ) ),
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

    resize( 300, 400 );
}


EffectWidget::~EffectWidget()
{}


// SLOTS ----------------------------------------------------------------------------

void EffectWidget::slotButtonTop()
{
    new EffectListItem( m_pListView, m_pComboBox->currentText() );
}


void EffectWidget::slotButtonBotConf()
{
    EffectListItem *pItem = static_cast<EffectListItem*>( m_pListView->currentItem() );
    pItem->configure();
}


void EffectWidget::slotButtonBotRem()
{
    delete m_pListView->currentItem();
    m_pButtonBotConf->setEnabled( false );
}


void EffectWidget::slotItemClicked( QListViewItem *pCurrentItem )
{
    if ( pCurrentItem )
    {
        EffectListItem *pEffect = static_cast<EffectListItem *>( pCurrentItem );
        m_pButtonBotConf->setEnabled( pApp->m_pEngine->effectConfigurable( pEffect->text( 0 ) ) );
    }

    else
    {
        m_pButtonBotConf->setEnabled( false );
    }
}

#include "effectwidget.moc"
