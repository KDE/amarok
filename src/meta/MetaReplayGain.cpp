/****************************************************************************************
 * Copyright (c) 2009 Alex Merry <alex.merry@kdemail.net>                               *
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

// NOTE: this file is used by amarokcollectionscanner and CANNOT use any amaroklib
//       code [this includes debug()]

#include "MetaReplayGain.h"

#include <QString>
#include <cmath>

// Taglib
#include <tag.h>
#include <tlist.h>
#include <tstring.h>
#include <textidentificationframe.h>
#include <apetag.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <mpcfile.h>
#include <mpegfile.h>
#include <oggfile.h>
#include <oggflacfile.h>
#include <speexfile.h>
#include <trueaudiofile.h>
#include <vorbisfile.h>
#include <wavpackfile.h>
#ifdef TAGLIB_EXTRAS_FOUND
#include <asffile.h>
#include <mp4file.h>
#endif

// converts a peak value from the normal digital scale form to the more useful decibel form
// decibels are relative to the /adjusted/ waveform
static qreal peakToDecibels( qreal scaleVal )
{
    if ( scaleVal > 0 )
        return 20 * log10( scaleVal );
    else
        return 0;
}

// NOTE: representation is taken to be a binary value with units in the first column,
//       1/2 in the second and so on.
static qreal readRVA2PeakValue( const TagLib::ByteVector &data, int bits, bool *ok )
{
    qreal peak = 0.0;
    // discarding digits at the end reduces precision, but doesn't otherwise change the value
    if ( bits > 32 )
        bits = 32;
    // the +7 makes sure we round up when we divide by 8
    unsigned int bytes = (bits + 7) / 8;

    // normalize appears not to write a peak at all, and hence sets bits to 0
    if ( bits == 0 )
    {
        if ( ok )
            *ok = true;
    }
    else if ( bits >= 4 && data.size() >= bytes ) // fewer than 4 bits would just be daft
    {
        // excessBits is the number of bits we have to discard at the end
        unsigned int excessBits = (8 * bytes) - bits;
        // mask has 1s everywhere but the last /excessBits/ bits
        quint32 mask = 0xffffffff << excessBits;
        quint32 rawValue = 0;
        for ( unsigned int i = 0; i < bytes; ++i )
        {
            rawValue <<= 8;
            rawValue += (unsigned char)data[i];
        }
        rawValue &= mask;
        peak = rawValue;
        // amount we need to "shift" the value right to make the first digit the unit column
        unsigned int rightShift = (8 * bytes) - 1;
        peak /= (qreal)(1 << rightShift);
        if ( ok )
            *ok = true;
    }
    else
    {
        if ( ok )
            *ok = false;
    }
    return peak;
}

// adds the converted version of the scale value if it is a valid, non-negative float
static void maybeAddPeak( const TagLib::String &scaleVal, Meta::ReplayGainTag key, Meta::ReplayGainTagMap *map )
{
    // scale value is >= 0, and typically not much bigger than 1
    QString value = TStringToQString( scaleVal );
    bool ok = false;
    qreal peak = value.toFloat( &ok );
    if ( ok && peak >= 0 )
        (*map)[key] = peakToDecibels( peak );
}

static void maybeAddGain( const TagLib::String &input, Meta::ReplayGainTag key, Meta::ReplayGainTagMap *map )
{
    QString value = TStringToQString( input ).remove( " dB" );
    bool ok = false;
    qreal gain = value.toFloat( &ok );
    if (ok)
        (*map)[key] = gain;
}

static Meta::ReplayGainTagMap readID3v2Tags( TagLib::ID3v2::Tag *tag )
{
    Meta::ReplayGainTagMap map;
    {   // ID3v2.4.0 native replay gain tag support (as written by Quod Libet, for example).
        TagLib::ID3v2::FrameList frames = tag->frameListMap()["RVA2"];
        frames.append(tag->frameListMap()["XRVA"]);
        if ( !frames.isEmpty() )
        {
            for ( unsigned int i = 0; i < frames.size(); ++i )
            {
                // we have to parse this frame ourselves
                // ID3v2 frame header is 10 bytes, so skip that
                TagLib::ByteVector data = frames[i]->render().mid( 10 );
                unsigned int offset = 0;
                QString desc( data.data() );
                offset += desc.count() + 1;
                unsigned int channel = data.mid( offset, 1 ).toUInt( true );
                // channel 1 is the main volume - the only one we care about
                if ( channel == 1 )
                {
                    ++offset;
                    qint16 adjustment512 = data.mid( offset, 2 ).toShort( true );
                    qreal adjustment = ( (qreal)adjustment512 ) / 512.0;
                    offset += 2;
                    unsigned int peakBits = data.mid( offset, 1 ).toUInt( true );
                    ++offset;
                    bool ok = false;
                    qreal peak = readRVA2PeakValue( data.mid( offset ), peakBits, &ok );
                    if ( ok )
                    {
                        if ( desc.toLower() == "album" )
                        {
                            map[Meta::ReplayGain_Album_Gain] = adjustment;
                            map[Meta::ReplayGain_Album_Peak] = peakToDecibels( peak );
                        }
                        else if ( desc.toLower() == "track" || !map.contains( Meta::ReplayGain_Track_Gain ) )
                        {
                            map[Meta::ReplayGain_Track_Gain] = adjustment;
                            map[Meta::ReplayGain_Track_Peak] = peakToDecibels( peak );
                        }
                    }
                }
            }
            if ( !map.isEmpty() )
                return map;
        }
    }

    {   // Foobar2000-style ID3v2.3.0 tags
        TagLib::ID3v2::FrameList frames = tag->frameListMap()["TXXX"];
        for ( TagLib::ID3v2::FrameList::Iterator it = frames.begin(); it != frames.end(); ++it ) {
            TagLib::ID3v2::UserTextIdentificationFrame* frame =
                dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>( *it );
            if ( frame && frame->fieldList().size() >= 2 )
            {
                QString desc = TStringToQString( frame->description() ).toLower();
                if ( desc == "replaygain_album_gain" )
                    maybeAddGain( frame->fieldList()[1], Meta::ReplayGain_Album_Gain, &map );
                if ( desc == "replaygain_album_peak" )
                    maybeAddPeak( frame->fieldList()[1], Meta::ReplayGain_Album_Peak, &map );
                if ( desc == "replaygain_track_gain" )
                    maybeAddGain( frame->fieldList()[1], Meta::ReplayGain_Track_Gain, &map );
                if ( desc == "replaygain_track_peak" )
                    maybeAddPeak( frame->fieldList()[1], Meta::ReplayGain_Track_Peak, &map );
            }
        }
    }
    return map;
}

static Meta::ReplayGainTagMap readAPETags( TagLib::APE::Tag *tag )
{
    Meta::ReplayGainTagMap map;
    const TagLib::APE::ItemListMap &items = tag->itemListMap();
    if ( items.contains("REPLAYGAIN_TRACK_GAIN") )
    {
        maybeAddGain( items["REPLAYGAIN_TRACK_GAIN"].values()[0], Meta::ReplayGain_Track_Gain, &map );
        if ( items.contains("REPLAYGAIN_TRACK_PEAK") )
            maybeAddPeak( items["REPLAYGAIN_TRACK_PEAK"].values()[0], Meta::ReplayGain_Track_Peak, &map );
    }
    if ( items.contains("REPLAYGAIN_ALBUM_GAIN") )
    {
        maybeAddGain( items["REPLAYGAIN_ALBUM_GAIN"].values()[0], Meta::ReplayGain_Album_Gain, &map );
        if ( items.contains("REPLAYGAIN_ALBUM_PEAK") )
            maybeAddPeak( items["REPLAYGAIN_ALBUM_PEAK"].values()[0], Meta::ReplayGain_Album_Peak, &map );
    }
    return map;
}

static Meta::ReplayGainTagMap readXiphTags( TagLib::Ogg::XiphComment *tag )
{
    const TagLib::Ogg::FieldListMap &tagMap = tag->fieldListMap();
    Meta::ReplayGainTagMap outputMap;

    if ( !tagMap["REPLAYGAIN_TRACK_GAIN"].isEmpty() )
    {
        maybeAddGain( tagMap["REPLAYGAIN_TRACK_GAIN"].front(), Meta::ReplayGain_Track_Gain, &outputMap );
        if ( !tagMap["REPLAYGAIN_TRACK_PEAK"].isEmpty() )
            maybeAddPeak( tagMap["REPLAYGAIN_TRACK_PEAK"].front(), Meta::ReplayGain_Track_Peak, &outputMap );
    }

    if ( !tagMap["REPLAYGAIN_ALBUM_GAIN"].isEmpty() )
    {
        maybeAddGain( tagMap["REPLAYGAIN_ALBUM_GAIN"].front(), Meta::ReplayGain_Album_Gain, &outputMap );
        if ( !tagMap["REPLAYGAIN_ALBUM_PEAK"].isEmpty() )
            maybeAddPeak( tagMap["REPLAYGAIN_ALBUM_PEAK"].front(), Meta::ReplayGain_Album_Peak, &outputMap );
    }

    return outputMap;
}

#ifdef TAGLIB_EXTRAS_FOUND
static Meta::ReplayGainTagMap readASFTags( TagLib::ASF::Tag *tag )
{
    const TagLib::ASF::AttributeListMap &tagMap = tag->attributeListMap();
    Meta::ReplayGainTagMap outputMap;

    if ( !tagMap["REPLAYGAIN_TRACK_GAIN"].isEmpty() )
    {
        maybeAddGain( tagMap["REPLAYGAIN_TRACK_GAIN"].front().toString(), Meta::ReplayGain_Track_Gain, &outputMap );
        if ( !tagMap["REPLAYGAIN_TRACK_PEAK"].isEmpty() )
            maybeAddPeak( tagMap["REPLAYGAIN_TRACK_PEAK"].front().toString(), Meta::ReplayGain_Track_Peak, &outputMap );
    }

    if ( !tagMap["REPLAYGAIN_ALBUM_GAIN"].isEmpty() )
    {
        maybeAddGain( tagMap["REPLAYGAIN_ALBUM_GAIN"].front().toString(), Meta::ReplayGain_Album_Gain, &outputMap );
        if ( !tagMap["REPLAYGAIN_ALBUM_PEAK"].isEmpty() )
            maybeAddPeak( tagMap["REPLAYGAIN_ALBUM_PEAK"].front().toString(), Meta::ReplayGain_Album_Peak, &outputMap );
    }

    return outputMap;
}
#endif
// Bad news: ReplayGain in MP4 is not actually standardized in any way.  Maybe reimplement at some point...maybe.  See
// http://www.hydrogenaudio.org/forums/lofiversion/index.php/t14322.html
#ifdef DO_NOT_USE_THIS_UNTIL_FIXED
static Meta::ReplayGainTagMap readMP4Tags( TagLib::MP4::Tag *tag )
{
    Meta::ReplayGainTagMap outputMap;

    if ( !tag->trackReplayGain().isNull() ) {
        maybeAddGain( tag->trackReplayGain(), Meta::ReplayGain_Track_Gain, &outputMap );
        if ( !tag->trackReplayGainPeak().isNull() )
            maybeAddPeak( tag->trackReplayGainPeak(), Meta::ReplayGain_Track_Peak, &outputMap );
    }

    if ( !tag->albumReplayGain().isNull() ) {
        maybeAddGain( tag->albumReplayGain(), Meta::ReplayGain_Album_Gain, &outputMap );
        if ( !tag->albumReplayGainPeak().isNull() )
            maybeAddPeak( tag->albumReplayGainPeak(), Meta::ReplayGain_Album_Peak, &outputMap );
    }

    return outputMap;
}
#endif 

Meta::ReplayGainTagMap
Meta::readReplayGainTags( TagLib::FileRef fileref )
{
    Meta::ReplayGainTagMap map;
    // NB: we can't get replay gain info from MPC files, since it's stored in some magic place
    //     and not in the APE tags, and taglib doesn't let us access the information (unless
    //     we want to parse the file ourselves).
    // FIXME: should we try getting the info from the MPC APE tag just in case?

    if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
    {
        if ( file->ID3v2Tag() )
            map = readID3v2Tags( file->ID3v2Tag() );
        if ( map.isEmpty() && file->APETag() )
            map = readAPETags( file->APETag() );
    }
    else if ( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
    {
        if ( file->tag() )
            map = readXiphTags( file->tag() );
    }
    else if ( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
    {
        if ( file->xiphComment() )
            map = readXiphTags( file->xiphComment() );
        if ( map.isEmpty() && file->ID3v2Tag() )
            map = readID3v2Tags( file->ID3v2Tag() );
    }
    else if ( TagLib::Ogg::FLAC::File *file = dynamic_cast<TagLib::Ogg::FLAC::File *>( fileref.file() ) )
    {
        if ( file->tag() )
            map = readXiphTags( file->tag() );
    }
    else if ( TagLib::WavPack::File *file = dynamic_cast<TagLib::WavPack::File *>( fileref.file() ) )
    {
        if ( file->APETag() )
            map = readAPETags( file->APETag() );
    }
    else if ( TagLib::TrueAudio::File *file = dynamic_cast<TagLib::TrueAudio::File *>( fileref.file() ) )
    {
        if ( file->ID3v2Tag() )
            map = readID3v2Tags( file->ID3v2Tag() );
    }
    else if ( TagLib::Ogg::Speex::File *file = dynamic_cast<TagLib::Ogg::Speex::File *>( fileref.file() ) )
    {
        if ( file->tag() )
            map = readXiphTags( file->tag() );
    }
    else if ( TagLib::MPC::File *file = dynamic_cast<TagLib::MPC::File *>( fileref.file() ) )
    {
        // This is NOT the correct way to get replay gain tags from MPC files, but
        // taglib doesn't allow us access to the real information.
        // This allows people to work around this issue by copying their replay gain
        // information to the APE tag.
        if ( file->APETag() )
            map = readAPETags( file->APETag() );
    }
#ifdef TAGLIB_EXTRAS_FOUND
    else if ( TagLib::ASF::File *file = dynamic_cast<TagLib::ASF::File *>( fileref.file() ) )
    {
        if ( file->tag() )
            map = readASFTags( file->tag() );
    }
#endif
// See comment above
#ifdef DO_NOT_USE_THIS_UNTIL_FIXED
    else if ( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) )
    {
        if ( file->tag() )
            map = readMP4Tags( file->getMP4Tag() );
    }
#endif
    return map;
}

