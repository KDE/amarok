/******************************************************************************
*   Copyright 2007 by Bertjan Broeksema <b.broeksema@kdemail.net>             *
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

#include "plasmoidpackagetest.h"

#include <QDir>
#include <QFile>
#include <KZip>

#include "plasma/applet.h"
#include "plasma/packagemetadata.h"

void PlasmoidPackageTest::init()
{
    mPackage = QString("Package");
    mPackageRoot = QDir::homePath() + "/.kde-unit-test/packageRoot";
    ps = Plasma::Applet::packageStructure();
}

void PlasmoidPackageTest::cleanup()
{
    if (p) {
        delete p;
        p = 0;
    }

    // Clean things up.
    QDir local = QDir::homePath() + QLatin1String("/.kde-unit-test/packageRoot");
    foreach(const QString &dir, local.entryList(QDir::Dirs)) {
        removeDir(QLatin1String("packageRoot/" + dir.toLatin1() + "/contents/code"));
        removeDir(QLatin1String("packageRoot/" + dir.toLatin1() + "/contents/images"));
        removeDir(QLatin1String("packageRoot/" + dir.toLatin1() + "/contents"));
        removeDir(QLatin1String("packageRoot/" + dir.toLatin1()));
    }

    QDir().rmpath(QDir::homePath() + "/.kde-unit-test/packageRoot");
}

// Copied from ktimezonetest.h
void PlasmoidPackageTest::removeDir(const QString &subdir)
{
    QDir local = QDir::homePath() + QLatin1String("/.kde-unit-test/") + subdir;
    foreach(const QString &file, local.entryList(QDir::Files))
        if(!local.remove(file))
            qWarning("%s: removing failed", qPrintable( file ));
    QCOMPARE((int)local.entryList(QDir::Files).count(), 0);
    local.cdUp();
    QString subd = subdir;
    subd.remove(QRegExp("^.*/"));
    local.rmpath(subd);
}

void PlasmoidPackageTest::createTestPackage(const QString &packageName)
{
    QDir pRoot(mPackageRoot);
    // Create the root and package dir.
    if(!pRoot.exists())
    {
        QVERIFY(QDir().mkpath(mPackageRoot));
    }

    // Create the package dir
    QVERIFY(QDir().mkpath(mPackageRoot + "/" + packageName));

    // Create the metadata.desktop file
    QFile file(mPackageRoot + "/" + packageName + "/metadata.desktop");

    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

    QTextStream out(&file);
    out << "[Desktop Entry]\n";
    out << "Name=" << packageName << "\n";
    out << "X-KDE-PluginInfo-Name=" << packageName << "\n";
    file.flush();
    file.close();

    // Create the code dir.
    QVERIFY(QDir().mkpath(mPackageRoot + "/" + packageName + "/contents/code"));

    // Create the main file.
    file.setFileName(mPackageRoot + "/" + packageName + "/contents/code/main");
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

    out << "THIS IS A PLASMOID SCRIPT.....";
    file.flush();
    file.close();

    // Now we have a minimal plasmoid package which is valid. Let's add some 
    // files to it for test purposes.

    // Create the images dir.
    QVERIFY(QDir().mkpath(mPackageRoot + "/" + packageName + "/contents/images"));
    file.setFileName(mPackageRoot + "/" + packageName + "/contents/images/image-1.svg");

    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

    out << "<svg>This is a test image</svg>";
    file.flush();
    file.close();

    file.setFileName(mPackageRoot + "/" + packageName + "/contents/images/image-2.svg");

    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

    out.setDevice(&file);
    out << "<svg>This is another test image</svg>";
    file.flush();
    file.close();
}

void PlasmoidPackageTest::isValid()
{
    p = new Plasma::Package(mPackageRoot, mPackage, ps);

    // A PlasmoidPackage is valid when:
    // - The package root exists.
    // - The package root consists an file named "code/main"
    QVERIFY(!p->isValid());

    // Create the root and package dir.
    QVERIFY(QDir().mkpath(mPackageRoot));
    QVERIFY(QDir().mkpath(mPackageRoot + "/" + mPackage));

    // Should still be invalid.
    delete p;
    p = new Plasma::Package(mPackageRoot, mPackage, ps);
    QVERIFY(!p->isValid());

    // Create the metadata.desktop file.
    QFile file(mPackageRoot + "/" + mPackage + "/metadata.desktop");
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

    QTextStream out(&file);
    out << "[Desktop Entry]\n";
    out << "Name=test\n";
    out << "Description=Just a test desktop file";
    file.flush();
    file.close();

    // Create the code dir.
    QVERIFY(QDir().mkpath(mPackageRoot + "/" + mPackage + "/contents/code"));

    // No main file yet so should still be invalid.
    delete p;
    p = new Plasma::Package(mPackageRoot, mPackage, ps);
    QVERIFY(!p->isValid());

    // Create the main file.
    file.setFileName(mPackageRoot + "/" + mPackage + "/contents/code/main");
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

    out.setDevice(&file);
    out << "THIS IS A PLASMOID SCRIPT.....";
    file.flush();
    file.close();

    // Main file exists so should be valid now.
    delete p;
    p = new Plasma::Package(mPackageRoot, mPackage, ps);
    QVERIFY(p->isValid());
}

