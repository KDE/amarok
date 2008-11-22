/***************************************************************************
 * copyright            : (C) 2005 Ian Monroe <ian@monroe.nu> 
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
 **************************************************************************/
#ifndef AMAROK_REFRESHIMAGES_H
#define AMAROK_REFRESHIMAGES_H

#include <QMap>
#include <QObject>

class KJob;

class JobInfo
{
  public:
    JobInfo()
      : m_asin(), m_locale(), m_detailUrl(), m_last( false ) { } //for QMap
    JobInfo(const QString& asin, const QString& locale, bool last) :
      m_asin(asin), m_locale(locale), m_last(last) { }
    QString m_asin;
    QString m_locale;
    QString m_detailUrl;
    bool m_last;
};

class RefreshImages : public QObject
{
    Q_OBJECT
  public:
    RefreshImages();
  private slots:
    void finishedXmlFetch( KJob* );
    void finishedImageFetch( KJob* );
  private:
    static QString localeToTLD(const QString& locale);
    QMap<QString, JobInfo> m_jobInfo;
};
#endif
