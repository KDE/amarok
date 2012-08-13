#ifndef SPOTIFYSETTINGS_H_
#define SPOTIFYSETTINGS_H_

#include <KDialog>
#include "SpotifyConfig.h"
#include "network/NetworkAccessManagerProxy.h"

namespace Ui { class SpotifyConfigWidget; }

class SpotifySettings: public KDialog
{
    Q_OBJECT

public:
    explicit SpotifySettings( QWidget *parent = 0, const QVariantList &args = QVariantList() );
    virtual ~SpotifySettings();

signals:
    void changed( bool );

public Q_SLOTS:
    virtual void save();
    virtual void load();
    virtual void defaults();
    void settingsChanged();
    void cancel();

private Q_SLOTS:
    void tryLogin();
    void slotDownloadError( QNetworkReply::NetworkError error );
    void slotDownloadProgress( qint64 current, qint64 total );
    void slotDownloadFinished();

private:
    void tryDownloadResolver();
    Ui::SpotifyConfigWidget *m_configWidget;
    SpotifyConfig m_config;
    QNetworkReply* m_downloadReply;
};

#endif
