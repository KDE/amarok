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

#include <QApplication>
#include <QAction>
#include <QCommandLineParser>
#include <QDBusInterface>
#include <QDir>
#include <QLocale>
#include <QTextStream>

#include <KAboutData>
#include <klocalizedstring.h>
#include <KService>
#include <KServiceTypeTrader>
#include <KShell>
#include <QStandardPaths>
#include <KProcess>
#include <KSycoca>
#include <KConfigGroup>
#include <KPackage/Package>
#include <KPluginInfo>


static const char description[] = "Install, list, remove Amarok applets";
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
    KAboutData aboutData("amarokpkg", i18n("Amarok Applet Manager"),
                         version, i18n(description), KAboutLicense::GPL,
                         i18n("(C) 2008, Aaron Seigo, (C) 2009, Leo Franchi"));
    aboutData.addAuthor( i18n("Aaron Seigo"),
                         i18n("Original author"),
                        "aseigo@kde.org" );
    aboutData.addAuthor( i18n( "Leo Franchi" ),
                         i18n( "Developer" ) ,
                         "lfranchi@kde.org"  );

    QApplication app(argc, argv);
    app.setApplicationName("amarokpkg");
    app.setOrganizationDomain("kde.org");
    app.setApplicationDisplayName(i18n("Amarok Applet Manager"));
    app.setApplicationVersion(version);

    /**
     * @TODO: DO WE NEED THIS ?
     */
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;

    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    parser.addOption(QCommandLineOption(QStringList() << "g" << "global",
                                        i18n("For install or remove, operates on applets installed for all users.")));
    parser.addOption(QCommandLineOption(QStringList() << "s" << "i" << "install <path>",
                                        i18nc("Do not translate <path>", "Install the applet at <path>")));
    parser.addOption(QCommandLineOption(QStringList() << "u" << "upgrade <path>",
                                        i18nc("Do not translate <path>", "Upgrade the applet at <path>")));
    parser.addOption(QCommandLineOption(QStringList() << "l" << "list",
                                        i18n("Most installed applets")));
    parser.addOption(QCommandLineOption(QStringList() << "r" << "remove <name>",
                                        i18nc("Do not translate <name>", "Remove the applet named <name>")));
    parser.addOption(QCommandLineOption(QStringList() << "p" << "packageroot <path>",
                                        i18n("Absolute path to the package root. If not supplied, then the standard data directories for this KDE session will be searched instead.")));

    QString packageRoot = "plasma/plasmoids/";
    QString servicePrefix = "amarok-applet-";
    QString pluginType = "Applet";
    KPackage::Package *installer = 0;

    if (parser.isSet("list")) {
        listPackages(pluginType);
    } else {
        // install, remove or upgrade
        if (!installer) {
            installer = new KPackage::Package();
            installer->setServicePrefixPaths(servicePrefix);
        }

        if (parser.isSet("packageroot")) {
            packageRoot = parser.value("packageroot");
        } else if (parser.isSet("global")) {
            packageRoot = QStandardPaths::locate(QStandardPaths::GenericDataLocation, packageRoot);
        } else {
            packageRoot = KStandardDirs::locateLocal("data", packageRoot);
        }

        QString package;
        QString packageFile;
        if (parser.isSet("remove")) {
            package = parser.value("remove");
        } else if (parser.isSet("upgrade")) {
            package = parser.value("upgrade");
        } else if (parser.isSet("install")) {
            package = parser.value("install");
        }
        if (!QDir::isAbsolutePath(package)) {
            packageFile = QDir(QDir::currentPath() + '/' + package).absolutePath();
        } else {
            packageFile = package;
        }

        if (parser.isSet("remove") || parser.isSet("upgrade")) {
            installer->setPath(packageFile);
            KPluginMetaData metadata = installer->metadata();

            QString pluginName;
            if (metadata.name().isEmpty()) {
                // plugin name given in command line
                pluginName = package;
            } else {
                // Parameter was a plasma package, get plugin name from the package
                pluginName = metadata.name();
            }

            QStringList installed = packages(pluginType);
            if (installed.contains(pluginName)) {
                if (installer->uninstall(pluginName, packageRoot)) {
                    output(i18n("Successfully removed %1", pluginName));
                } else if (!parser.isSet("upgrade")) {
                    output(i18n("Removal of %1 failed.", pluginName));
                    delete installer;
                    return 1;
                }
            } else {
                output(i18n("Plugin %1 is not installed.", pluginName));
            }
        }
        if (parser.isSet("install") || parser.isSet("upgrade")) {
            if (installer->install(packageFile, packageRoot)) {
                output(i18n("Successfully installed %1", packageFile));
                runKbuildsycoca();
            } else {
                output(i18n("Installation of %1 failed.", packageFile));
                delete installer;
                return 1;
            }
        }

        if (package.isEmpty()) {
            QTextStream out(stdout);
            out << i18nc("No option was given, this is the error message telling the user he needs at least one, do not translate install, remove, upgrade nor list", "One of install, remove, upgrade or list is required.");
        } else {
            runKbuildsycoca();
        }
    }
    delete installer;
    return 0;
}



