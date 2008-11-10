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

#include "packagemetadatatest.h"

#include <QDir>
#include <QFile>

void PackageMetadataTest::init()
{
    pm = new Plasma::PackageMetadata;

    // Create data dir
    mDataDir = QDir::homePath() + "/.kde-unit-test/share/config/";
    QVERIFY(QDir().mkpath(mDataDir));

    QDir dir(mDataDir);
    QFile::copy(QString::fromLatin1(KDESRCDIR) 
        + QLatin1String("packagemetadatatest.desktop"), mDataDir 
        + QLatin1String("packagemetadatatest.desktop"));
}

void PackageMetadataTest::cleanup()
{
    delete pm;
}

// Copied from ktimezonetest.h
void PackageMetadataTest::removeDir(const QString &subdir)
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

void PackageMetadataTest::read()
{
    pm->read("packagemetadatatest.desktop");

    QVERIFY(pm->isValid());

    QCOMPARE(pm->name(), QString("Package metadata test file"));
    QCOMPARE(pm->description(), QString("A test desktop file to test the PackageMetaData class."));
    QCOMPARE(pm->serviceType(), QString("Plasma/Applet"));
    QCOMPARE(pm->author(), QString("Bertjan Broeksema"));
    QCOMPARE(pm->email(), QString("b.broeksema@kdemail.net"));
    QCOMPARE(pm->version(), QString("pre0.1"));
    QCOMPARE(pm->website(), QString("http://plasma.kde.org/"));
    QCOMPARE(pm->license(), QString("GPL"));
    QCOMPARE(pm->application(), QString("A Test name"));
    QCOMPARE(pm->requiredVersion(), QString("1.2.3"));
    QCOMPARE(pm->category(), QString("System test"));
}

void PackageMetadataTest::write()
{
    pm->setName(QString("Package metadata test file copy"));
    pm->setDescription(QString("Some other fancy test description"));
    pm->setServiceType(QString("Plasma/Applet"));
    pm->setAuthor(QString("Bertjan Broeksema"));
    pm->setEmail(QString("b.broeksema@kdemail.net"));
    pm->setVersion(QString("pre0.1"));
    pm->setWebsite(QString("http://plasma.kde.org/"));
    pm->setLicense(QString("GPL"));
    pm->setApplication(QString("A Test name"));
    pm->setRequiredVersion(QString("1.2.3"));

    pm->write(mDataDir + "somefile.desktop");
    delete pm;

    pm = new Plasma::PackageMetadata(mDataDir + "somefile.desktop");

    QCOMPARE(pm->name(), QString("Package metadata test file copy"));
    QCOMPARE(pm->description(), QString("Some other fancy test description"));
    QCOMPARE(pm->serviceType(), QString("Plasma/Applet"));
    QCOMPARE(pm->author(), QString("Bertjan Broeksema"));
    QCOMPARE(pm->email(), QString("b.broeksema@kdemail.net"));
    QCOMPARE(pm->version(), QString("pre0.1"));
    QCOMPARE(pm->website(), QString("http://plasma.kde.org/"));
    QCOMPARE(pm->license(), QString("GPL"));
    QCOMPARE(pm->application(), QString("A Test name"));
    QCOMPARE(pm->requiredVersion(), QString("1.2.3"));
}

QTEST_KDEMAIN(PackageMetadataTest, NoGUI)
