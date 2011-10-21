/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
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

#include "Amazon.h"
#include "AmazonCart.h"
#include "AmazonShoppingCartDialog.h"

#include "ui_AmazonShoppingCartDialog.h"

AmazonShoppingCartDialog::AmazonShoppingCartDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AmazonShoppingCartDialog)
{
    ui->setupUi( this );

    m_model = new QStringListModel;
    m_model->setStringList( AmazonCart::instance()->list() );
    ui->listView->setModel( m_model );
    ui->cartValueLabel->setText( i18n( "Shopping cart value: %1", Amazon::prettyPrice( AmazonCart::instance()->price() ) ) );
}

AmazonShoppingCartDialog::~AmazonShoppingCartDialog()
{
    delete ui;
}
