/***************************************************************************
 *   Copyright (C) 2004-2007 by Mark Kretschmann <markey@web.de>           *
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

#include "EngineConfig.h"
#include "amarokconfig.h"
#include "enginecontroller.h"
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"

#include <Q3GroupBox>
#include <QComboBox>
#include <QPushButton>

#include <KDialog>
#include <KLocale>
#include <KVBox>


EngineConfig::EngineConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    KVBox* mainWidget = new KVBox( this );
    mainWidget->setSpacing( KDialog::spacingHint() );
    QWidget *groupBox, *aboutEngineButton;
    groupBox            = new Q3GroupBox( 2, Qt::Horizontal, i18n("Sound System"), mainWidget );
    m_engineConfigFrame = new Q3GroupBox( 1, Qt::Horizontal, mainWidget );
    m_soundSystem       = new QComboBox( groupBox );
    aboutEngineButton   = new QPushButton( i18n("About"), groupBox );

    m_soundSystem->setToolTip( i18n("Click to select the sound system to use for playback.") );
    aboutEngineButton->setToolTip( i18n("Click to get the plugin information.") );

    /// Populate the engine selection combo box
    KService::List offers = PluginManager::query( "[X-KDE-Amarok-plugintype] == 'engine'" );
    KService::List::ConstIterator end( offers.end() );
    for( KService::List::ConstIterator it = offers.begin(); it != end; ++it ) {
        // Don't list the <no engine> (void engine) entry if it's not currently active,
        // cause there's no point in choosing this engine (it's a dummy, after all).
        if( (*it)->property( "X-KDE-Amarok-name" ).toString() == "void-engine"
            && AmarokConfig::soundSystem() != "void-engine" ) continue;

        m_soundSystem->addItem( (*it)->name() );
        // Save name properties in QMap for lookup
        m_pluginName[(*it)->name()] = (*it)->property( "X-KDE-Amarok-name" ).toString();
        m_pluginAmarokName[(*it)->property( "X-KDE-Amarok-name" ).toString()] = (*it)->name();
    }

    connect( aboutEngineButton, SIGNAL( clicked() ), SLOT( aboutEngine() ) );
    connect( m_soundSystem, SIGNAL( activated( int ) ), parent, SLOT( updateButtons() ) );
}

EngineConfig::~EngineConfig()
{}


///////////////////////////////////////////////////////////////
// REIMPLEMENTED METHODS from ConfigDialogBase
///////////////////////////////////////////////////////////////

bool
EngineConfig::hasChanged()
{
    return m_soundSystem->currentText() != m_pluginAmarokName[AmarokConfig::soundSystem()];
}

bool
EngineConfig::isDefault()
{
    return false;
}

void
EngineConfig::updateSettings()
{
    // When sound system has changed, update engine config page
    if ( m_soundSystem->currentText() != m_pluginAmarokName[AmarokConfig::soundSystem()] ) {
        AmarokConfig::setSoundSystem( m_pluginName[m_soundSystem->currentText()] );
        emit settingsChanged( parent()->objectName() );
        soundSystemChanged();
    }
}

void
EngineConfig::updateWidgets()
{
    int current = m_soundSystem->findText( m_pluginAmarokName[AmarokConfig::soundSystem()] );
    m_soundSystem->setCurrentIndex( current );
    soundSystemChanged();
}

void
EngineConfig::updateWidgetsDefault()
{
    m_soundSystem->setCurrentIndex( 0 );
}


///////////////////////////////////////////////////////////////
// PRIVATE METHODS 
///////////////////////////////////////////////////////////////

void
EngineConfig::aboutEngine() //SLOT
{
    PluginManager::showAbout( QString( "Name == '%1'" ).arg( m_soundSystem->currentText() ) );
}

void
EngineConfig::soundSystemChanged()
{
    ///A new sound system has been LOADED
    ///If only the sound system widget has been changed don't call this!

    // remove old engine config widget
    // will delete the view if implementation is done correctly
    delete m_engineConfig;

    if( EngineController::hasEngineProperty( "HasConfigure" ) )
    {
        m_engineConfig = EngineController::engine()->configure();
        m_engineConfig->view()->setParent( m_engineConfigFrame );
        m_engineConfig->view()->show();
        m_engineConfigFrame->setTitle( i18nc( "to change settings", "Configure %1", m_soundSystem->currentText() ) );
        m_engineConfigFrame->show();

        connect( m_engineConfig, SIGNAL(viewChanged()), SLOT(updateButtons()) );
    }
    else {
        m_engineConfig = 0;
        m_engineConfigFrame->hide();
    }

    // FIXME What to do about this interdependency with the Playback tab?
#if 0
    const bool hasCrossfade = EngineController::hasEngineProperty( "HasCrossfade" );
    const bool crossfadeOn = m_opt4->kcfg_Crossfade->isChecked();
    // Enable crossfading option when available
    m_opt4->kcfg_Crossfade->setEnabled( hasCrossfade );
    m_opt4->kcfg_CrossfadeLength->setEnabled( hasCrossfade && crossfadeOn );
    m_opt4->crossfadeLengthLabel->setEnabled( hasCrossfade && crossfadeOn );
    m_opt4->kcfg_CrossfadeType->setEnabled( hasCrossfade && crossfadeOn );

    if (!hasCrossfade)
    {
        m_opt4->radioButtonNormalPlayback->setChecked( true );
    }
#endif
}


#include "EngineConfig.moc"


