// (c) 2005 Ian Monroe <ian@monroe.nu>
// See COPYING file for licensing information.

#ifndef AMAROK_REFRESHIMAGES_H
#define AMAROK_REFRESHIMAGES_H

#include <qobject.h>
namespace KIO {
    class StoredTransferJob;
    class Job;
}
class QStringList;

class JobInfo
{
  public:
    JobInfo() : m_last(false) { } //for QMap
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
    void finishedXmlFetch( KIO::Job* );
    void finishedImageFetch( KIO::Job* );
  private:
    static QString localeToTLD(const QString& locale);
    QMap<QString, JobInfo> m_jobInfo;
};
#endif
