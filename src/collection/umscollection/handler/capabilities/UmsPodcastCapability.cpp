#include "UmsPodcastCapability.h"

UmsPodcastCapability::UmsPodcastCapability()
        : Handler::PodcastCapability()
{
}

UmsPodcastCapability::~UmsPodcastCapability()
{
    
}

void
UmsPodcastCapability::prepareToParsePodcasts()
{
    
}

bool
UmsPodcastCapability::isEndOfParsePodcastsList()
{
    return true;
}

void
UmsPodcastCapability::prepareToParseNextPodcast() {}

void
UmsPodcastCapability::nextPodcastToParse()
{

}

bool
UmsPodcastCapability::shouldNotParseNextPodcast()
{
    return true;
}

void
UmsPodcastCapability::prepareToParsePodcastEpisode()
{
}

bool
UmsPodcastCapability::isEndOfParsePodcast()
{
    return true;
}

void
UmsPodcastCapability::prepareToParseNextPodcastEpisode()
{

}

void
UmsPodcastCapability::nextPodcastEpisodeToParse()
{

}

Handler::MediaDevicePodcastEpisodePtr
UmsPodcastCapability::libGetEpisodePtrForEpisodeStruct()
{
    return Handler::MediaDevicePodcastEpisodePtr();
}

QString
UmsPodcastCapability::libGetPodcastName()
{
    return QString( "UMS Podcast" );
}
