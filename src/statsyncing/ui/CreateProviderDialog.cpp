/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "CreateProviderDialog.h"

#include "statsyncing/Provider.h"

#include <KLocale>

#include <QVBoxLayout>
#include <QRadioButton>
#include <QDebug>

namespace StatSyncing
{

CreateProviderDialog::CreateProviderDialog( QWidget *parent, Qt::WindowFlags f )
    : KAssistantDialog( parent, f )
{
    setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
    setWindowTitle( i18n( "Add Synchronization Target" ) );
    setModal( true );

    m_providerButtons.setExclusive( true );

    QWidget *providerTypeWidget = new QWidget;
    providerTypeWidget->setLayout( new QVBoxLayout );

    m_providerTypePage = new KPageWidgetItem( providerTypeWidget,
                                              i18n( "Choose Target Type" ) );
    providerTypeWidget->hide();
    addPage( m_providerTypePage );

    connect( this, SIGNAL(accepted()), SLOT(slotFinished()) );
}

CreateProviderDialog::~CreateProviderDialog()
{
}

void
CreateProviderDialog::providerTypeAdded( QString id, QString prettyName, KIcon icon,
                                         ProviderConfigWidget *configWidget )
{
    QRadioButton *providerTypeButton = new QRadioButton;
    providerTypeButton->setText( prettyName );
    providerTypeButton->setIcon( icon );

    m_providerButtons.addButton( providerTypeButton );
    m_providerTypePage->widget()->layout()->addWidget( providerTypeButton );
    m_idForButton.insert( providerTypeButton, id );

    KPageWidgetItem *configPage =
            new KPageWidgetItem( configWidget, i18n( "Configure Target" ) );
    configPage->setParent( this );
    m_configForButton.insert( providerTypeButton, configPage );

    connect( providerTypeButton, SIGNAL(toggled(bool)),
             SLOT(providerButtonToggled(bool)) );

    if( !m_providerButtons.checkedButton() )
        providerTypeButton->setChecked( true );
}

void
CreateProviderDialog::providerTypeRemoved( QString id )
{
    //TODO:
    Q_UNUSED( id )
}

void
CreateProviderDialog::providerButtonToggled( bool checked )
{
    KPageWidgetItem *configPage = m_configForButton[sender()];
    checked ? addPage( configPage ) : removePage( configPage );
}

void
CreateProviderDialog::slotFinished()
{
    QAbstractButton *checkedButton = m_providerButtons.checkedButton();
    if( !checkedButton ) return;

    const QString id = m_idForButton[checkedButton];
    KPageWidgetItem *configPage = m_configForButton[checkedButton];
    const ProviderConfigWidget *configWidget =
            qobject_cast<ProviderConfigWidget*>( configPage->widget() );

    emit providerConfigured( id, configWidget->config() );
}

} // namespace StatSyncing
