#include "providerinitjob.h"

#include <QTimer>

#include <KUrl>


using namespace AmarokAttica;


ProviderInitJob::ProviderInitJob(const QString& id, QObject* parent)
    : KJob(parent), m_id(id)
{
}


void ProviderInitJob::start()
{
    QTimer::singleShot(0, this, SLOT(doWork()));
}


void ProviderInitJob::doWork()
{
    if (m_id == "opendesktop") {
        m_provider = Provider(m_id, KUrl("https://api.opendesktop.org/v1/"), "OpenDesktop.org");
    }
    emitResult();
}


Provider ProviderInitJob::provider() const
{
    return m_provider;
}


#include "providerinitjob.moc"
