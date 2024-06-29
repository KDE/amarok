/****************************************************************************************
 * Copyright (c) 2017 Malte Veerman <malte.veerman@gmail.com>                             *
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

#define DEBUG_PREFIX "AppletModel"

#include "AppletModel.h"

#include "AmarokContextPackageStructure.h"
#include "AppletLoader.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QUrl>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include <algorithm>


using namespace Context;


class Context::AppletPackage : public KPackage::Package
{
public:
    AppletPackage(const KPackage::Package &package) : KPackage::Package(package) {}

    bool operator==(const AppletPackage &p) const
    {
        return metadata() == p.metadata();
    }
};


AppletModel::AppletModel(AppletLoader *loader, QObject* parent)
    : QAbstractListModel(parent)
    , m_loader(loader)
{
    newApplets(loader->applets());
    connect(loader, &AppletLoader::finished, this, &AppletModel::newApplets);
}

AppletModel::~AppletModel()
{
}

int AppletModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)

    return m_packages.size();
}

void AppletModel::newApplets(const QList<KPluginMetaData>& applets)
{
    DEBUG_BLOCK

    beginResetModel();

    m_packages.clear();

    for (const auto &applet : applets)
    {
        auto loader = KPackage::PackageLoader::self();
        auto structure = new AmarokContextPackageStructure;
        loader->addKnownPackageStructure(QStringLiteral("Amarok/Context"), structure);
        auto package = loader->loadPackage(QStringLiteral("Amarok/Context"), applet.pluginId());

        if (package.isValid())
        {
            m_packages << package;
        }
        else
            error() << "Error loading package:" << applet.pluginId();
    }

    //Sort applets by name
    std::sort(m_packages.begin(), m_packages.end(), [] (const AppletPackage &p1, const AppletPackage &p2) {
        return p1.metadata().name() < p2.metadata().name();
    });

    endResetModel();
}

QVariant AppletModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();

    if (row >= m_packages.size())
        return QVariant();

    const auto &package = m_packages.at(row);

    switch (role)
    {
        case Name:
            return package.metadata().name();

        case Id:
            return package.metadata().pluginId();

        case Icon:
            return package.fileUrl("icon");

        case Mainscript:
            return package.fileUrl("mainscript");

        case Collapsed:
            return Amarok::config(QStringLiteral("Context")).readEntry(package.metadata().pluginId() + "_collapsed", false);

        case ContentHeight:
            return Amarok::config(QStringLiteral("Context")).readEntry(package.metadata().pluginId() + "_contentHeight", 300);
    }

    return QVariant();
}

bool Context::AppletModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    int row = index.row();

    if (row >= m_packages.size())
        return false;

    const auto &package = m_packages.at(row);

    switch (role)
    {
        case Collapsed:
        {
            Amarok::config(QStringLiteral("Context")).writeEntry(package.metadata().pluginId() + "_collapsed", value.toBool());
            Q_EMIT dataChanged(index, index, QVector<int>{role});
            return true;
        }
        case ContentHeight:
        {
            Amarok::config(QStringLiteral("Context")).writeEntry(package.metadata().pluginId() + "_contentHeight", value.toReal());
            Q_EMIT dataChanged(index, index, QVector<int>{role});
            return true;
        }

        default:
            warning() << (Role) role << "is read-only.";
    }

    return false;
}

QHash< int, QByteArray > AppletModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(Name, "name");
    roles.insert(Id, "appletId");
    roles.insert(Icon, "icon");
    roles.insert(Mainscript, "mainscript");
    roles.insert(Collapsed, "collapsed");
    roles.insert(ContentHeight, "contentHeight");

    return roles;
}

void AppletModel::setAppletCollapsed(const QString& id, bool collapsed)
{
    DEBUG_BLOCK

    debug() << "Set collapsed for applet:" << id << "to:" << collapsed;

    auto package = findPackage(id);
    if (package.isValid())
    {
        Amarok::config(QStringLiteral("Context")).writeEntry(id + QStringLiteral("_collapsed"), collapsed);
        int row = m_packages.indexOf(package);
        auto index = createIndex(row, 0);
        Q_EMIT dataChanged(index, index, QVector<int>{Collapsed});
    }
}

void Context::AppletModel::setAppletContentHeight(const QString& id, qreal height)
{
    DEBUG_BLOCK

    debug() << "Set content height for applet:" << id << "to:" << height;

    auto package = findPackage(id);
    if (package.isValid())
    {
        Amarok::config(QStringLiteral("Context")).writeEntry(id + "_contentHeight", height);
        int row = m_packages.indexOf(package);
        auto index = createIndex(row, 0);
        Q_EMIT dataChanged(index, index, QVector<int>{ContentHeight});
    }
}

QUrl Context::AppletModel::imageUrl(const QString& id, const QString& imageName)
{
    auto package = findPackage(id);
    if (package.isValid())
        return package.fileUrl("images", imageName);
    return QUrl();
}

AppletPackage AppletModel::findPackage(const QString& id)
{
    for (const auto &package : m_packages)
    {
        auto metadata = package.metadata();

        if (metadata.pluginId() == id)
            return package;
    }

    warning() << "Applet with id:" << id << "not found.";
    return AppletPackage(KPackage::Package());
}

AppletProxyModel::AppletProxyModel(AppletModel* appletModel, QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_appletModel(appletModel)
{
    setSourceModel(appletModel);
    setDynamicSortFilter(true);
    sort(0);

    connect(m_appletModel->loader(), &AppletLoader::finished, this, &AppletProxyModel::enabledAppletsChanged);
}

AppletProxyModel::~AppletProxyModel()
{
}

QStringList AppletProxyModel::enabledApplets() const
{
    QStringList list;
    for (const auto &applet : m_appletModel->loader()->enabledApplets())
    {
        list << applet.pluginId();
    }

    std::sort(list.begin(), list.end(),
              [] (const QString &a, const QString &b)  {
                  QStringList ae = Amarok::config(QStringLiteral("Context")).readEntry("enabledApplets", QStringList());
                  return ae.indexOf(a) < ae.indexOf(b);
              }
    );

    return list;
}

void AppletProxyModel::setAppletEnabled(const QString& id, bool enabled, int place)
{
    DEBUG_BLOCK

    debug() << "Set enabled for applet:" << id << "to:" << enabled;

    if (enabled == appletEnabled(id))
        return;

    QStringList ea = enabledApplets();

    if (enabled)
    {
        if (place <= -1)
            place = ea.size();

        debug() << "Applet's new place is" << place;
        ea.insert(place, id);
    }
    else
    {
        ea.removeAll(id);
    }
    Amarok::config(QStringLiteral("Context")).writeEntry("enabledApplets", ea);

    debug() << "New enabled applets:" << ea;

    invalidateFilter();

    Q_EMIT enabledAppletsChanged();
}

void AppletProxyModel::setAppletPlace(const QString& id, int place)
{
    DEBUG_BLOCK

    debug() << "Set place for applet:" << id << "to:" << place;

    int currentPlace = appletPlace(id);
    debug() << "Current place is" << currentPlace;

    if (currentPlace == place)
        return;

    if (place <= -1)
    {
        setAppletEnabled(id, false);
        return;
    }

    if (currentPlace <= -1)
        setAppletEnabled(id, true, place);

    QStringList ea = enabledApplets();

    place = qMin(place, ea.size() - 1);
    bool forward = place > currentPlace;

    beginMoveRows(QModelIndex(), currentPlace, currentPlace, QModelIndex(), forward ? place + 1 : place);
    ea.move(currentPlace, place);
    Amarok::config(QStringLiteral("Context")).writeEntry("enabledApplets", ea);
    endMoveRows();

    debug() << "New enabled applets:" << ea;
}

int AppletProxyModel::appletPlace(const QString& id) const
{
    return enabledApplets().indexOf(id);
}

bool AppletProxyModel::appletEnabled(const QString& id) const
{
    return enabledApplets().contains(id);
}

void Context::AppletProxyModel::clear()
{
    for( const auto &applet : enabledApplets() )
    {
        setAppletEnabled( applet, false );
    }
}

bool AppletProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
{
    int placeLeft = appletPlace(source_left.data(AppletModel::Id).toString());
    int placeRight = appletPlace(source_right.data(AppletModel::Id).toString());

    return placeLeft < placeRight;
}

bool AppletProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    return appletEnabled(index.data(AppletModel::Id).toString());
}


