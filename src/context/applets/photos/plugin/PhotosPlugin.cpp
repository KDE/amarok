/*
 * Copyright 2017  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "PhotosEngine.h"
#include "Version.h"

#include <QNetworkAccessManager>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QQmlNetworkAccessManagerFactory>
#include <qqml.h>


// User-Agent header needed to avoid 403 Forbidden on some photos when QML Image fetches from Flickr
// Using NetworkAccessManagerProxy might have been nicer, but QML runs in different thread than The::networkAccessManager
// so this is simpler.
class PhotoAppletNetworkManager : public QNetworkAccessManager
{
public:
    PhotoAppletNetworkManager(QObject *parent) : QNetworkAccessManager(parent) { }
    QNetworkReply *createRequest( Operation op, const QNetworkRequest &req, QIODevice * outgoingData = nullptr ) override
    {
        QNetworkRequest newreq=req;
        newreq.setRawHeader("User-Agent", ( ( QStringLiteral( "Amarok/" ) + QStringLiteral(AMAROK_VERSION) ) ).toUtf8() );
        QNetworkReply *reply = QNetworkAccessManager::createRequest( op, newreq, outgoingData );
        return reply;
    }
};

class PhotoAppletNetworkAccess : public QQmlNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent) override
    {
        return new PhotoAppletNetworkManager(parent);
    }
};


class PhotosPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char* uri) override
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("org.kde.amarok.photos"));

        qmlRegisterSingletonType<PhotosEngine>(uri, 1, 0, "PhotosEngine", photos_engine_provider);
    }

    static QObject *photos_engine_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
    {
        engine->setNetworkAccessManagerFactory(new PhotoAppletNetworkAccess);
        Q_UNUSED(scriptEngine)

        return new PhotosEngine();
    }
};

#include <PhotosPlugin.moc>
