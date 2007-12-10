/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#include "AmpacheSettings.h"

#include "ui_AmpacheConfigWidget.h"

#include <kdebug.h>
#include <kgenericfactory.h>

#include <QVBoxLayout>

K_PLUGIN_FACTORY(AmpacheSettingsFactory, registerPlugin<AmpacheSettings>();)
K_EXPORT_PLUGIN(AmpacheSettingsFactory( "kcm_amarok_ampache" ))


AmpacheSettings::AmpacheSettings(QWidget * parent, const QVariantList & args)
    : KCModule( AmpacheSettingsFactory::componentData(), parent, args )
{

    kDebug( 14310 ) << "Creating Ampache config object";

    QVBoxLayout* l = new QVBoxLayout( this );
    QWidget *w = new QWidget;
    m_configDialog = new Ui::AmpacheConfigWidget;
    m_configDialog->setupUi( w );
    l->addWidget( w );


    connect ( m_configDialog->addButton, SIGNAL( clicked() ), this, SLOT( add() ) );
    connect ( m_configDialog->removeButton, SIGNAL( clicked() ), this, SLOT( remove() ) );
    connect ( m_configDialog->serverList, SIGNAL ( currentTextChanged ( const QString & ) ), this, SLOT( selectedItemChanged( const QString & ) ) );

    load();
}

AmpacheSettings::~AmpacheSettings()
{
}


void AmpacheSettings::save()
{

    kDebug( 14310 ) << "save";

    m_config.save();
    
    KCModule::save();
}

void AmpacheSettings::load()
{

    kDebug( 14310 ) << "load";

    foreach( QString name, m_config.servers().keys() ) {
        m_configDialog->serverList->addItem( name );
    }
    
    KCModule::load();
}

void AmpacheSettings::defaults()
{
    kDebug( 14310 ) << "defaults";
}


void AmpacheSettings::add()
{
    kDebug( 14310 ) << "add";
    
    QString name = m_configDialog->nameEdit->text();

    AmpacheServerEntry server;

    server.url = m_configDialog->serverEdit->text();
    server.username = m_configDialog->userEdit->text();
    server.password = m_configDialog->passEdit->text();

    m_configDialog->serverList->addItem( name );

    m_config.addServer( name, server );

    emit changed( true );
}

void AmpacheSettings::selectedItemChanged(const QString & name)
{
    kDebug( 14310 ) << "Selection changed";

    if ( name != "" ) {
        AmpacheServerEntry server = m_config.servers().value( name );
    
        m_configDialog->nameEdit->setText( name );
        m_configDialog->serverEdit->setText( server.url );
        m_configDialog->userEdit->setText( server.username );
        m_configDialog->passEdit->setText( server.password );


        m_configDialog->removeButton->setEnabled( true );

    } else {
        m_configDialog->removeButton->setEnabled( false );
    }

    
    
}

void AmpacheSettings::remove()
{

    QString selectedName = m_configDialog->serverList->takeItem( m_configDialog->serverList->currentRow() )->text();
    m_config.removeServer( selectedName );
    m_configDialog->nameEdit->setText( QString() );
    m_configDialog->serverEdit->setText( QString() );
    m_configDialog->userEdit->setText( QString() );
    m_configDialog->passEdit->setText( QString() );
    
}

#include "AmpacheSettings.moc"

