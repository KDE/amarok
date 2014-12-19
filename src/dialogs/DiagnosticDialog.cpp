/****************************************************************************************
 * Copyright (c) 2012 Andrzej J. R. Hunt <andrzej at ahunt.org>                         *
 * Copyright (c) Mark Kretschmann <kretschmann@kde.org>                                 *
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

#include "DiagnosticDialog.h"

#include "context/ContextView.h"
#include "PluginManager.h"
#include "scripting/scriptmanager/ScriptManager.h"

#include <KAboutData>
#include <KApplication>
#include <KGlobal>
#include <KPluginInfo>
#include <KService>
#include <KServiceTypeTrader>
#include <Plasma/Applet>
#include <phonon/pulsesupport.h>

#include <QClipboard>
#include <QPlainTextEdit>


DiagnosticDialog::DiagnosticDialog( const KAboutData *aboutData, QWidget *parent )
        : KDialog( parent )
{
    if( aboutData == 0 )
        aboutData = KGlobal::mainComponent().aboutData();

    m_textBox = new QPlainTextEdit( generateReport( aboutData ), this );

    setPlainCaption( i18nc( "%1 is the program name", "%1 Diagnostics", aboutData->programName() ) );

    setButtons( Close | User1 );
    setButtonText( User1, i18n( "Copy to Clipboard" ) );

    m_textBox->setReadOnly( true );

    setMainWidget( m_textBox );
    setInitialSize( QSize( 480, 460 ) );

    connect( this, SIGNAL(user1Clicked()), SLOT(slotCopyToClipboard()) );
    connect( this, SIGNAL(finished()), SLOT(deleteLater()) );
}

const QString
DiagnosticDialog::generateReport( const KAboutData *aboutData )
{
    // Get scripts -- we have to assemble 3 lists into one
    KPluginInfo::List aScripts;
    const ScriptManager *aScriptManager = ScriptManager::instance();
    aScripts.append( aScriptManager->scripts( QLatin1String( "Generic" ) ) );
    aScripts.append( aScriptManager->scripts( QLatin1String( "Lyrics" ) ) );
    aScripts.append( aScriptManager->scripts( QLatin1String( "Scriptable Service" ) ) );

    // Format the data to be readable
    QString aScriptString;
    foreach( KPluginInfo aInfo, aScripts )
    {
        if( aInfo.isPluginEnabled() )
            aScriptString += "   " + aInfo.name() + " (" + aInfo.version() + ")\n";
    }


    // Get plugins -- we have to assemble a list again.
    KPluginInfo::List aPlugins;
    const Plugins::PluginManager *aPluginManager = Plugins::PluginManager::instance();
    aPlugins.append( aPluginManager->plugins( Plugins::PluginManager::Collection ) );
    aPlugins.append( aPluginManager->plugins( Plugins::PluginManager::Service ) );
    aPlugins.append( aPluginManager->plugins( Plugins::PluginManager::Importer ) );

    QString aPluginString;
    foreach( KPluginInfo aInfo, aPlugins )
    {
        if( aInfo.isPluginEnabled() )
            aPluginString += "   " + aInfo.name() + " (" + aInfo.version() + ")\n";
    }


    // Get applets
    const QStringList appletList = Context::ContextView::self()->currentAppletNames();
    QString appletString;

    foreach( const QString &applet, appletList )
    {
        // Currently we cannot extract the applet version number this way
        appletString += "   " + applet + '\n';
    }


    const KService::Ptr aPhononBackend =
        KServiceTypeTrader::self()->preferredService( "PhononBackend" );

    const bool hasPulse = Phonon::PulseSupport::getInstance()->isActive();
    const QString pulse = hasPulse ? i18nc( "Usage", "Yes" ) : i18nc( "Usage", "No" );

    return i18n(
               "%1 Diagnostics\n\nGeneral Information:\n"
               "   %1 Version: %2\n"
               "   KDE Version: %3\n"
               "   Qt Version: %4\n"
               "   Phonon Version: %5\n"
               "   Phonon Backend: %6 (%7)\n"
               "   PulseAudio: %8\n\n",

               aboutData->programName(), aboutData->version(),      // Amarok
               KDE::versionString(),                                // KDE
               qVersion(),                                          // QT
               Phonon::phononVersion(),                             // Phonon
               aPhononBackend.data()->name(),
               aPhononBackend.data()->property( "X-KDE-PhononBackendInfo-Version", QVariant::String ).toString(), // & Backend
               pulse                                                // PulseAudio
           ) + i18n(
               "Enabled Scripts:\n%1\n"
               "Enabled Plugins:\n%2\n"
               "Enabled Applets:\n%3\n",
               aScriptString, aPluginString, appletString
           );
}

void
DiagnosticDialog::slotCopyToClipboard() const
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText( m_textBox->toPlainText() );
}
