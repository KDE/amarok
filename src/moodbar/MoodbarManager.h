/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef MOODBARMANAGER_H
#define MOODBARMANAGER_H

#include "amarok_export.h"
#include "core/meta/forward_declarations.h"

#include <KImageCache>

#include <QColor>
#include <QMap>
#include <QObject>

class QUrl;
class QPalette;
class MoodbarManager;

namespace The {
    AMAROK_EXPORT MoodbarManager* moodbarManager();
}

typedef QVector<QColor> MoodbarColorList;

class AMAROK_EXPORT  MoodbarManager : public QObject
{

Q_OBJECT

friend MoodbarManager* The::moodbarManager();

public:
    enum Style
    {
        SystemColours,
        Angry,
        Frozen,
        Happy,
        Normal
    };
    ~MoodbarManager() override;

    bool hasMoodbar( Meta::TrackPtr track );
    QPixmap getMoodbar( Meta::TrackPtr track, int width, int height, bool rtl = false );

Q_SIGNALS:
    void moodbarReady( const QPixmap &pixmap );
    void moodbarCreationFailed( const QString &error );
    void moodbarStyleChanged();

private Q_SLOTS:
    void paletteChanged( const QPalette &palette );

private:
    MoodbarManager();

    MoodbarColorList readMoodFile( const QUrl &moodFileUrl );
    QPixmap drawMoodbar( const MoodbarColorList &data, int width, int height, bool rtl );
    QString moodPath( const QString &trackPath ) const;

    //let this class take care of caching everything as needed, otherwise things get pretty complex pretty fast.
    QMap<Meta::TrackPtr, bool> m_hasMoodMap;
    QMap<Meta::TrackPtr, QString> m_moodFileMap;
    QMap<Meta::TrackPtr, MoodbarColorList> m_moodDataMap;

    KImageCache * m_cache;

    int m_lastPaintMode;
};

#endif // MOODBARMANAGER_H
