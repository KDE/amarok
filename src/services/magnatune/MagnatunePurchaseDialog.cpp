/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "MagnatunePurchaseDialog.h"

#include "Debug.h"
#include "MagnatuneDatabaseHandler.h"
#include "meta/Meta.h"

#include <KMessageBox>

#include <QPixmap>
#include <QRegExp>

using namespace Meta;

MagnatunePurchaseDialog::MagnatunePurchaseDialog( QWidget* parent, const char* name, bool modal, Qt::WFlags fl )
        : QDialog( parent, fl )
{
    Q_UNUSED( modal );
    DEBUG_BLOCK

    setObjectName( name );
    setupUi( this );

    connect( ccRadioButton, SIGNAL( clicked() ), this, SLOT( useCc() ) );
    connect( gcRadioButton, SIGNAL( clicked() ), this, SLOT( useGc() ) );

    ccRadioButton->setChecked ( true );
}

MagnatunePurchaseDialog::~MagnatunePurchaseDialog()
{
    DEBUG_BLOCK
}

void MagnatunePurchaseDialog::setAlbum( MagnatuneAlbum * album )
{
    //albumEdit->setText("Hello!");
    albumEdit->setText( album->name() );

    artistEdit->setText( album->albumArtist()->name() );
    //genresEdit->setText( album.getMp3Genre() ); FIXME: Broken because there can be more than one genre per album
    launchDateEdit->setText( QString::number( album->launchYear() ) );

    m_albumCode = album->albumCode();

    Meta::AlbumPtr albumptr( album );
    subscribeTo( albumptr );

    coverPixmapLabel->setPixmap( album->image( 200 ) );
}

void MagnatunePurchaseDialog::purchase( )
{
    if ( verifyEntries( ) )
    {
        setEnabled( false ); //to prevent accidental double purchases

        if ( ccRadioButton->isChecked() )
            emit( makePurchase( ccEdit->text(), expYearEdit->text(), expMonthEdit->text(), nameEdit->text(), emailEdit->text(), m_albumCode, amountComboBox->currentText().toInt() ) );
        else
            emit( makeGiftCardPurchase( gcEdit->text(), nameEdit->text(), emailEdit->text(), m_albumCode, amountComboBox->currentText().toInt() ) );
    }
}

void MagnatunePurchaseDialog::reject( )
{
    cancel();
}

void MagnatunePurchaseDialog::cancel( )
{
    hide();
    emit ( cancelled() );
}

bool MagnatunePurchaseDialog::verifyEntries( )
{
    // check all the entries for validity

    //credit card entries

    if ( ccRadioButton->isChecked() ) {

        //cc number:
        QString ccString = ccEdit->text();
        ccString.trimmed();
        QRegExp ccExp( "^[\\d]{10,20}$" );

        if ( !ccExp.exactMatch( ccString ) )
        {
            KMessageBox::information( this, i18n("The credit card number entered does not appear to be valid"),
                                    i18n("Invalid credit card number"));
            return false;
        }

        //month
        QString monthString = expMonthEdit->text();
        monthString.trimmed ();
        QRegExp monthExp( "^\\d{2}$" );

        if ( !monthExp.exactMatch( monthString ) )
        {
            KMessageBox::information( this, i18n("The credit card expiration month does not appear to be valid"), 
                                    i18n("Invalid expiration month"));
            return false;
        }

        //year
        QString yearString = expYearEdit->text();
        yearString.trimmed ();
        QRegExp yearExp( "^\\d{2}$" );

        if ( !yearExp.exactMatch( yearString ) )
        {
            KMessageBox::information( this,i18n("The credit card expiration year does not appear to be valid"), i18n("Invalid expiration year"));

            return false;
        }


    } else {
        //check the gift card code
        QString ccString = gcEdit->text();
        ccString.trimmed ();
        QRegExp ccExp( "^[\\d]{10,20}$" );

        if ( !ccExp.exactMatch( ccString ) )
        {
            KMessageBox::information( this, i18n("The gift card code entered does not appear to be valid"), i18n("Invalid gift card code"));
            return false;
        }
    }

    //email
    QString emailString = emailEdit->text();
    emailString.trimmed ();
    QRegExp emailExp( "^\\S+@\\S+\\.\\S+$" );

    if ( !emailExp.exactMatch( emailString ) )
    {
        KMessageBox::information( this, i18n("The email address entered does not appear to be valid"), i18n("Invalid email"));
        return false;
    }

    return true;
}

/*void MagnatunePurchaseDialog::setCover( const QString &coverFile )
{
    coverPixmapLabel->setPixmap( QPixmap( coverFile ) );
}*/

void MagnatunePurchaseDialog::metadataChanged( AlbumPtr album )
{
    coverPixmapLabel->setPixmap( album->image( 200 ) );
}


/*$SPECIALIZATION$*/

void MagnatunePurchaseDialog::useCc()
{
    paymentWidget->setCurrentIndex( 0 );

}

void MagnatunePurchaseDialog::useGc()
{
    paymentWidget->setCurrentIndex( 1 );
}


#include "MagnatunePurchaseDialog.moc"

