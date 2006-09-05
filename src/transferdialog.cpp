//
// C++ Implementation: transferdialog
//
// Description:
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "amarok.h"
#include "debug.h"
#include "mediabrowser.h"
#include "transferdialog.h"

#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qvbox.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>

TransferDialog::TransferDialog( MediaDevice *mdev )
        : KDialogBase( Amarok::mainWindow(), "transferdialog", true, QString::null, Ok|Cancel, Ok )
{
    m_dev = mdev;
    m_accepted = false;
    m_sort1LastIndex = m_sort2LastIndex = -1;

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Transfer Queue to Device" ) ) );

    QVBox* vbox = makeVBoxMainWidget();
    vbox->setSpacing( KDialog::spacingHint() );

    QString transferDir = mdev->getTransferDir();

    QGroupBox *location = new QGroupBox( 1, Qt::Vertical, i18n( "Music Location" ), vbox );

    new QLabel( i18n( "Your music will be transferred to:\n%1" )
                    .arg( transferDir ), location );

    QVBox *vbox2 = new QVBox( vbox );
    QSpacerItem *spacer = new QSpacerItem( 0, 25 );
    QLayout *vlayout = vbox2->layout();
    if( vlayout )
        vlayout->addItem( spacer );

    new QLabel( i18n( "You can have your music automatically grouped in\n"
                      "a variety of ways. Each grouping will create\n"
                      "directories based upon the specified criteria.\n"), vbox );

    QGroupBox *sorting = new QGroupBox( 6, Qt::Vertical, i18n( "Groupings" ), vbox );
    m_label1 = new QLabel( i18n( "Select first grouping:\n" ), sorting );
    m_sort1  = new KComboBox( sorting );
    m_label2 = new QLabel( i18n( "Select second grouping:\n" ), sorting );
    m_sort2  = new KComboBox( sorting );
    m_label3 = new QLabel( i18n( "Select third grouping:\n" ), sorting );
    m_sort3  = new KComboBox( sorting );

    m_combolist = new QPtrList<KComboBox>();
    m_combolist->append( m_sort1 );
    m_combolist->append( m_sort2 );
    m_combolist->append( m_sort3 );

    KComboBox * comboTemp;
    for( comboTemp = m_combolist->first(); comboTemp; comboTemp = m_combolist->next() )
    {
        comboTemp->insertItem( i18n("None") );
        comboTemp->insertItem( i18n("Artist") );
        comboTemp->insertItem( i18n("Album") );
        comboTemp->insertItem( i18n("Genre") );
        comboTemp->setCurrentItem( 0 );
    }

    m_sort1->setCurrentItem( mdev->m_firstSort );
    m_sort2->setCurrentItem( mdev->m_secondSort );
    m_sort3->setCurrentItem( mdev->m_thirdSort );

    m_label2->setDisabled( m_sort1->currentItem() == 0 );
    m_sort2->setDisabled( m_sort1->currentItem() == 0 );
    m_label3->setDisabled( m_sort2->currentItem() == 0 );
    m_sort3->setDisabled( m_sort2->currentItem() == 0 );

    connect( m_sort1, SIGNAL( activated(int) ), SLOT( sort1_activated(int)) );
    connect( m_sort2, SIGNAL( activated(int) ), SLOT( sort2_activated(int)) );

    QVBox *vbox3 = new QVBox( vbox );
    QSpacerItem *spacer2 = new QSpacerItem( 0, 25 );
    QLayout *vlayout2 = vbox3->layout();
    if( vlayout2 )
        vlayout2->addItem( spacer2 );

    QGroupBox *options = new QGroupBox( 6, Qt::Vertical, i18n( "Options" ), vbox );

    QCheckBox *convertSpaces = new QCheckBox( i18n( "Convert spaces to underscores" ), options );
    convertSpaces->setChecked( mdev->getSpacesToUnderscores() );

    connect( convertSpaces, SIGNAL( toggled(bool) ), this, SLOT( convertSpaces_toggled(bool) ) );
}

void
TransferDialog::slotOk()
{
    m_accepted = true;
    KDialogBase::slotOk();

    m_dev->setFirstSort( m_sort1->currentText() );
    m_dev->setSecondSort( m_sort2->currentText() );
    m_dev->setThirdSort( m_sort3->currentText() );
}

void
TransferDialog::slotCancel()
{
    m_accepted = false;
    KDialogBase::slotCancel();
}

void
TransferDialog::sort1_activated( int index )
{
    //sort3
    if( m_sort2LastIndex > 0 )
        m_sort3->insertItem( m_sort2->text( m_sort2LastIndex ), m_sort2LastIndex );
    if( m_sort1LastIndex > 0 )
        m_sort3->insertItem( m_sort1->text( m_sort1LastIndex ), m_sort1LastIndex );
    if( index > 0 )
        m_sort3->removeItem( index );
    m_sort3->setCurrentItem( 0 );
    m_sort3->setDisabled( true );
    //sort2
    if( m_sort1LastIndex > 0 )
        m_sort2->insertItem( m_sort1->text( m_sort1LastIndex ), m_sort1LastIndex );
    if( index > 0 )
        m_sort2->removeItem( index );
    m_sort2->setCurrentItem( 0 );
    if( index == 0 )
        m_sort2->setDisabled( true );
    else
        m_sort2->setDisabled( false );

    m_sort2LastIndex = 0;
    m_sort1LastIndex = index;
}

void
TransferDialog::sort2_activated( int index )
{
    //sort3
    if( m_sort2LastIndex > 0 )
        m_sort3->insertItem( m_sort2->text( m_sort2LastIndex ), m_sort2LastIndex );
    if( index > 0 )
        m_sort3->removeItem( index );
    m_sort3->setCurrentItem( 0 );
    if( index == 0 )
        m_sort3->setDisabled( true );
    else
        m_sort3->setDisabled( false );

    m_sort2LastIndex = index;
}

void
TransferDialog::convertSpaces_toggled( bool on )
{
    m_dev->setSpacesToUnderscores( on );
}

#include "transferdialog.moc"
