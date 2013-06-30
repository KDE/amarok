/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef TEST_IMPORTERS_COMMON_H
#define TEST_IMPORTERS_COMMON_H

#include "core/meta/forward_declarations.h"

#include <QObject>
#include <QMap>

namespace Collections
{
    class CollectionTestImpl;
}
class DatabaseImporter;

class TestImportersCommon : public QObject
{
    Q_OBJECT

public:
    TestImportersCommon();
    ~TestImportersCommon();

public slots:
    void importFailed();

protected:
    virtual DatabaseImporter *newInstance() = 0;
    virtual QString pathForMetadata( const QString &artist, const QString &album,
                                     const QString &title ) = 0;
    void blockingImport();
    void setUpTestData();

    QList<QString> m_trackList;
    QMap<QString, Meta::TrackPtr> m_trackForName;

    /// The collection to which we import
    QScopedPointer<Collections::CollectionTestImpl> m_localCollection;

    /// Simulates a file collection, enabling usage of CollectionManager::trackForUrl
    QScopedPointer<Collections::CollectionTestImpl> m_fileCollection;

private slots:
    void initTestCase();
    void cleanup();
    void init();

    void newTracksShouldBeAddedToCollection_data() { setUpTestData(); }
    void newTracksShouldBeAddedToCollection();
    void importedTracksShouldOverwriteEmptyStatistics_data() { setUpTestData(); }
    void importedTracksShouldOverwriteEmptyStatistics();
    void importingShouldUseHigherPlaycount();
};

#endif
