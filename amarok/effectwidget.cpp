/***************************************************************************
                          effectwidget.cpp  -  description
                             -------------------
    begin                : Don Mär 6 2003
    copyright            : (C) 2003 by Mark Kretschmann
    email                :
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
#include "playerapp.h"
#include "playerwidget.h"

#include <vector>
#include <string>

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
#include <qstrlist.h>
#include <qtooltip.h>
#include <qvbox.h>

#include <kcombobox.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <arts/artsgui.h>
#include <arts/connect.h>
#include <arts/dynamicrequest.h>
#include <arts/flowsystem.h>
#include <arts/kartsdispatcher.h>
#include <arts/kartswidget.h>
#include <arts/kmedia2.h>
#include <arts/kplayobjectfactory.h>
#include <arts/soundserver.h>

// CLASS EffectListItem --------------------------------------------------------

EffectListItem::EffectListItem( QListView *parent, const QString &label ) :
QListViewItem( parent, label )
{
    m_pFX = new Arts::StereoEffect;
    *m_pFX = Arts::DynamicCast( pApp->m_Server.createObject( std::string( label.ascii() ) ) );
    m_pFX->start();

    m_ID = pApp->m_effectStack.insertBottom( *m_pFX, std::string( label.ascii() ) );

    if ( !m_ID )
    {
        kdDebug() << "insertBottom failed" << endl;
        m_pFX->stop();
        return;
    }
}



EffectListItem::~EffectListItem()
{
    m_pFX->stop();
    pApp->m_effectStack.remove( m_ID );

    delete m_pFX;
}



// CLASS EffectConfigWidget --------------------------------------------------------

ArtsConfigWidget::ArtsConfigWidget( Arts::Object object, QWidget *parent ) : QWidget( parent, 0, Qt::WType_TopLevel | Qt::WDestructiveClose )
{
    setCaption( kapp->makeStdCaption( QString( object._interfaceName().c_str() ) ) );

    Arts::GenericGuiFactory factory;
    m_gui = factory.createGui( object );

    if ( m_gui.isNull() )
    {
        kdDebug() << "Arts::Widget gui == NULL! Returning.." << endl;
        return;
    }

    else
    {
        m_pArtsWidget = new KArtsWidget( m_gui, this );

        QBoxLayout *lay = new QHBoxLayout( this );
        lay->add( m_pArtsWidget );
    }
}



ArtsConfigWidget::~ArtsConfigWidget()
{
    delete m_pArtsWidget;
    m_gui = Arts::Widget::null();
}



// METHODS -------------------------------------------------------------------------

void EffectListItem::configure()
{
    ArtsConfigWidget *pWidget = new ArtsConfigWidget( *m_pFX, pApp->m_pPlayerWidget );
    pWidget->show();
}



bool EffectListItem::configurable() const
{
    Arts::TraderQuery query;
    query.supports( "Interface", "Arts::GuiFactory" );
    query.supports( "CanCreate", m_pFX->_interfaceName() );

    std::vector<Arts::TraderOffer> *queryResults = query.query();
    bool yes = queryResults->size();
    delete queryResults;

    return yes;
}



// CLASS EffectWidget -----------------------------------------------------------

EffectWidget::EffectWidget( QWidget *parent, const char *name ) : KDialogBase( parent, name, false )
{
    setName( "EffectWidget" );
    setWFlags( Qt::WType_TopLevel );
    setCaption( kapp->makeStdCaption( i18n("Effects") ) );
    showButtonApply( false );
    showButtonCancel( false );
    setButtonText( Ok, i18n("Close") );

    QVBox *pFrame = makeVBoxMainWidget();
    pFrame->layout()->setResizeMode( QLayout::FreeResize );
    KIconLoader iconLoader;

    m_pGroupBoxTop = new QGroupBox( 2, Qt::Horizontal, i18n("Available Effects"), pFrame );
    m_pGroupBoxTop->setInsideSpacing( KDialog::spacingHint() );
    m_pGroupBoxBot = new QGroupBox( 2, Qt::Horizontal, i18n("Active Effects"), pFrame );

    pFrame->setStretchFactor( m_pGroupBoxTop, 1 );
    pFrame->setStretchFactor( m_pGroupBoxBot, 10 );

    m_pComboBox = new KComboBox( m_pGroupBoxTop );
    m_pComboBox->setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum, 9, 9 ) ) ;
    m_pComboBox->insertStrList( queryEffects() );

    m_pButtonTopDown = new QPushButton( iconLoader.loadIconSet( "down", KIcon::Toolbar, KIcon::SizeSmall ),
        0, m_pGroupBoxTop );
    m_pButtonTopDown->setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum, 1, 1 ) );
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
    m_pButtonBotConf->resize( m_pButtonBotConf->minimumSize() );
    m_pButtonBotConf->setEnabled( false );
    QToolTip::add( m_pButtonBotConf, i18n( "Configure" ) );
    connect( m_pButtonBotConf, SIGNAL( clicked() ), this, SLOT( slotButtonBotConf() ) );

    m_pButtonBotRem = new QPushButton( iconLoader.loadIconSet( "remove", KIcon::Toolbar, KIcon::SizeSmall ),
        0, pContainerBotButtons );
    m_pButtonBotRem->resize( m_pButtonBotRem->minimumSize() );
    QToolTip::add( m_pButtonBotRem, i18n("Remove") );
    connect( m_pButtonBotRem, SIGNAL( clicked() ), this, SLOT( slotButtonBotRem() ) );

    QBoxLayout *pLayoutBotButtons = new QVBoxLayout( pContainerBotButtons );
    pLayoutBotButtons->setResizeMode( QLayout::FreeResize );
    pLayoutBotButtons->addWidget( m_pButtonBotConf );
    pLayoutBotButtons->addWidget( m_pButtonBotRem );
    pLayoutBotButtons->addItem( new QSpacerItem( 0, 10 ) );

    resize( 300, 400 );
}



EffectWidget::~EffectWidget()
{
}



// METHODS ------------------------------------------------------------------------

QStrList EffectWidget::queryEffects() const
{
    QStrList val;
    Arts::TraderQuery query;
    query.supports( "Interface", "Arts::StereoEffect" );
    query.supports( "Interface", "Arts::SynthModule" );
    std::vector<Arts::TraderOffer> *offers = query.query();

    for ( std::vector<Arts::TraderOffer>::iterator i = offers->begin(); i != offers->end(); i++ )
    {
        Arts::TraderOffer &offer = *i;
        QCString name = offer.interfaceName().c_str();
        val.append( name );
    }
    delete offers;
    return val;
}



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
        m_pButtonBotConf->setEnabled( pEffect->configurable() );
    }

    else
    {
        m_pButtonBotConf->setEnabled( false );
    }
}

#include "effectwidget.moc"
