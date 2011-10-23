/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
 * The Amazon store in based upon the Magnatune store in Amarok,                        *
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#include "AmazonSettingsModule.h"

#include "AmazonCart.h"
#include "AmazonCollection.h"
#include "AmazonConfig.h"
#include "AmazonMeta.h"

#include "ui_AmazonConfigWidget.h"

#include <kgenericfactory.h>
#include <QVBoxLayout>


K_PLUGIN_FACTORY( AmazonSettingsFactory, registerPlugin<AmazonSettingsModule>(); )
K_EXPORT_PLUGIN( AmazonSettingsFactory( "kcm_amarok_service_amazonstore" ) )

AmazonSettingsModule::AmazonSettingsModule( QWidget *parent, const QVariantList &args )
    : KCModule( AmazonSettingsFactory::componentData(), parent, args )
{
    QVBoxLayout* l = new QVBoxLayout( this );
    QWidget *w = new QWidget;
    m_configDialog = new Ui::AmazonConfigWidget;
    m_configDialog->setupUi( w );
    l->addWidget( w );

    connect( m_configDialog->countrySelectionComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( settingsChanged() ) );

    setButtons( KCModule::NoAdditionalButton ); // TODO: does not work
    load();
}

AmazonSettingsModule::~AmazonSettingsModule()
{
}

void AmazonSettingsModule::save()
{
    switch( m_configDialog->countrySelectionComboBox->currentIndex() )
    {
    case 0:
        AmazonConfig::instance()->setCountry( QString ( "fr" ) );
        break;

    case 1:
        AmazonConfig::instance()->setCountry( QString ( "de" ) );
        break;

    case 2:
        AmazonConfig::instance()->setCountry( QString ( "co.jp" ) );
        break;

    case 3:
        AmazonConfig::instance()->setCountry( QString ( "co.uk" ) );
        break;

    case 4:
        AmazonConfig::instance()->setCountry( QString ( "com" ) );
        break;

    default:
        AmazonConfig::instance()->setCountry( QString ( "" ) );
        break;
    }

    KCModule::save();
}

void AmazonSettingsModule::load()
{
    int index = -1;
    QString text = AmazonConfig::instance()->country();

    if( text == QString( "fr" ) )
        index = 0;
    else if ( text == QString( "de" ) )
        index = 1;
    else if ( text == QString( "co.jp" ) )
        index = 2;
    else if ( text == QString( "co.uk" ) )
        index = 3;
    else if ( text == QString( "com" ) )
        index = 4;
    // no match -> index is still -1

    m_configDialog->countrySelectionComboBox->setCurrentIndex( index );

    KCModule::load();
}

void AmazonSettingsModule::defaults()
{
}

void AmazonSettingsModule::settingsChanged()
{
    // TODO: clear cart and collection, as they are no longer valid
    emit changed( true );
}
