//
// C++ Interface: 
//
// Description: 
//
//
// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef MAGNATUNEPURCHASEDIALOG_H
#define MAGNATUNEPURCHASEDIALOG_H

#include "magnatunepurchasedialogbase.h"
#include "magnatunetypes.h"

class MagnatunePurchaseDialog : public magnatunePurchaseDialogBase
{
  Q_OBJECT

public:
  MagnatunePurchaseDialog(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
  ~MagnatunePurchaseDialog();
  /*$PUBLIC_FUNCTIONS$*/


  void setAlbum(MagnatuneAlbum * album);


signals:

  void makePurchase(QString ccNumber, QString expYear, QString expMonth, QString name, QString email, QString albumCode, int amount);

public slots:
  /*$PUBLIC_SLOTS$*/

protected:
  /*$PROTECTED_FUNCTIONS$*/

  QString m_albumCode;

  bool verifyEntries();

protected slots:
  /*$PROTECTED_SLOTS$*/

  void purchase();
  void cancel();

};

#endif

