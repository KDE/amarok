/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "ChooseProvidersPage.h"

#include "core/meta/support/MetaConstants.h"
#include "core/support/Debug.h"
#include "statsyncing/models/ProvidersModel.h"

#include <KPushButton>

#include <QCheckBox>

using namespace StatSyncing;

ChooseProvidersPage::ChooseProvidersPage( QWidget *parent, Qt::WindowFlags f )
    : QWidget( parent, f )
    , m_providersModel( 0 )
{
    DEBUG_BLOCK
    setupUi( this );
    KPushButton *save = buttonBox->addButton( KGuiItem( i18n( "Save Settings and Close" ) ),
                                              QDialogButtonBox::ActionRole );
    buttonBox->addButton( KGuiItem( i18n( "Next" ), "go-next" ), QDialogButtonBox::AcceptRole );
    connect( save, SIGNAL(clicked(bool)), SIGNAL(saveSettings()) );
    connect( buttonBox, SIGNAL(accepted()), SIGNAL(accepted()) );
    connect( buttonBox, SIGNAL(rejected()), SIGNAL(rejected()) );
}

ChooseProvidersPage::~ChooseProvidersPage()
{
}

void
ChooseProvidersPage::setFields( const QList<qint64> &fields, qint64 checkedFields )
{
    QLayout *fieldsLayout = fieldsBox->layout();
    foreach( qint64 field, fields )
    {
        QString name = Meta::i18nForField( field );
        QCheckBox *checkBox = new QCheckBox( name );
        fieldsLayout->addWidget( checkBox );
        checkBox->setCheckState( ( field & checkedFields ) ? Qt::Checked : Qt::Unchecked );
        checkBox->setStyleSheet( "margin-right: 0.5em" );
        checkBox->setProperty( "field", field );
        connect( checkBox, SIGNAL(stateChanged(int)), SIGNAL(checkedFieldsChanged()) );
    }
    fieldsLayout->addItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding ) );

    connect( this, SIGNAL(checkedFieldsChanged()), SLOT(updateSynchronizedLabel()) );
    updateSynchronizedLabel();
}

qint64
ChooseProvidersPage::checkedFields() const
{
    qint64 ret = 0;
    QLayout *fieldsLayout = fieldsBox->layout();
    for( int i = 0; i < fieldsLayout->count(); i++ )
    {
        QCheckBox *checkBox = qobject_cast<QCheckBox *>( fieldsLayout->itemAt( i )->widget() );
        if( !checkBox )
            continue;
        if( checkBox->isChecked() && checkBox->property( "field" ).canConvert<qint64>() )
            ret |= checkBox->property( "field" ).value<qint64>();
    }
    return ret;
}

void
ChooseProvidersPage::setProvidersModel( ProvidersModel *model, QItemSelectionModel *selectionModel )
{
    m_providersModel = model;
    providersView->setModel( model );
    providersView->setSelectionModel( selectionModel );

    connect( model, SIGNAL(selectedProvidersChanged()), SLOT(updateMatchedLabel()) );
    connect( model, SIGNAL(selectedProvidersChanged()), SLOT(updateSynchronizedLabel()) );
    updateMatchedLabel();
    updateSynchronizedLabel();
}

void
ChooseProvidersPage::disableControls()
{
    // disable checkboxes
    QLayout *fieldsLayout = fieldsBox->layout();
    for( int i = 0; i < fieldsLayout->count(); i++ )
    {
        QWidget *widget = fieldsLayout->itemAt( i )->widget();
        if( widget )
            widget->setEnabled( false );
    }

    // disable view
    providersView->setEnabled( false );

    // disable Next buton
    foreach( QAbstractButton *button, buttonBox->buttons() )
    {
        if( buttonBox->buttonRole( button ) == QDialogButtonBox::AcceptRole )
            button->setEnabled( false );
    }
}

void
ChooseProvidersPage::setProgressBarText( const QString &text )
{
    progressBar->setFormat( text );
}

void
ChooseProvidersPage::setProgressBarMaximum( int maximum )
{
    progressBar->setMaximum( maximum );
}

void
ChooseProvidersPage::progressBarIncrementProgress()
{
    progressBar->setValue( progressBar->value() + 1 );
}

void
ChooseProvidersPage::updateMatchedLabel()
{
    qint64 fields = m_providersModel->reliableTrackMetadataIntersection();
    QString fieldNames = m_providersModel->fieldsToString( fields );
    matchLabel->setText( i18n( "Tracks matched by: %1", fieldNames ) );
}

void
ChooseProvidersPage::updateSynchronizedLabel()
{
    if( !m_providersModel )
        return;

    qint64 fields = checkedFields();
    // TODO: enable/disable save prefs and close button based on fields variable now
    fields &= m_providersModel->writableTrackStatsDataIntersection();
    QString fieldNames = m_providersModel->fieldsToString( fields );
    synchronizedLabel->setText( i18n( "Synchronized: %1", fieldNames ) );
    QAbstractButton *nextButton = 0;
    foreach( QAbstractButton *button, buttonBox->buttons() )
    {
        if( buttonBox->buttonRole( button ) == QDialogButtonBox::AcceptRole )
            nextButton = button;
    }
    if( nextButton )
        nextButton->setEnabled( fields != 0 );
}
