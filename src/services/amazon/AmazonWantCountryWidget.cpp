/****************************************************************************************
 * Copyright (c) 2012 Edward "hades" Toroshchin <amarok@hades.name>                     *
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

#include "AmazonWantCountryWidget.h"

#include "ui_AmazonWantCountryWidget.h"

#include "AmazonConfig.h"
#include "AmazonSettingsModule.h"
#include "AmazonStore.h"

AmazonWantCountryWidget::AmazonWantCountryWidget(QWidget *parent) :
    QWidget(parent), ui(new Ui::AmazonWantCountryWidget)
{
    ui->setupUi(this);

    // TODO: this is a code duplication from AmazonSettingsModule.cpp
    int index = -1;
    QString text = AmazonConfig::instance()->country();
    if( text.isEmpty() || text == QLatin1String("none") )
    {
        QString country(KGlobal::locale()->country());
        text = AmazonStore::iso3166toAmazon(country);
    }

    if( text == QLatin1String( "fr" ) )
        index = AMAZON_FR;
    else if ( text == QLatin1String( "de" ) )
        index = AMAZON_DE;
    else if ( text == QLatin1String( "co.jp" ) )
        index = AMAZON_JP;
    else if ( text == QLatin1String( "co.uk" ) )
        index = AMAZON_UK;
    else if ( text == QLatin1String( "com" ) )
        index = AMAZON_COM;
    else if ( text == QLatin1String( "none" ) )
        index = AMAZON_NONE;

    if( index != -1 )
        ui->countrySelectionComboBox->setCurrentIndex( index );

    connect(ui->saveSettings, SIGNAL(clicked()), SLOT(storeCountry()));
    connect(ui->countrySelectionComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(adjustButtonState()));

    adjustButtonState();
}

void
AmazonWantCountryWidget::storeCountry()
{
    // TODO: this is a code duplication with AmazonSettingsModule.cpp
    switch( ui->countrySelectionComboBox->currentIndex() )
    {
    case AMAZON_FR:
        AmazonConfig::instance()->setCountry( QLatin1String( "fr" ) );
        break;

    case AMAZON_DE:
        AmazonConfig::instance()->setCountry( QLatin1String( "de" ) );
        break;

    case AMAZON_JP:
        AmazonConfig::instance()->setCountry( QLatin1String( "co.jp" ) );
        break;

    case AMAZON_UK:
        AmazonConfig::instance()->setCountry( QLatin1String( "co.uk" ) );
        break;

    case AMAZON_COM:
        AmazonConfig::instance()->setCountry( QLatin1String( "com" ) );
        break;

    case AMAZON_NONE:
        AmazonConfig::instance()->setCountry( QLatin1String( "none" ) );
        break;

    default:
        return;
    }

    emit countrySelected();
}

void
AmazonWantCountryWidget::adjustButtonState()
{
    ui->saveSettings->setEnabled(
        ui->countrySelectionComboBox->currentIndex() != AMAZON_NONE);
}
