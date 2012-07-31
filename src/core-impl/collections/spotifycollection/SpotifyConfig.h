#ifndef SPOTIFYCONFIG_H_
#define SPOTIFYCONFIG_H_

#include <QObject>
#include <QString>

namespace KWallet { class Wallet; }

class KDialog;

class SpotifyConfig: public QObject
{
    Q_OBJECT
public:
    SpotifyConfig();
    ~SpotifyConfig();

    static const char *configSectionName() { return "Collection_Spotify"; }
    static const QString resolverName();
    void load();
    void save();
    void reset();

    const QString username() const { return m_username; }
    void setUsername( const QString& username ) { m_username = username; }

    const QString password() const { return m_password; }
    void setPassword( const QString& password ) { m_password = password; }

    const QByteArray apikey() const { return m_apikey; }
    void setApiKey( const QString base64 ) { m_apikey = QByteArray::fromBase64( QByteArray( base64.toAscii() ) ); }

    const QString resolverPath() const { return m_resolverPath; }
    void setResolverPath( const QString& path ) { m_resolverPath = path; }

    bool highQuality() const { return m_highQuality; }
    void setHighQuality( const bool highquality ) { m_highQuality = highquality; }

private:
    QString m_username;
    QString m_password;
    QByteArray m_apikey;
    QString m_resolverPath;
    bool m_highQuality;
    const static QString m_resolverDownloadUrl;

    KWallet::Wallet* m_wallet;
};

#endif
