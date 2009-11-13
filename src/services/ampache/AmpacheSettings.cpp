/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "AmpacheSettings.h"

#include "ui_AmpacheConfigWidget.h"

#include <kdebug.h>
#include <kgenericfactory.h>

#include <QVBoxLayout>

K_PLUGIN_FACTORY( AmpacheSettingsFactory, registerPlugin<AmpacheSettings>(); )
K_EXPORT_PLUGIN( AmpacheSettingsFactory( "kcm_amarok_ampache" ) )


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
    connect ( m_configDialog->modifyButton, SIGNAL( clicked() ), this, SLOT( modify() ) );
    connect ( m_configDialog->serverList, SIGNAL ( currentTextChanged ( const QString & ) ), this, SLOT( selectedItemChanged( const QString & ) ) );
    connect ( m_configDialog->nameEdit, SIGNAL( textChanged ( const QString & )), this,SLOT(serverNameChanged( const QString & )));
    load();
}

AmpacheSettings::~AmpacheSettings()
{
}

void
AmpacheSettings::serverNameChanged(const QString & text)
{
   m_configDialog->addButton->setEnabled( !text.isEmpty() );
}

void
AmpacheSettings::save()
{
    kDebug( 14310 ) << "save";
    m_config.save();
    KCModule::save();
}

void
AmpacheSettings::load()
{
    kDebug( 14310 ) << "load";
    for( int i = 0; i < m_config.servers().size(); i++ )
    {
        if( !m_configDialog->serverList->findItems( m_config.servers().at( i ).name, Qt::MatchFixedString | Qt::MatchCaseSensitive ).count() )
        {
            m_configDialog->serverList->addItem( m_config.servers().at( i ).name );

            // Also select the item in the list
            if( i == 0 )
                m_configDialog->serverList->item( 0 )->setSelected( true );
        }
    }

    KCModule::load();
}

void
AmpacheSettings::defaults()
{
    kDebug( 14310 ) << "defaults";
}

void
AmpacheSettings::add()
{
    kDebug( 14310 ) << "add";

    AmpacheServerEntry server;
    server.name = m_configDialog->nameEdit->text();
    if( server.name.isEmpty())
        return;
    server.url = m_configDialog->serverEdit->text();
    server.username = m_configDialog->userEdit->text();
    server.password = m_configDialog->passEdit->text();

    m_configDialog->serverList->addItem( server.name );
    m_config.addServer( server );
    emit changed( true );
}

void
AmpacheSettings::selectedItemChanged( const QString & name )
{
    kDebug( 14310 ) << "Selection changed to " << name;

    if ( !name.isEmpty() )
    {
        int index = m_configDialog->serverList->currentRow();
        kDebug( 14310 ) << "what index number did I find? " << index;
        AmpacheServerEntry server = m_config.servers().at( index );

        m_configDialog->nameEdit->setText( server.name );
        m_configDialog->serverEdit->setText( server.url );
        m_configDialog->userEdit->setText( server.username );
        m_configDialog->passEdit->setText( server.password );
        m_configDialog->removeButton->setEnabled( true );
    }
    else
    {
        m_configDialog->removeButton->setEnabled( false );
    }
}

void
AmpacheSettings::remove()
{
    int index = m_configDialog->serverList->currentRow();
    m_configDialog->serverList->takeItem( index );
    m_config.removeServer( index );
    m_configDialog->nameEdit->setText( QString() );
    m_configDialog->serverEdit->setText( QString() );
    m_configDialog->userEdit->setText( QString() );
    m_configDialog->passEdit->setText( QString() );

    emit changed( true );
}

void
AmpacheSettings::modify()
{
    int index = m_configDialog->serverList->currentRow();

    AmpacheServerEntry server;
    server.name = m_configDialog->nameEdit->text();
    server.url = m_configDialog->serverEdit->text();
    server.username = m_configDialog->userEdit->text();
    server.password = m_configDialog->passEdit->text();
    m_config.updateServer( index, server );
    m_configDialog->serverList->takeItem( index );
    m_configDialog->serverList->insertItem( index, server.name );

    emit changed( true );
}

#include "AmpacheSettings.moc"

