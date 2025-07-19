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
#include "core/support/Amarok.h"
#include "PluginManager.h"
#include "scripting/scriptmanager/ScriptManager.h"

#include <KAboutData>
#include <KCoreAddons>
#include <KLocalizedString>
#include <KPluginMetaData>

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QDir>
#include <QPlainTextEdit>
#include <QPluginLoader>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include <algorithm>
#include <gst/gst.h>


DiagnosticDialog::DiagnosticDialog( const KAboutData &about, QWidget *parent )
    : QDialog( parent )
{
    setLayout( new QVBoxLayout );
    m_textBox = new QPlainTextEdit( generateReport( &about ), this );
    m_textBox->setReadOnly( true );
    layout()->addWidget( m_textBox );

    auto buttonBox = new QDialogButtonBox( this );
    auto closeButton = buttonBox->addButton( QDialogButtonBox::Close );
    auto copyButton = new QPushButton( i18n( "Copy to Clipboard" ) );
    buttonBox->addButton( copyButton, QDialogButtonBox::ActionRole );
    layout()->addWidget( buttonBox );

    setWindowTitle( i18nc( "%1 is the program name", "%1 Diagnostics", KAboutData::applicationData().displayName() ) );

    resize( QSize( 480, 460 ) );

    connect( closeButton, &QPushButton::clicked, this, &DiagnosticDialog::close );
    connect( copyButton, &QPushButton::clicked, this, &DiagnosticDialog::slotCopyToClipboard );

    setAttribute( Qt::WA_DeleteOnClose );
}

const QString
DiagnosticDialog::generateReport( const KAboutData *aboutData )
{
    // Get scripts -- we have to assemble 3 lists into one
    QVector<KPluginMetaData> aScripts;
    const auto aScriptManager = ScriptManager::instance();
    aScripts.append( aScriptManager->scripts( QStringLiteral( "Generic" ) ) );
    aScripts.append( aScriptManager->scripts( QStringLiteral( "Lyrics" ) ) );
    aScripts.append( aScriptManager->scripts( QStringLiteral( "Scriptable Service" ) ) );

    // Format the data to be readable
    QString aScriptString;
    for( KPluginMetaData aInfo : aScripts )
    {
        if( aInfo.isEnabled( Amarok::config( QStringLiteral("Scripts") ) ) )
            aScriptString += QStringLiteral("   ") + aInfo.name() + QStringLiteral(" (") + aInfo.version() + QStringLiteral(")\n");
    }


    // Get plugins -- we have to assemble a list again.
    QVector<KPluginMetaData> aPlugins;
    const auto aPluginManager = Plugins::PluginManager::instance();
    aPlugins.append( aPluginManager->enabledPlugins( Plugins::PluginManager::Collection ) );
    aPlugins.append( aPluginManager->enabledPlugins( Plugins::PluginManager::Service ) );
    aPlugins.append( aPluginManager->enabledPlugins( Plugins::PluginManager::Importer ) );

    QString aPluginString;
    for( const auto &aInfo : std::as_const(aPlugins) )
    {
        aPluginString += QStringLiteral("   ") + aInfo.name() + QStringLiteral(" (") + aInfo.version() + QStringLiteral(")\n");
    }

    // Get applets
    QString appletString;
    const QStringList appletList = Context::ContextView::self()->currentAppletNames();

    for ( const QString &applet : appletList )
    {
        // Currently we cannot extract the applet version number this way
        appletString += QStringLiteral("   ") + applet + QLatin1Char('\n');
    }

    const bool hasPulse = false;
    const QString pulse = hasPulse ? i18nc( "Usage", "Yes" ) : i18nc( "Usage", "No" );

    return i18n(
               "%1 Diagnostics\n\nGeneral Information:\n"
               "   %1 Version: %2\n"
               "   KDE Frameworks Version: %3\n"
               "   Qt Version: %4\n"
               "   GStreamer: %5\n\n",

               KAboutData::applicationData().displayName(), aboutData->version(),      // Amarok
               KCoreAddons::versionString(),                        // KDE Frameworks
               QLatin1String(qVersion()),                           // Qt
               QLatin1String(gst_version_string())
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
