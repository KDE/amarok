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

#include "AmazonCollection.h"
#include "AmazonConfig.h"
#include "AmazonMeta.h"

#include "ui_AmazonConfigWidget.h"

#include <kgenericfactory.h>

#include <QTimer>
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
}

AmazonSettingsModule::~AmazonSettingsModule()
{
    // TODO: clear cart and collection, if the settings have changed
}

void
AmazonSettingsModule::save()
{
    switch( m_configDialog->countrySelectionComboBox->currentIndex() )
    {
    case AMAZON_COM:
        AmazonConfig::instance()->setCountry( QLatin1String( "com" ) );
        break;

    case AMAZON_DE:
        AmazonConfig::instance()->setCountry( QLatin1String( "de" ) );
        break;

    case AMAZON_ES:
        AmazonConfig::instance()->setCountry( QLatin1String( "es" ) );
        break;

    case AMAZON_FR:
        AmazonConfig::instance()->setCountry( QLatin1String( "fr" ) );
        break;

    case AMAZON_IT:
        AmazonConfig::instance()->setCountry( QLatin1String( "it" ) );
        break;

    case AMAZON_JP:
        AmazonConfig::instance()->setCountry( QLatin1String( "co.jp" ) );
        break;

    case AMAZON_UK:
        AmazonConfig::instance()->setCountry( QLatin1String( "co.uk" ) );
        break;

    case AMAZON_NONE:
        AmazonConfig::instance()->setCountry( QLatin1String( "none" ) );
        break;

    default:
        break;
    }
}

void
AmazonSettingsModule::load()
{
    int index = -1;
    QString text = AmazonConfig::instance()->country();

    if ( text == QLatin1String( "co.jp" ) )
        index = AMAZON_JP;
    else if ( text == QLatin1String( "co.uk" ) )
        index = AMAZON_UK;
    else if ( text == QLatin1String( "com" ) )
        index = AMAZON_COM;
    else if ( text == QLatin1String( "de" ) )
        index = AMAZON_DE;
    else if ( text == QLatin1String( "es" ) )
        index = AMAZON_ES;
    else if( text == QLatin1String( "fr" ) )
        index = AMAZON_FR;
    else if ( text == QLatin1String( "it" ) )
        index = AMAZON_IT;
    else if ( text == QLatin1String( "none" ) )
        index = AMAZON_NONE;

    if( index != -1 )
        m_configDialog->countrySelectionComboBox->setCurrentIndex( index );
    else
    {
        defaults();
        /*
         * The following line is my entry to the "Ugliest Hack of the Year" contest.
         *
         * load() is being called during initialization of the KCModule. In that phase
         * the connections to react to changes in the widget seem not yet to be set up.
         *
         * As a result, when
         * 1. we guess the location and propose it in the widget
         * 2. the user clicks OK without doing anything else
         * nothing gets saved, as the change in the widget has been done during init
         * and the changed( true ) signal is consequently being lost.
         *
         * Workaround:
         */
        QTimer::singleShot( 200, this, SLOT( settingsChanged() ) );

        /*
         * I'm going to burn in hell for that one, am I? :-/
         */
    }
}

void
AmazonSettingsModule::defaults()
{
    int index = -1;

    // try to guess
    KLocale locale( "amarok" );
    QString guess = locale.country();

    if( guess == QLatin1String( "fr" ) )
        index = AMAZON_FR;
    else if ( guess == QLatin1String( "de" ) || guess == QLatin1String( "at" ) || guess == QLatin1String( "ch" ) )
        index = AMAZON_DE;
    else if ( guess == QLatin1String( "es" ) )
        index = AMAZON_ES;
    else if ( guess == QLatin1String( "it" ) )
        index = AMAZON_IT;
    else if ( guess == QLatin1String( "jp" ) )
        index = AMAZON_JP;
    else if ( guess == QLatin1String( "gb" ) )
        index = AMAZON_UK;
    else if ( guess == QLatin1String( "us" ) )
        index = AMAZON_COM;
    else
        index = AMAZON_NONE;

    m_configDialog->countrySelectionComboBox->setCurrentIndex( index );
}

void
AmazonSettingsModule::settingsChanged()
{
    emit changed( true );
}
