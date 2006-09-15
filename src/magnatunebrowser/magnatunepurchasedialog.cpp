// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution


#include "magnatunepurchasedialog.h"
#include "magnatunedatabasehandler.h"
#include <qlineedit.h>
#include <qcombobox.h>
#include <qregexp.h>
#include <qmessagebox.h>
 #include "debug.h"

MagnatunePurchaseDialog::MagnatunePurchaseDialog(QWidget* parent, const char* name, bool modal, WFlags fl)
: magnatunePurchaseDialogBase(parent,name, modal,fl)
{
}

MagnatunePurchaseDialog::~MagnatunePurchaseDialog()
{
}

void MagnatunePurchaseDialog::setAlbum(MagnatuneAlbum * album )
{
   
   //albumEdit->setText("Hello!");
   albumEdit->setText(album->getName());

   MagnatuneArtist artist = MagnatuneDatabaseHandler::instance()->getArtistById(album->getArtistId());
   artistEdit->setText(artist.getName());
   genresEdit->setText(album->getMp3Genre());
   launchDateEdit->setText(QString::number(album->getLaunchDate().year()));

   m_albumCode = album->getAlbumCode();

}

void MagnatunePurchaseDialog::purchase( )
{

   if (verifyEntries( )) {

      emit(makePurchase(ccEdit->text(), expYearEdit->text(), expMonthEdit->text(), nameEdit->text(), emailEdit->text(), m_albumCode, amountComboBox->currentText().toInt()));

      //close();
      //hide();

   }
}

void MagnatunePurchaseDialog::cancel( )
{ 
   close();
}

bool MagnatunePurchaseDialog::verifyEntries( )
{

   // check all the entries for validity
  
   //cc number:
   QString ccString = ccEdit->text();
   ccString.stripWhiteSpace ();
   QRegExp ccExp("^[\\d]{10,20}$");

   if (!ccExp.exactMatch(ccString)) {
      QMessageBox::information( this, "Invalid credit card number",
    "The credit card number entered does not appear to be valid\n");
      return false;
   }

    //email
   QString emailString = emailEdit->text();
   emailString.stripWhiteSpace ();
   QRegExp emailExp("^\\S+@\\S+\\.\\S+$");

   if (!emailExp.exactMatch(emailString)) {
      QMessageBox::information( this, "Invalid email",
    "The email address entered does not appear to be valid\n");
      return false;
   }

    //month
   QString monthString = expMonthEdit->text();
   monthString.stripWhiteSpace ();
   QRegExp monthExp("^\\d{2}$");

   if (!monthExp.exactMatch(monthString)) {
      QMessageBox::information( this, "Invalid expiration month",
    "The credit card expitation month does not appear to be valid\n");
      return false;
   }

    //month
   QString yearString = expYearEdit->text();
   yearString.stripWhiteSpace ();
   QRegExp yearExp("^\\d{2}$");

   if (!yearExp.exactMatch(yearString)) {
      QMessageBox::information( this, "Invalid expiration month",
    "The credit card expitation year does not appear to be valid\n");
      return false;
   }




   return true;

}


/*$SPECIALIZATION$*/


#include "magnatunepurchasedialog.moc"

