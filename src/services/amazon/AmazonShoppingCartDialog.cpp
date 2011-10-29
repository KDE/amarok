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

#include "AmazonShoppingCartDialog.h"

#include "Amazon.h"
#include "AmazonCart.h"

#include "ui_AmazonShoppingCartDialog.h"

AmazonShoppingCartDialog::AmazonShoppingCartDialog( QWidget *parent, AmazonStore *store ) :
    QDialog( parent ),
    ui( new Ui::AmazonShoppingCartDialog ),
    m_store( store )
{
    ui->setupUi( this );

    m_model = new AmazonShoppingCartModel;
    m_model->setStringList( AmazonCart::instance()->stringList() );
    ui->listView->setModel( m_model );
    ui->cartValueLabel->setText( i18n( "Shopping cart value: %1", Amazon::prettyPrice( AmazonCart::instance()->price() ) ) );

    if( AmazonCart::instance()->isEmpty() )
        ui->checkoutButton->setEnabled( false );
    else
        ui->checkoutButton->setEnabled( true );

    connect( ui->checkoutButton, SIGNAL( clicked() ), m_store, SLOT( checkout() ) );
    connect( ui->checkoutButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( m_model, SIGNAL( contentsChanged() ), this, SLOT( contentsChanged() ) );
}

AmazonShoppingCartDialog::~AmazonShoppingCartDialog()
{
    delete ui;
}


/* public slots */

void
AmazonShoppingCartDialog::contentsChanged()
{
    // update price
    ui->cartValueLabel->setText( i18n( "Shopping cart value: %1", Amazon::prettyPrice( AmazonCart::instance()->price() ) ) );

    // update view
    m_model->setStringList( AmazonCart::instance()->stringList() ); // HACK, but works
    ui->listView->setModel( m_model );

    // update button status
    if( AmazonCart::instance()->isEmpty() )
        ui->checkoutButton->setEnabled( false );
    else
        ui->checkoutButton->setEnabled( true );
}
