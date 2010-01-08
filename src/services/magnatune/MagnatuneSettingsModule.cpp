/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MagnatuneSettingsModule.h"

#include "MagnatuneMeta.h"

#include "ui_MagnatuneConfigWidget.h"

#include <kdebug.h>
#include <kgenericfactory.h>
#include <QVBoxLayout>



K_PLUGIN_FACTORY( MagnatuneSettingsFactory, registerPlugin<MagnatuneSettingsModule>(); )
K_EXPORT_PLUGIN( MagnatuneSettingsFactory( "kcm_amarok_mp3tunes" ) )

MagnatuneSettingsModule::MagnatuneSettingsModule( QWidget *parent, const QVariantList &args )
    : KCModule( MagnatuneSettingsFactory::componentData(), parent, args )
{
    kDebug( 14310 ) << "Creating Mp3tunes config object";

    QVBoxLayout* l = new QVBoxLayout( this );
    QWidget *w = new QWidget;
    m_configDialog = new Ui::MagnatuneConfigWidget;
    m_configDialog->setupUi( w );
    l->addWidget( w );

    m_configDialog->passwordEdit->setEchoMode( QLineEdit::Password );
    connect ( m_configDialog->usernameEdit, SIGNAL( textChanged ( const QString & ) ), this, SLOT( settingsChanged() ) );
    connect ( m_configDialog->passwordEdit, SIGNAL( textChanged ( const QString & ) ), this, SLOT( settingsChanged() ) );
    connect ( m_configDialog->emailEdit, SIGNAL( textChanged ( const QString & ) ), this, SLOT( settingsChanged() ) );
    connect ( m_configDialog->typeComboBox, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( settingsChanged() ) );
    connect ( m_configDialog->isMemberCheckbox, SIGNAL( stateChanged ( int ) ), this, SLOT( settingsChanged() ) );
    connect ( m_configDialog->streamTypeComboBox, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( settingsChanged() ) );
    

    load();

}


MagnatuneSettingsModule::~MagnatuneSettingsModule()
{
}

void MagnatuneSettingsModule::save()
{
    m_config.setIsMember( m_configDialog->isMemberCheckbox->checkState() == Qt::Checked );


    int typeIndex = m_configDialog->typeComboBox->currentIndex();
    if( typeIndex == 0 )
        m_config.setMembershipType( MagnatuneConfig::STREAM );
    else
        m_config.setMembershipType( MagnatuneConfig::DOWNLOAD );
    
    m_config.setUsername( m_configDialog->usernameEdit->text() );
    m_config.setPassword( m_configDialog->passwordEdit->text() );
    m_config.setEmail( m_configDialog->emailEdit->text() );


    QString streamTypeString = m_configDialog->streamTypeComboBox->currentText();
    m_config.setStreamType( m_configDialog->streamTypeComboBox->currentIndex() );

    m_config.save();
    KCModule::save();

}

void MagnatuneSettingsModule::load()
{
    if ( m_config.isMember() )
        m_configDialog->isMemberCheckbox->setCheckState( Qt::Checked );
    else
        m_configDialog->isMemberCheckbox->setCheckState( Qt::Unchecked );

    int index = 0;
    if ( m_config.membershipType() == MagnatuneConfig::STREAM ) index = 0;
    else if ( m_config.membershipType() == MagnatuneConfig::DOWNLOAD ) index = 1;
    
    m_configDialog->typeComboBox->setCurrentIndex( index );
    m_configDialog->usernameEdit->setText( m_config.username() );
    m_configDialog->passwordEdit->setText( m_config.password() );
    m_configDialog->emailEdit->setText( m_config.email() );

    m_configDialog->streamTypeComboBox->setCurrentIndex( m_config.streamType() );

    KCModule::load();
}

void MagnatuneSettingsModule::defaults()
{
}

void MagnatuneSettingsModule::settingsChanged()
{
    emit changed( true );
}


