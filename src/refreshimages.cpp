// (c) 2005 Ian Monroe <ian@monroe.nu>
// See COPYING file for licensing information.

#define DEBUG_PREFIX "RefreshImages"

#include "amarok.h"
#include "debug.h"
#include "collectiondb.h"
#include "refreshimages.h"
#include "statusbar.h"

#include <qdom.h>
#include <qimage.h>
#include <qmap.h>
#include <qobject.h>
#include <qvaluelist.h> 
#include <qvariant.h>

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/scheduler.h>

#include <klocale.h>


RefreshImages::RefreshImages()
{
    static const QString LICENSE = "0RQSQ0B8CRY7VX2VF3G2"; //Ian Monroe
    const QStringList staleImages = CollectionDB::instance()->staleImages();
    //"SELECT asin, locale, filename FROM amazon WHERE refetchdate > %1 ;"
    QStringList::ConstIterator it = staleImages.begin();
    QStringList::ConstIterator end = staleImages.end();
    while(it != end)
    {
        QString asin=*it;
        it++;
        QString locale = *it;
        it++;
        QString md5sum = *it;
        QString url =
            QString("http://webservices.amazon.%1/onca/xml?Service=AWSECommerceService&SubscriptionId=%2&Operation=ItemLookup&ItemId=%3&ResponseGroup=Small,Images")
             .arg(localeToTLD(locale))
             .arg(LICENSE)
             .arg(asin);
        debug() << "going to get " << url << endl;  
        KIO::TransferJob* job = KIO::storedGet( url, false, false );
        KIO::Scheduler::scheduleJob(job);
        amaroK::StatusBar::instance()->newProgressOperation( job );
        job->setName(md5sum.ascii());
        it++; //iterate to the next set
        m_jobInfo[md5sum] = JobInfo(asin, locale, it==end); 
        connect( job, SIGNAL(result( KIO::Job* )), SLOT(finishedXmlFetch( KIO::Job* )) );
    }
}

void RefreshImages::finishedXmlFetch( KIO::Job* xmlJob ) //SLOT
{

    if( xmlJob->error() ) {
        amaroK::StatusBar::instance()->shortMessage(i18n("There was an error communicating with Amazon."));
        return;
    }
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( xmlJob );
    const QString xml = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );

    QDomDocument doc;
    if( !doc.setContent( xml ) )
      { debug() << "Amazon returned invalid XML" << endl;  return; }

    QString imageUrl = doc.documentElement()
       .namedItem( "Items" )
       .namedItem("Item")
       .namedItem("LargeImage")
       .namedItem("URL").firstChild().toText().data();
    debug() << "setting up " << imageUrl << endl;
    KURL testUrl(imageUrl);
    if(!testUrl.isValid()) //KIO crashs on empty strings!!!
    {
        debug() << "Image url not returned, cover not being refreshed" << endl;
        return;
    }
    KIO::TransferJob* imageJob = KIO::storedGet( imageUrl, false, false );
    KIO::Scheduler::scheduleJob(imageJob);
    amaroK::StatusBar::instance()->newProgressOperation( imageJob );
    imageJob->setName(xmlJob->name());
    //get the URL of the detail page
    m_jobInfo[xmlJob->name()].m_detailUrl = doc.documentElement()
       .namedItem( "Items" )
       .namedItem("Item")
       .namedItem("DetailPageURL").firstChild().toText().data();
    connect( imageJob, SIGNAL(result( KIO::Job* )), SLOT(finishedImageFetch( KIO::Job* )) );
}

void RefreshImages::finishedImageFetch(KIO::Job* imageJob)
{
   if( imageJob->error() ) {
        amaroK::StatusBar::instance()->shortMessage(i18n("There was an error communicating with Amazon."));
        return;
    }
    QImage img;
    img.loadFromData(static_cast<KIO::StoredTransferJob*>(imageJob)->data());
    img.setText( "amazon-url", 0, m_jobInfo[imageJob->name()].m_detailUrl);
    img.save( amaroK::saveLocation("albumcovers/large/") + imageJob->name(), "PNG");

    CollectionDB::instance()->newAmazonReloadDate( m_jobInfo[imageJob->name()].m_asin
        , m_jobInfo[imageJob->name()].m_locale
        , imageJob->name());

    debug() << m_jobInfo[imageJob->name()].m_asin << " copied " << imageJob->name() << endl;
    if(m_jobInfo[imageJob->name()].m_last)
    {
      deleteLater();
    }
}

QString RefreshImages::localeToTLD(const QString& locale)
{
    if(locale=="us") 
        return "com";
    else if(locale=="jp")
        return "co.jp";
    else if(locale=="uk")
        return "co.uk";
    else
        return locale;
}
#include "refreshimages.moc"
