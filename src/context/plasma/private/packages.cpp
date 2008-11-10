/******************************************************************************
*   Copyright 2007 by Aaron Seigo <aseigo@kde.org>                        *
*                                                                             *
*   This library is free software; you can redistribute it and/or             *
*   modify it under the terms of the GNU Library General Public               *
*   License as published by the Free Software Foundation; either              *
*   version 2 of the License, or (at your option) any later version.          *
*                                                                             *
*   This library is distributed in the hope that it will be useful,           *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU          *
*   Library General Public License for more details.                          *
*                                                                             *
*   You should have received a copy of the GNU Library General Public License *
*   along with this library; see the file COPYING.LIB.  If not, write to      *
*   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
*   Boston, MA 02110-1301, USA.                                               *
*******************************************************************************/

#include "private/packages_p.h"

#include <KConfigGroup>
#include <KDesktopFile>
#include <KLocale>
#include <KMessageBox>

#include <knewstuff2/engine.h>

namespace Plasma
{

PlasmoidPackage::PlasmoidPackage(QObject *parent)
    : Plasma::PackageStructure(parent, QString("Plasmoid"))
{
    addDirectoryDefinition("images", "images", i18n("Images"));
    QStringList mimetypes;
    mimetypes << "image/svg+xml" << "image/png" << "image/jpeg";
    setMimetypes("images", mimetypes);

    addDirectoryDefinition("config", "config/", i18n("Configuration Definitions"));
    mimetypes.clear();
    mimetypes << "text/xml";
    setMimetypes("config", mimetypes);
    setMimetypes("configui", mimetypes);

    addDirectoryDefinition("ui", "ui", i18n("Executable Scripts"));
    setMimetypes("ui", mimetypes);

    addDirectoryDefinition("scripts", "code", i18n("Executable Scripts"));
    mimetypes.clear();
    mimetypes << "text/*";
    setMimetypes("scripts", mimetypes);

    addFileDefinition("mainconfigui", "ui/config.ui", i18n("Main Config UI File"));
    addFileDefinition("mainconfigxml", "config/main.xml", i18n("Configuration XML file"));
    addFileDefinition("mainscript", "code/main", i18n("Main Script File"));
    setRequired("mainscript", true);
}

void PlasmoidPackage::pathChanged()
{
    KDesktopFile config(path() + "/metadata.desktop");
    KConfigGroup cg = config.desktopGroup();
    QString mainScript = cg.readEntry("X-Plasma-MainScript", QString());
    if (!mainScript.isEmpty()) {
        addFileDefinition("mainscript", mainScript, i18n("Main Script File"));
        setRequired("mainscript", true);
    }
}

void PlasmoidPackage::createNewWidgetBrowser(QWidget *parent)
{
    KNS::Engine engine(0);
    if (engine.init("plasmoids.knsrc")) {
        KNS::Entry::List entries = engine.downloadDialogModal(parent);

        foreach (KNS::Entry *entry, entries) {
            if (entry->status() != KNS::Entry::Installed) {
                continue;
            }

            // install the packges!
            foreach (const QString &package, entry->installedFiles()) {
                if (!installPackage(package, defaultPackageRoot())) {
                    kDebug() << "FAIL! on install of" << package;
                    KMessageBox::error(0, i18n("Installation of <b>%1</b> failed!", package),
                                       i18n("Installation Failed"));
                }
            }
        }

        qDeleteAll(entries);
    }
    emit newWidgetBrowserFinished();
}

ThemePackage::ThemePackage(QObject *parent)
    : Plasma::PackageStructure(parent, QString("Plasma Theme"))
{
    addDirectoryDefinition("dialogs", "dialogs/", i18n("Images for dialogs"));
    addFileDefinition("dialogs/background", "dialogs/background.svg",
                      i18n("Generic dialog background"));
    addFileDefinition("dialogs/shutdowndialog", "dialogs/shutdowndialog.svg",
                      i18n("Theme for the logout dialog"));

    addDirectoryDefinition("wallpapers", "wallpapers/", i18n("Wallpaper packages"));

    addDirectoryDefinition("widgets", "widgets/", i18n("Images for widgets"));
    addFileDefinition("widgets/background", "widgets/background.svg",
                      i18n("Background image for plasmoids"));
    addFileDefinition("widgets/clock", "widgets/clock.svg",
                      i18n("Analog clock face"));
    addFileDefinition("widgets/panel-background", "widgets/panel-background.svg",
                      i18n("Background image for panels"));
    addFileDefinition("widgets/plot-background", "widgets/plot-background.svg",
                      i18n("Background for graphing widgets"));
    addFileDefinition("widgets/tooltip", "widgets/tooltip.svg",
                      i18n("Background image for tooltips"));

    addDirectoryDefinition("opaque/dialogs", "opaque/dialogs/", i18n("Opaque images for dialogs"));
    addFileDefinition("opaque/dialogs/background", "opaque/dialogs/background.svg",
                      i18n("Opaque generic dialog background"));
    addFileDefinition("opaque/dialogs/shutdowndialog", "opaque/dialogs/shutdowndialog.svg",
                      i18n("Opaque theme for the logout dialog"));

    addDirectoryDefinition("opaque/widgets", "opaque/widgets/", i18n("Opaque images for widgets"));
    addFileDefinition("opaque/widgets/panel-background", "opaque/widgets/panel-background.svg",
                      i18n("Opaque background image for panels"));
    addFileDefinition("opaque/widgets/tooltip", "opaque/widgets/tooltip.svg",
                      i18n("Opaque background image for tooltips"));

    addDirectoryDefinition("locolor/dialogs", "locolor/dialogs/",
                           i18n("Low color images for dialogs"));
    addFileDefinition("locolor/dialogs/background", "locolor/dialogs/background.svg",
                      i18n("Low color generic dialog background"));
    addFileDefinition("locolor/dialogs/shutdowndialog", "locolor/dialogs/shutdowndialog.svg",
                      i18n("Low color theme for the logout dialog"));

    addDirectoryDefinition("locolor/widgets", "locolor/widgets/", i18n("Images for widgets"));
    addFileDefinition("locolor/widgets/background", "locolor/widgets/background.svg",
                      i18n("Low color background image for plasmoids"));
    addFileDefinition("locolor/widgets/clock", "locolor/widgets/clock.svg",
                      i18n("Low color analog clock face"));
    addFileDefinition("locolor/widgets/panel-background", "locolor/widgets/panel-background.svg",
                      i18n("Low color background image for panels"));
    addFileDefinition("locolor/widgets/plot-background", "locolor/widgets/plot-background.svg",
                      i18n("Low color background for graphing widgets"));
    addFileDefinition("locolor/widgets/tooltip", "locolor/widgets/tooltip.svg",
                      i18n("Low color background image for tooltips"));

    addFileDefinition("colors", "colors", i18n("KColorScheme configuration file"));

    QStringList mimetypes;
    mimetypes << "image/svg+xml";
    setDefaultMimetypes(mimetypes);
}

} // namespace Plasma

#include "packages_p.moc"

