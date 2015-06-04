/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
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

#ifndef AMAZONSHOPPINGCARTDIALOG_H
#define AMAZONSHOPPINGCARTDIALOG_H

#include "AmazonShoppingCartModel.h"
#include "AmazonStore.h"

#include <QDialog>

namespace Ui {
    class AmazonShoppingCartDialog;
}

class AmazonShoppingCartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AmazonShoppingCartDialog( QWidget *parent = 0, AmazonStore *store = 0 );
    ~AmazonShoppingCartDialog();

public Q_SLOTS:
    void contentsChanged();

private:
    Ui::AmazonShoppingCartDialog *ui;
    AmazonShoppingCartModel *m_model;
    AmazonStore *m_store;
};

#endif // AMAZONSHOPPINGCARTDIALOG_H
