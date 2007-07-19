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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 

#include "debug.h"
#include "magnatunedatabasehandler.h"
#include "magnatunepurchasedialog.h"

#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegExp>
#include <QLabel>
//Added by qt3to4:
#include <QPixmap>


MagnatunePurchaseDialog::MagnatunePurchaseDialog( QWidget* parent, const char* name, bool modal, Qt::WFlags fl )
        : magnatunePurchaseDialogBase( parent, name, modal, fl )
{}

MagnatunePurchaseDialog::~MagnatunePurchaseDialog()
{}


void MagnatunePurchaseDialog::setAlbum( MagnatuneAlbum * album )
{
    //albumEdit->setText("Hello!");
    albumEdit->setText( album->name() );

    artistEdit->setText( album->artistName() );
    //genresEdit->setText( album.getMp3Genre() ); FIXME:
    launchDateEdit->setText( QString::number( album->launchYear() ) );

    m_albumCode = album->albumCode();

}

void MagnatunePurchaseDialog::purchase( )
{

    if ( verifyEntries( ) )
    {

	setEnabled( false ); //to prevent accidental double purchases
        emit( makePurchase( ccEdit->text(), expYearEdit->text(), expMonthEdit->text(), nameEdit->text(), emailEdit->text(), m_albumCode, amountComboBox->currentText().toInt() ) );

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
    ccString.trimmed ();
    QRegExp ccExp( "^[\\d]{10,20}$" );

    if ( !ccExp.exactMatch( ccString ) )
    {
        QMessageBox::information( this, "Invalid credit card number",
                                  "The credit card number entered does not appear to be valid\n" );
        return false;
    }

    //email
    QString emailString = emailEdit->text();
    emailString.trimmed ();
    QRegExp emailExp( "^\\S+@\\S+\\.\\S+$" );

    if ( !emailExp.exactMatch( emailString ) )
    {
        QMessageBox::information( this, "Invalid email",
                                  "The email address entered does not appear to be valid\n" );
        return false;
    }

    //month
    QString monthString = expMonthEdit->text();
    monthString.trimmed ();
    QRegExp monthExp( "^\\d{2}$" );

    if ( !monthExp.exactMatch( monthString ) )
    {
        QMessageBox::information( this, "Invalid expiration month",
                                  "The credit card expitation month does not appear to be valid\n" );
        return false;
    }

    //month
    QString yearString = expYearEdit->text();
    yearString.trimmed ();
    QRegExp yearExp( "^\\d{2}$" );

    if ( !yearExp.exactMatch( yearString ) )
    {
        QMessageBox::information( this, "Invalid expiration month",
                                  "The credit card expitation year does not appear to be valid\n" );
        return false;
    }




    return true;

}


void MagnatunePurchaseDialog::setCover( const QString &coverFile )
{
    coverPixmapLabel->setPixmap( QPixmap( coverFile ) );
}



/*$SPECIALIZATION$*/


#include "magnatunepurchasedialog.moc"

