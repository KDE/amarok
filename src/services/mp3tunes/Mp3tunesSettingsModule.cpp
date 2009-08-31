/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "Mp3tunesSettingsModule.h"

#include "ui_Mp3tunesConfigWidget.h"

#include <kdebug.h>
#include <kgenericfactory.h>
#include <QVBoxLayout>

K_PLUGIN_FACTORY( Mp3tunesSettingsFactory, registerPlugin<Mp3tunesSettingsModule>(); )
K_EXPORT_PLUGIN( Mp3tunesSettingsFactory( "kcm_amarok_mp3tunes" ) )

Mp3tunesSettingsModule::Mp3tunesSettingsModule( QWidget *parent, const QVariantList &args )
    : KCModule( Mp3tunesSettingsFactory::componentData(), parent, args )
{

    kDebug( 14310 ) << "Creating Mp3tunes config object";

    QVBoxLayout* l = new QVBoxLayout( this );
    QWidget *w = new QWidget;
    m_configDialog = new Ui::Mp3tunesConfigWidget;
    m_configDialog->setupUi( w );
    //m_configDialog->pinEdit->setReadOnly( true );
    l->addWidget( w );

    m_configDialog->passwordEdit->setEchoMode( QLineEdit::Password );
    connect ( m_configDialog->emailEdit, SIGNAL( textChanged ( const QString & ) ), this, SLOT( settingsChanged() ) );
    connect ( m_configDialog->passwordEdit, SIGNAL( textChanged ( const QString & ) ), this, SLOT( settingsChanged() ) );
    //connect ( m_configDialog->enableHarmony, SIGNAL( stateChanged ( int ) ), this, SLOT( settingsChanged() ) );

    load();

}


Mp3tunesSettingsModule::~Mp3tunesSettingsModule()
{
}


void Mp3tunesSettingsModule::save()
{
    m_config.setEmail( m_configDialog->emailEdit->text() );
    m_config.setPassword( m_configDialog->passwordEdit->text() );
    //m_config.setPin( m_configDialog->pinEdit->text() );
    //m_config.setHarmonyEnabled( m_configDialog->enableHarmony->isChecked() );

    m_config.save();
    KCModule::save();
}

void Mp3tunesSettingsModule::load()
{
    m_configDialog->emailEdit->setText( m_config.email() );
    m_configDialog->passwordEdit->setText( m_config.password() );
    //m_configDialog->enableHarmony->setChecked( m_config.harmonyEnabled() );
    //m_configDialog->pinEdit->setText( m_config.pin() );

    KCModule::load();
}

void Mp3tunesSettingsModule::defaults()
{
    m_config.setEmail( QString() );
    m_config.setPassword( QString() );
    //m_config.setHarmonyEnabled( false );
    load();
}

void Mp3tunesSettingsModule::settingsChanged()
{
    emit changed( true );
}

#include "Mp3tunesSettingsModule.moc"
