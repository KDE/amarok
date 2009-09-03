/****************************************************************************************
 * Copyright (c) 2008 Aaron Seigo <aseigo@kde.org>                                      *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#include <iostream>

#include <QDir>
#include <QDBusInterface>

#include <KApplication>
#include <KAboutData>
#include <KAction>
#include <KCmdLineArgs>
#include <KLocale>
#include <KService>
#include <KServiceTypeTrader>
#include <KShell>
#include <KStandardDirs>
#include <KProcess>
#include <KSycoca>
#include <KConfigGroup>

#include <Plasma/PackageStructure>
#include <Plasma/Package>
#include <Plasma/PackageMetadata>

static const char description[] = I18N_NOOP("Install, list, remove Amarok applets");
static const char version[] = "0.1";


void output(const QString &msg)
{
    std::cout << msg.toLocal8Bit().constData() << std::endl;
}

void runKbuildsycoca()
{
    QDBusInterface dbus("org.kde.kded", "/kbuildsycoca", "org.kde.kbuildsycoca");
    dbus.call(QDBus::NoBlock, "recreate");
}

QStringList packages(const QString& type)
{
    QStringList result;
    KService::List services = KServiceTypeTrader::self()->query("Plasma/" + type, "'amarok' ~ [X-KDE-ParentApp]");
    foreach(const KService::Ptr &service, services) {
        result << service->property("X-KDE-PluginInfo-Name", QVariant::String).toString();
    }
    return result;
}

void listPackages(const QString& type)
{
    QStringList list = packages(type);
    list.sort();
    foreach(const QString& package, list) {
        output(package);
    }
}

int main(int argc, char **argv)
{
    KAboutData aboutData("amarokpkg", 0, ki18n("Amarok Applet Manager"),
                         version, ki18n(description), KAboutData::License_GPL,
                         ki18n("(C) 2008, Aaron Seigo, (C) 2009, Leo franchi"));
    aboutData.addAuthor( ki18n("Aaron Seigo"),
                         ki18n("Original author"),
                        "aseigo@kde.org" );
    aboutData.addAuthor( ki18n( "Leo Franchi" ),
                         ki18n( "Developer" ) ,
                         "lfranchi@kde.org"  );

    KComponentData componentData(aboutData);

    KCmdLineArgs::init( argc, argv, &aboutData );

    KCmdLineOptions options;
    options.add("g");
    options.add("global", ki18n("For install or remove, operates on applets installed for all users."));
    options.add("s");
    options.add("i");
    options.add("install <path>", ki18nc("Do not translate <path>", "Install the applet at <path>"));
    options.add("u");
    options.add("upgrade <path>", ki18nc("Do not translate <path>", "Upgrade the applet at <path>"));
    options.add("l");
    options.add("list", ki18n("List installed applet"));
    options.add("r");
    options.add("remove <name>", ki18nc("Do not translate <name>", "Remove the applet named <name>"));
    options.add("p");
    options.add("packageroot <path>", ki18n("Absolute path to the package root. If not supplied, then the standard data directories for this KDE session will be searched instead."));
    KCmdLineArgs::addCmdLineOptions( options );

    KApplication app;

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    QString packageRoot = "plasma/plasmoids/";
    QString servicePrefix = "amarok-applet-";
    QString pluginType = "Applet";
    Plasma::PackageStructure *installer = 0;

    if (args->isSet("list")) {
        listPackages(pluginType);
    } else {
        // install, remove or upgrade
        if (!installer) {
            installer = new Plasma::PackageStructure();
            installer->setServicePrefix(servicePrefix);
        }

        if (args->isSet("packageroot")) {
            packageRoot = args->getOption("packageroot");
        } else if (args->isSet("global")) {
            packageRoot = KStandardDirs::locate("data", packageRoot);
        } else {
            packageRoot = KStandardDirs::locateLocal("data", packageRoot);
        }

        QString package;
        QString packageFile;
        if (args->isSet("remove")) {
            package = args->getOption("remove");
        } else if (args->isSet("upgrade")) {
            package = args->getOption("upgrade");
        } else if (args->isSet("install")) {
            package = args->getOption("install");
        }
        if (!QDir::isAbsolutePath(package)) {
            packageFile = QDir(QDir::currentPath() + '/' + package).absolutePath();
        } else {
            packageFile = package;
        }

        if (args->isSet("remove") || args->isSet("upgrade")) {
            installer->setPath(packageFile);
            Plasma::PackageMetadata metadata = installer->metadata();

            QString pluginName;
            if (metadata.pluginName().isEmpty()) {
                // plugin name given in command line
                pluginName = package;
            } else {
                // Parameter was a plasma package, get plugin name from the package
                pluginName = metadata.pluginName();
            }

            QStringList installed = packages(pluginType);
            if (installed.contains(pluginName)) {
                if (installer->uninstallPackage(pluginName, packageRoot)) {
                    output(i18n("Successfully removed %1", pluginName));
                } else if (!args->isSet("upgrade")) {
                    output(i18n("Removal of %1 failed.", pluginName));
                    delete installer;
                    return 1;
                }
            } else {
                output(i18n("Plugin %1 is not installed.", pluginName));
            }
        }
        if (args->isSet("install") || args->isSet("upgrade")) {
            if (installer->installPackage(packageFile, packageRoot)) {
                output(i18n("Successfully installed %1", packageFile));
                runKbuildsycoca();
            } else {
                output(i18n("Installation of %1 failed.", packageFile));
                delete installer;
                return 1;
            }
        }
        if (package.isEmpty()) {
            KCmdLineArgs::usageError(i18nc("No option was given, this is the error message telling the user he needs at least one, do not translate install, remove, upgrade nor list", "One of install, remove, upgrade or list is required."));
        } else {
            runKbuildsycoca();
        }
    }
    delete installer;
    return 0;
}



