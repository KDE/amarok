/*
 Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

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

#include "debug.h"
#include "magnatunedatabasehandler.h"
#include "magnatunepurchasedialog.h"

#include <qcombobox.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qlabel.h>


MagnatunePurchaseDialog::MagnatunePurchaseDialog( QWidget* parent, const char* name, bool modal, WFlags fl )
        : magnatunePurchaseDialogBase( parent, name, modal, fl )
{}

MagnatunePurchaseDialog::~MagnatunePurchaseDialog()
{}

void MagnatunePurchaseDialog::setAlbum( const MagnatuneAlbum &album )
{

    //albumEdit->setText("Hello!");
    albumEdit->setText( album.getName() );

    MagnatuneArtist artist = MagnatuneDatabaseHandler::instance()->getArtistById( album.getArtistId() );
    artistEdit->setText( artist.getName() );
    genresEdit->setText( album.getMp3Genre() );
    launchDateEdit->setText( QString::number( album.getLaunchDate().year() ) );

    m_albumCode = album.getAlbumCode();

}

void MagnatunePurchaseDialog::purchase( )
{

    if ( verifyEntries( ) )
    {

	setEnabled( false ); //to prevent accidental double purchases
        emit( makePurchase( ccEdit->text(), expYearEdit->text(), expMonthEdit->text(), nameEdit->text(), emailEdit->text(), m_albumCode, amountComboBox->currentText().toInt() ) );

        //close();
        //hide();

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

    //cc number:
    QString ccString = ccEdit->text();
    ccString.stripWhiteSpace ();
    QRegExp ccExp( "^[\\d]{10,20}$" );

    if ( !ccExp.exactMatch( ccString ) )
    {
        QMessageBox::information( this, "Invalid credit card number",
                                  "The credit card number entered does not appear to be valid\n" );
        return false;
    }

    //email
    QString emailString = emailEdit->text();
    emailString.stripWhiteSpace ();
    QRegExp emailExp( "^\\S+@\\S+\\.\\S+$" );

    if ( !emailExp.exactMatch( emailString ) )
    {
        QMessageBox::information( this, "Invalid email",
                                  "The email address entered does not appear to be valid\n" );
        return false;
    }

    //month
    QString monthString = expMonthEdit->text();
    monthString.stripWhiteSpace ();
    QRegExp monthExp( "^\\d{2}$" );

    if ( !monthExp.exactMatch( monthString ) )
    {
        QMessageBox::information( this, "Invalid expiration month",
                                  "The credit card expiration month does not appear to be valid\n" );
        return false;
    }

    //month
    QString yearString = expYearEdit->text();
    yearString.stripWhiteSpace ();
    QRegExp yearExp( "^\\d{2}$" );

    if ( !yearExp.exactMatch( yearString ) )
    {
        QMessageBox::information( this, "Invalid expiration month",
                                  "The credit card expiration year does not appear to be valid\n" );
        return false;
    }




    return true;

}


void MagnatunePurchaseDialog::setCover( QString coverFile )
{
    coverPixmapLabel->setPixmap( QPixmap( coverFile ) );
}



/*$SPECIALIZATION$*/


#include "magnatunepurchasedialog.moc"

