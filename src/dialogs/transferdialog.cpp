/****************************************************************************************
 * Copyright (c) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com>                         *
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

#include "transferdialog.h"

#include "Amarok.h"
#include "Debug.h"
#include "mediabrowser.h"
#include "MediaDevice.h"

#include <KComboBox>
#include <KLocale>
#include <KVBox>

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>


TransferDialog::TransferDialog( MediaDevice *mdev )
        : KDialog( Amarok::mainWindow() )
{
    setModal( true );
    setButtons( Ok|Cancel );
    setDefaultButton( Ok );
    showButtonSeparator( true );

    m_dev = mdev;
    m_accepted = false;
    m_sort1LastIndex = m_sort2LastIndex = -1;

    kapp->setTopWidget( this );
    setCaption( KDialog::makeStandardCaption( i18n( "Transfer Queue to Device" ) ) );

    KVBox *vbox = new KVBox( this );
    setMainWidget( vbox );

    QString transferDir = mdev->getTransferDir();

    QGroupBox *location = new QGroupBox( i18n( "Music Location" ), vbox );
    location->setAlignment( Qt::Vertical );
    KVBox *locationLayout = new KVBox( location );

    new QLabel( i18n( "Your music will be transferred to:\n%1",
                    transferDir ),
                locationLayout );

    KVBox *vbox2 = new KVBox( vbox );
    QLayout *vlayout = vbox2->layout();
    if( vlayout )
        vlayout->addItem( new QSpacerItem( 0, 25 ) );

    new QLabel( i18n( "You can have your music automatically grouped in\n"
                      "a variety of ways. Each grouping will create\n"
                      "directories based upon the specified criteria.\n"), vbox );

    QGroupBox *sorting = new QGroupBox( i18n( "Groupings" ), vbox );
    sorting->setAlignment( Qt::Vertical );
    KVBox *sortingLayout = new KVBox( sorting );
    m_label1 = new QLabel( i18n( "Select first grouping:\n" ), sortingLayout );
    m_sort1  = new KComboBox( sortingLayout );
    m_label2 = new QLabel( i18n( "Select second grouping:\n" ), sortingLayout );
    m_sort2  = new KComboBox( sortingLayout );
    m_label3 = new QLabel( i18n( "Select third grouping:\n" ), sortingLayout );
    m_sort3  = new KComboBox( sortingLayout );

    m_combolist.append( m_sort1 );
    m_combolist.append( m_sort2 );
    m_combolist.append( m_sort3 );

    QListIterator<KComboBox *> it(m_combolist);
    while (it.hasNext()) {
        it.next()->addItem( i18n("None") );
        it.next()->addItem( i18n("Artist") );
        it.next()->addItem( i18n("Album") );
        it.next()->addItem( i18n("Genre") );
        it.next()->setCurrentItem( 0 );
    }

    m_sort1->setCurrentItem( mdev->m_firstSort );
    m_sort2->setCurrentItem( mdev->m_secondSort );
    m_sort3->setCurrentItem( mdev->m_thirdSort );

    m_label2->setDisabled( m_sort1->currentIndex() == 0 );
    m_sort2->setDisabled( m_sort1->currentIndex() == 0 );
    m_label3->setDisabled( m_sort2->currentIndex() == 0 );
    m_sort3->setDisabled( m_sort2->currentIndex() == 0 );

    connect( m_sort1, SIGNAL( activated(int) ), SLOT( sort1_activated(int)) );
    connect( m_sort2, SIGNAL( activated(int) ), SLOT( sort2_activated(int)) );

    KVBox *vbox3 = new KVBox( vbox );
    QLayout *vlayout2 = vbox3->layout();
    if( vlayout2 )
        vlayout2->addItem( new QSpacerItem( 0, 25 ) );

    QGroupBox *options = new QGroupBox( i18n( "Options" ), vbox );
    options->setAlignment( Qt::Vertical );
    KVBox *optionsLayout = new KVBox( options );

    QCheckBox *convertSpaces = new QCheckBox( i18n( "Convert spaces to underscores" ), optionsLayout );
    convertSpaces->setChecked( mdev->getSpacesToUnderscores() );

    connect( convertSpaces, SIGNAL( toggled(bool) ), this, SLOT( convertSpaces_toggled(bool) ) );
    connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
    connect(this,SIGNAL(cancelClicked()),this,SLOT(slotCancel()));
}

void
TransferDialog::slotOk()
{
    m_accepted = true;
    //KDialog::slotOk();
    slotButtonClicked( Ok );

    m_dev->setFirstSort( m_sort1->currentText() );
    m_dev->setSecondSort( m_sort2->currentText() );
    m_dev->setThirdSort( m_sort3->currentText() );
}

void
TransferDialog::slotCancel()
{
    m_accepted = false;
    //KDialog::slotCancel();
    slotButtonClicked( Cancel );
}

void
TransferDialog::sort1_activated( int index )
{
    //sort3
    if( m_sort2LastIndex > 0 )
        m_sort3->addItem( m_sort2->itemText( m_sort2LastIndex ), m_sort2LastIndex );
    if( m_sort1LastIndex > 0 )
        m_sort3->addItem( m_sort1->itemText( m_sort1LastIndex ), m_sort1LastIndex );
    if( index > 0 )
        m_sort3->removeItem( index );
    m_sort3->setCurrentItem( 0 );
    m_sort3->setDisabled( true );
    //sort2
    if( m_sort1LastIndex > 0 )
        m_sort2->addItem( m_sort1->itemText( m_sort1LastIndex ), m_sort1LastIndex );
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
        m_sort3->addItem( m_sort2->itemText( m_sort2LastIndex ), m_sort2LastIndex );
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

