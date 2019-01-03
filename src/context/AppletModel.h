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

#ifndef APPLETMODEL_H
#define APPLETMODEL_H

#include <QAbstractListModel>
#include <QSortFilterProxyModel>


class KPluginMetaData;

namespace Context
{

class AppletLoader;
class AppletPackage;

class AppletModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        Name,
        Id,
        Icon,
        Mainscript,
        Collapsed,
        PackagePath,
        ContentHeight
    };
    Q_ENUM(Role)

    explicit AppletModel(AppletLoader *loader, QObject *parent = nullptr);
    ~AppletModel() override;

    virtual int rowCount(const QModelIndex& parent) const override;
    virtual QVariant data(const QModelIndex& index, int role) const override;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    virtual QHash< int, QByteArray > roleNames() const override;

    AppletLoader* loader() const { return m_loader; }

    Q_INVOKABLE void setAppletCollapsed(const QString &id, bool collapsed);
    Q_INVOKABLE void setAppletContentHeight(const QString& id, qreal height);
    Q_INVOKABLE QUrl imageUrl(const QString &id, const QString &imageName);

public Q_SLOTS:
    void newApplets(const QList<KPluginMetaData> &applets);

protected:
    AppletPackage findPackage(const QString &id);

private:
    QList<AppletPackage> m_packages;
    AppletLoader *const m_loader;
};

class AppletProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(QStringList enabledApplets READ enabledApplets NOTIFY enabledAppletsChanged)

public:
    explicit AppletProxyModel(AppletModel *appletModel, QObject *parent = nullptr);
    virtual ~AppletProxyModel();

    /**
     * @returns QStringList with ids of all enabled applets.
     * Sorted in ascending order of place of applets.
     */
    QStringList enabledApplets() const;

    /**
     * Set an applet to be enabled or disabled. Does nothing if id is invalid.
     * Disabling an applet sets its place to -1.
     *
     * @param id Id of the applet.
     * @param enabled Set to true if applet should be enabled and false for disabled.
     * @param place The place of the applet after enabling. -1 appends the applet to the end of the list.
     * Irrelevant if applet is to be disabled.
     */
    Q_INVOKABLE void setAppletEnabled(const QString &id, bool enabled, int place = -1);

    /**
     * Set an applet's place. Does nothing if id is invalid.
     * Enables the applet if it is disabled.
     *
     * @param id Id of the applet.
     * @param place The new place of the applet. Negative values disable the applet.
     */
    Q_INVOKABLE void setAppletPlace(const QString &id, int place);

    /**
     * Find out an applet's place.
     *
     * @returns an integer with the applet's place. -1 if the applet is disabled.
     *
     * @param id Id of applet's place to be returned.
     */
    Q_INVOKABLE int appletPlace(const QString &id) const;

    /**
     * Find out if an applet is enabled or disabled.
     *
     * @returns true if applet is enabled. Returns false if not.
     *
     * @param id Id of the applet.
     */
    Q_INVOKABLE bool appletEnabled(const QString &id) const;

    /**
     * Clear the context area by disabling all applets
     */
    Q_INVOKABLE void clear();

Q_SIGNALS:
    void enabledAppletsChanged();

protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    AppletModel *m_appletModel;
};

}

#endif // APPLETMODEL_H
