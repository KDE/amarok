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

