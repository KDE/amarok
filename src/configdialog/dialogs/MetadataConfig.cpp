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

#include "MetadataConfig.h"

#include "amarokconfig.h"

MetadataConfig::MetadataConfig( QWidget *parent )
    : ConfigDialogBase( parent )
    , Ui_MetadataConfig() // seems needed or memory allocation is wrong (gcc bug?)
{
    connect( this, SIGNAL(changed()), parent, SLOT(updateButtons()) );

    setupUi( this );
    m_writeBackCoverDimensions->addItem(
        i18nc("Maximum cover size option", "Small (200 px)" ), QVariant( 200 ) );
    m_writeBackCoverDimensions->addItem(
        i18nc("Maximum cover size option", "Medium (400 px)" ), QVariant( 400 ) );
    m_writeBackCoverDimensions->addItem(
        i18nc("Maximum cover size option", "Large (800 px)" ), QVariant( 800 ) );
    m_writeBackCoverDimensions->addItem(
        i18nc("Maximum cover size option", "Huge (1600 px)" ), QVariant( 1600 ) );

    m_writeBack->setChecked( AmarokConfig::writeBack() );
    m_writeBack->setVisible( false ); // probably not a usecase
    m_writeBackStatistics->setChecked( AmarokConfig::writeBackStatistics() );
    m_writeBackStatistics->setEnabled( m_writeBack->isChecked() );
    m_writeBackCover->setChecked( AmarokConfig::writeBackCover() );
    m_writeBackCover->setEnabled( m_writeBack->isChecked() );
    if( m_writeBackCoverDimensions->findData( AmarokConfig::writeBackCoverDimensions() ) != -1 )
        m_writeBackCoverDimensions->setCurrentIndex( m_writeBackCoverDimensions->findData( AmarokConfig::writeBackCoverDimensions() ) );
    else
        m_writeBackCoverDimensions->setCurrentIndex( 1 ); // medium
    m_writeBackCoverDimensions->setEnabled( m_writeBackCover->isEnabled() && m_writeBackCover->isChecked() );
    m_useCharsetDetector->setChecked( AmarokConfig::useCharsetDetector() );

    connect( m_writeBack, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );
    connect( m_writeBackStatistics, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );
    connect( m_writeBackCover, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );
    connect( m_writeBackCoverDimensions, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()) );
    connect( m_useCharsetDetector, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );
}

MetadataConfig::~MetadataConfig()
{
}

bool MetadataConfig::isDefault()
{
    return false;
}

bool MetadataConfig::hasChanged()
{
    // a bit hacky, but updating enabled status here does the trick
    m_writeBackStatistics->setEnabled( m_writeBack->isChecked() );
    m_writeBackCover->setEnabled( m_writeBack->isChecked() );
    m_writeBackCoverDimensions->setEnabled( m_writeBackCover->isEnabled() && m_writeBackCover->isChecked() );

    return
        m_writeBack->isChecked() != AmarokConfig::writeBack() ||
        m_writeBackStatistics->isChecked() != AmarokConfig::writeBackStatistics() ||
        m_writeBackCover->isChecked() != AmarokConfig::writeBackCover() ||
        writeBackCoverDimensions() != AmarokConfig::writeBackCoverDimensions() ||
        m_useCharsetDetector->isChecked() != AmarokConfig::useCharsetDetector();
}

void MetadataConfig::updateSettings()
{
    AmarokConfig::setWriteBack( m_writeBack->isChecked() );
    AmarokConfig::setWriteBackStatistics( m_writeBackStatistics->isChecked() );
    AmarokConfig::setWriteBackCover( m_writeBackCover->isChecked() );
    if( writeBackCoverDimensions() > 0 )
        AmarokConfig::setWriteBackCoverDimensions( writeBackCoverDimensions() );
    AmarokConfig::setUseCharsetDetector( m_useCharsetDetector->isChecked() );
}

int MetadataConfig::writeBackCoverDimensions() const
{
    return m_writeBackCoverDimensions->itemData( m_writeBackCoverDimensions->currentIndex() ).toInt();
}
