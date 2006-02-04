#include <iostream>
#include "mp4audiosampleentry.h"
#include "mp4isobox.h"
#include "mp4file.h"
#include "mp4propsproxy.h"

using namespace TagLib;

class MP4::Mp4AudioSampleEntry::Mp4AudioSampleEntryPrivate
{
public:
  TagLib::uint channelcount;
  TagLib::uint samplerate;
};

MP4::Mp4AudioSampleEntry::Mp4AudioSampleEntry( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset )
	:Mp4SampleEntry(file, fourcc, size, offset)
{
  d = new MP4::Mp4AudioSampleEntry::Mp4AudioSampleEntryPrivate();
}

MP4::Mp4AudioSampleEntry::~Mp4AudioSampleEntry()
{
  delete d;
}

TagLib::uint MP4::Mp4AudioSampleEntry::channels() const
{
  return d->channelcount;
}

TagLib::uint MP4::Mp4AudioSampleEntry::samplerate() const
{
  return d->samplerate;
}

void MP4::Mp4AudioSampleEntry::parseEntry()
{
  TagLib::MP4::File* mp4file = dynamic_cast<TagLib::MP4::File*>(file());
  if(!mp4file)
    return;

  // read 8 reserved bytes
  mp4file->seek( 8, TagLib::File::Current );
  // read channelcount
  if(!mp4file->readShort( d->channelcount ))
    return;
  // seek over samplesize, pre_defined and reserved
  mp4file->seek( 6, TagLib::File::Current );
  // read samplerate
  if(!mp4file->readInt( d->samplerate ))
    return;

  mp4file->seek( size()-36, TagLib::File::Current );

  // register box at proxy
  mp4file->propProxy()->registerAudioSampleEntry( this );
}