void PlasmoidPackageTest::filePath()
{
    // Package::filePath() returns
    // - {package_root}/{package_name}/path/to/file if the file exists
    // - QString() otherwise.
    p = new Plasma::Package(mPackageRoot, mPackage, ps);

    QCOMPARE(p->filePath("scripts", "main"), QString());

    QVERIFY(QDir().mkpath(mPackageRoot + "/" + mPackage + "/contents/code"));
    QFile file(mPackageRoot + "/" + mPackage + "/contents/code/main");
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

    QTextStream out(&file);
    out << "THIS IS A PLASMOID SCRIPT.....";
    file.flush();
    file.close();

    // The package is valid by now so a path for code/main should get returned.
    delete p;
    p = new Plasma::Package(mPackageRoot, mPackage, ps);

    QString path = mPackageRoot + "/" + mPackage + "/contents/code/main";

    // Two ways to get the same info.
    // 1. Give the file type which refers to a class of files (a directory) in
    //    the package structure and the file name.
    // 2. Give the file type which refers to a file in the package structure.
    //
    // NOTE: scripts, main and mainscript are defined in packages.cpp and are
    //       specific for a PlasmoidPackage.
    QCOMPARE(p->filePath("scripts", "main"), path);
    QCOMPARE(p->filePath("mainscript"), path);
}

void PlasmoidPackageTest::entryList()
{
    QString packageName("SomePlasmoid");

    // Create a package named @p packageName which is valid and has some images.
    createTestPackage(packageName);

    // Create a package object and verify that it is valid.
    p = new Plasma::Package(mPackageRoot, packageName, ps);
    QVERIFY(p->isValid());

    // Now we have a valid package that should contain the following files in
    // given filetypes:
    // fileTye - Files
    // scripts - {"main"}
    // images - {"image-1.svg", "image-2.svg"}
    QStringList files = p->entryList("scripts");
    QCOMPARE(files.size(), 1);
    QVERIFY(files.contains("main"));

    files = p->entryList("images");
    QCOMPARE(files.size(), 2);
    QVERIFY(files.contains("image-1.svg"));
    QVERIFY(files.contains("image-2.svg"));
}

void PlasmoidPackageTest::knownPackages()
{
    // Don't do strange things when package root doesn't exists.
    QDir pRoot = QDir(mPackageRoot + "blah");
    QVERIFY(!pRoot.exists());
    p = new Plasma::Package(mPackageRoot + "blah", mPackage, ps);
    QCOMPARE(Plasma::Package::listInstalled(mPackageRoot), QStringList());
    delete p;

    // Don't do strange things when an empty package root exists
    QVERIFY(QDir().mkpath(mPackageRoot));
    //QVERIFY(pRoot.exists());
    p = new Plasma::Package(mPackageRoot, mPackage, ps);
    QCOMPARE(Plasma::Package::listInstalled(mPackageRoot), QStringList());
    delete p;

    // Do not return a directory as package if it has no metadata.desktop file
    QVERIFY(QDir().mkpath(mPackageRoot + "/invalid_plasmoid"));
    p = new Plasma::Package(mPackageRoot, mPackage, ps);
    QCOMPARE(Plasma::Package::listInstalled(mPackageRoot), QStringList());
    delete p;

    // Let's add a valid package and see what happens.
    QString plamoid1("a_valid_plasmoid");
    createTestPackage(plamoid1);
    p = new Plasma::Package(mPackageRoot, mPackage, ps);

    QStringList packages = Plasma::Package::listInstalled(mPackageRoot);
    QCOMPARE(packages.size(), 1);
    QVERIFY(packages.contains(plamoid1));

    // Ok.... one more valid package.
    QString plamoid2("another_valid_plasmoid");
    createTestPackage(plamoid2);
    p = new Plasma::Package(mPackageRoot, mPackage, ps);

    packages = Plasma::Package::listInstalled(mPackageRoot);
    QCOMPARE(packages.size(), 2);
    QVERIFY(packages.contains(plamoid1));
    QVERIFY(packages.contains(plamoid2));
}

void PlasmoidPackageTest::metadata()
{
    QString plasmoid("plasmoid_with_metadata");
    createTestPackage(plasmoid);

    QString path = mPackageRoot + '/' + plasmoid + "/metadata.desktop";
    p = new Plasma::Package(mPackageRoot, plasmoid, ps);
    const Plasma::PackageMetadata metadata = p->metadata();
    QVERIFY(p->isValid());
    QCOMPARE(metadata.name(), plasmoid);
}

void PlasmoidPackageTest::createAndInstallPackage()
{
    QString plasmoid("plasmoid_to_package");
    createTestPackage(plasmoid);
    
    QString packagePath = mPackageRoot + '/' + "package.zip";
    Plasma::PackageMetadata metadata(
        QString(KDESRCDIR) + "/packagemetadatatest.desktop");
    QVERIFY(Plasma::Package::createPackage(metadata,
                                           mPackageRoot + '/' + plasmoid + "/contents",
                                           packagePath));
    QVERIFY(QFile::exists(packagePath));

    KZip package(packagePath);
    QVERIFY(package.open(QIODevice::ReadOnly));
    const KArchiveDirectory *dir = package.directory();
    QVERIFY(dir);
    QVERIFY(dir->entry("metadata.desktop"));
    const KArchiveEntry *contentsEntry = dir->entry("contents");
    QVERIFY(contentsEntry);
    QVERIFY(contentsEntry->isDirectory());
    const KArchiveDirectory *contents = 
        static_cast<const KArchiveDirectory *>(contentsEntry);
    QVERIFY(contents->entry("code"));
    QVERIFY(contents->entry("images"));

    QVERIFY(Plasma::Package::installPackage(packagePath, mPackageRoot, "plasma-applet-"));
    QString installedPackage = mPackageRoot + "/test";

    QVERIFY(QFile::exists(installedPackage));

    p = new Plasma::Package(installedPackage, ps);
    QVERIFY(p->isValid());
}

QTEST_KDEMAIN(PlasmoidPackageTest, NoGUI)
