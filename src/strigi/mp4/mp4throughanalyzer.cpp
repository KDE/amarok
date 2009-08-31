/****************************************************************************************
 * Copyright (c) 2005-2007 Martin Aumueller <aumuell@reserv.at>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

//#define VERBOSE
#ifdef VERBOSE
#include <iostream>
#include <cstdio>
#endif

#include <string>
#include <sstream>
#include <string.h>
#if 1
#include <strigi/streamthroughanalyzer.h>
#include <strigi/analyzerplugin.h>
#include <strigi/analysisresult.h>
#include <strigi/fieldtypes.h>
#include <strigi/textutils.h>
#else
#include "streamthroughanalyzer.h"
#include "analyzerplugin.h"
#include "analysisresult.h"
#include "fieldtypes.h"
#include "textutils.h"
#endif

#include <QtGlobal>

using namespace Strigi;

class Mp4ThroughAnalyzerFactory;

static const Strigi::RegisteredField *sampleRateField = 0,
             *sampleFormatField = 0,
             *channelsField = 0,
             *audioDurationField = 0,
             *mediaDurationField = 0,
             *videoDurationField = 0,
             *mimeTypeField = 0,
             *audioCodecField = 0,
             *videoCodecField = 0,
             *imageWidthField = 0,
             *imageHeightField = 0,
             *audioGenreField = 0,
             *audioComposerField = 0,
             *audioArtistField = 0,
             *audioAlbumArtistField = 0,
             *audioAlbumField = 0,
             *audioTitleField = 0,
             *audioTrackNumberField = 0,
             *audioDiscNumberField = 0,
             *mediaCodecField = 0,
             *contentCommentField = 0,
             *contentGeneratorField = 0,
             *contentCopyrightField = 0,
             *contentThumbnailField = 0,
             *contentCreationTimeField = 0,
             *contentMaintainerField = 0,
             *contentIdField = 0,
             *contentKeywordField = 0,
             *contentDescriptionField = 0,
             *contentPurchaserField = 0,
             *contentLinksField = 0,
             *contentPurchaseDateField = 0,
             *userRatingField = 0;
        

// Analyzer
class STRIGI_EXPORT Mp4ThroughAnalyzer
    : public Strigi::StreamThroughAnalyzer {
        friend class Mp4ThroughAnalyzerFactory;
private:
    bool haveAudio;
    bool haveVideo;
    bool isQuicktime;

    Strigi::AnalysisResult* analysisResult;
    const Mp4ThroughAnalyzerFactory* factory;

    bool isFullBox(const std::string &type);
    bool parseFullBox(const char *buf, int64_t size, uint8_t *version, uint32_t *flags);
    bool parseBox(const char *buf, int64_t size, const std::string &typepath, int level);
    bool haveSubBoxes(const std::string &type);
    bool readSubBoxes(const char *buf, int64_t size, const std::string &parenttype, int level);
    bool parseFtypBox(const char *buf, int64_t size, const std::string &parenttype, int level);
    bool parseFdhdBox(const char *buf, int64_t size, const std::string &parenttype, int level);
    bool parseMdhdBox(const char *buf, int64_t size, const std::string &parenttype, int level);
    bool parseMvhdBox(const char *buf, int64_t size, const std::string &parenttype, int level);
    bool parseHdlrBox(const char *buf, int64_t size, const std::string &parenttype, int level);
    bool parseHintBox(const char *buf, int64_t size, const std::string &parenttype, int level);
    bool parseStsdBox(const char *buf, int64_t size, const std::string &parenttype, int level);
    bool parseMetaBox(const char *buf, int64_t size, const std::string &parenttype, int level);
    bool parseDataBox(const char *buf, int64_t size, const std::string &parenttype, int level);

public:
    Mp4ThroughAnalyzer(const Mp4ThroughAnalyzerFactory* f)
       :haveAudio(false)
       ,haveVideo(false)
       ,isQuicktime(false)
       ,analysisResult(0)
       ,factory(f) {}
    ~Mp4ThroughAnalyzer() {}
    const char *name() const { return "Mp4"; }
    void setIndexable(Strigi::AnalysisResult* i) {
        analysisResult = i;
    }
    bool isReadyWithStream() {
        return true;
    }
    Strigi::InputStream *connectInputStream(Strigi::InputStream *in);
};

bool Mp4ThroughAnalyzer::isFullBox(const std::string &type)
{
   return type == "mvhd"
      || type == "tkhd"
      || type == "meta"
      ;
}

bool Mp4ThroughAnalyzer::haveSubBoxes(const std::string &type)
{
#if 1
   return type == "moov"
      || type == "trak"
      || type == "edts"
      || type == "mdia"
      || type == "mdhd"
      || type == "minf"
      || type == "dinf"
      || type == "stbl"
      || type == "gmhd"
      || type == "tref"
      || type == "chap"
      || type == "udta"
      || type == "ilst"
      // the following all have a data subbox
      || type == "\251nam"
      || type == "\251ART"
      || type == "aART"
      || type == "\251alb"
      || type == "gnre"
      || type == "trkn"
      || type == "disk"
      || type == "\251day"
      || type == "pgap"
      || type == "apID"
      || type == "cprt"
      || type == "cnID"
      || type == "rtng"
      || type == "atID"
      || type == "plID"
      || type == "geID"
      || type == "sfID"
      || type == "akID"
      || type == "stik"
      || type == "purd"
      || type == "covr"
      ;
#else
   return type != "free"
      && type != "skip"
      && type != "ftyp"
      && type != "mvhd"
      && type != "mdhd"
      && type != "elst"
      && type != "hdlr"
      && type != "smhd"
      && type != "dref"
      && type != "mp4a"
      && type != "stts"
      && type != "stsc"
      && type != "stsz"
      && type != "stsd"
      && type != "stco"
      && type != "stss"
      && type != "meta"
      && type != "mdat"
      && type != "tkhd"
      && type != "iods"
      && type != "data"
      && type != "gmin"
      && type != "text"
      && type != "load"
      && type != "grou"
      && type != "code"
      && type != "name"
      && type != "mean"
      && type != "vmhd"
      && type != "cmvd"
      && type != "clef"
      && type != "prof"
      && type != "enof"
      ;
#endif
}

void indent(int level)
{
      for(int i=0; i<level; ++i)
         fprintf(stderr, "    ");
}

bool Mp4ThroughAnalyzer::readSubBoxes(const char *buf, int64_t size, const std::string &parenttype, int level)
{
   if(level > 15)
      return false;

   int64_t pos = 0;
   while(pos+8 <= size)
   {
      int64_t length = readBigEndianUInt32(buf+pos);
      std::string subtype(buf+pos+4, 4);
      std::string type = parenttype + '.' + subtype;

      int64_t boxoff = 8;
      if(length==0)
      {
         length = size-pos;
#ifdef VERBOSE
         indent(level);fprintf(stderr, "length: till EOF: %ld\n", (long)length);
#endif
      }
      else if(length==1)
      {
         length = readBigEndianUInt64(buf+pos+8);
         boxoff = 16;
         indent(level);fprintf(stderr, "64 bit length: %ld\n", (long)length);
      }

      if(length < boxoff)
      {
         indent(level);
#ifdef VERBOSE
         fprintf(stderr, "box too small at %d: length %d\n", (int)pos, (int)length);
#endif
         break;
      }

#ifdef VERBOSE
      //std::cerr << "length=" << length << ", type=" << t1 << t2 << t3 << t4 << std::endl;
      indent(level);
      fprintf(stderr, "%s (%u bytes at %u)\n", type.substr(type.length()-4).c_str(), (unsigned)length, (unsigned)pos);
#endif
      if(pos + length > size)
      {
         indent(level);
         fprintf(stderr, "%ld excess bytes in %s box\n", (long)(pos+length-size), type.c_str());
      }
      else
      {
         parseBox(buf+pos+boxoff, length-boxoff, type, level);
      }

      pos += length;
   }

   return true;
}

bool Mp4ThroughAnalyzer::parseFtypBox(const char *buf, int64_t size, const std::string &typepath, int level)
{
#ifdef VERBOSE
    std::string majorbrand(buf,4);
    uint32_t majorversion = readBigEndianUInt32(buf+4);
    std::cerr << "ftyp: majorbrand=" << majorbrand << ", vers=" << majorversion << std::endl;
    for(int64_t pos=8; pos+4<=size; pos+=4)
    {
       std::cerr << "ftyp: compbrand=" << std::string(buf+pos,4) << std::endl;
    }
#else
    Q_UNUSED( buf );
    Q_UNUSED( size );
    Q_UNUSED( typepath );
    Q_UNUSED( level );
#endif
    return true;
}

bool Mp4ThroughAnalyzer::parseHintBox(const char *buf, int64_t size, const std::string &typepath, int level)
{
   Q_UNUSED( typepath );
   Q_UNUSED( level );
   uint8_t version;
   uint32_t flags;
   parseFullBox(buf, size, &version, &flags);

   if(version > 0)
      return false;

#ifdef VERBOSE
   uint32_t maxbr = readBigEndianUInt32(buf+8);
   uint32_t avgbr = readBigEndianUInt32(buf+12);

   std::cerr << "bit rate=" << avgbr << " (" << maxbr << " max)" << std::endl;
#endif
   return true;
}

bool Mp4ThroughAnalyzer::parseStsdBox(const char *buf, int64_t size, const std::string &typepath, int level)
{
   Q_UNUSED( typepath );
   Q_UNUSED( level );
   uint8_t version;
   uint32_t flags;
   parseFullBox(buf, size, &version, &flags);

   if(version > 0)
      return false;

   std::string format(buf+12,4);
   if(format == "mp4v" || format == "avc1" || format == "encv" || format == "s263")
   {
      analysisResult->addValue(imageWidthField, readBigEndianUInt16(buf+40));
      analysisResult->addValue(imageHeightField, readBigEndianUInt16(buf+42));
      analysisResult->addValue(videoCodecField, format);
   }
   else  if(format == "mp4a" || format == "enca" || format == "samr" || format == "sawb") 
   {
      uint16_t channels = readBigEndianUInt16(buf+32);
      analysisResult->addValue(channelsField, channels);
      uint64_t samplesize = readBigEndianUInt16(buf+34);
      std::stringstream stream;
      stream << samplesize << " bit int";
      analysisResult->addValue(sampleFormatField, stream.str());
      uint32_t samplerate = readBigEndianUInt32(buf+40);
      analysisResult->addValue(sampleRateField, samplerate>>16);
      analysisResult->addValue(audioCodecField, format);
   }

   return true;
}

bool Mp4ThroughAnalyzer::parseHdlrBox(const char *buf, int64_t size, const std::string &typepath, int level)
{
   Q_UNUSED( typepath );
   Q_UNUSED( level );
   uint8_t version;
   uint32_t flags;
   parseFullBox(buf, size, &version, &flags);

   std::string quicktimetype(buf+4,4);
   std::string subtype(buf+8,4);

   if(subtype == "soun")
      haveAudio = true;
   else if(subtype == "vide")
      haveVideo = true;

   return true;
}


bool Mp4ThroughAnalyzer::parseMdhdBox(const char *buf, int64_t size, const std::string &typepath, int level)
{
   Q_UNUSED( typepath );
   Q_UNUSED( level );
   uint8_t version;
   uint32_t flags;
   parseFullBox(buf, size, &version, &flags);

   uint64_t creationTime = 0;
   uint64_t modTime = 0;
   uint32_t timescale = 0;
   int64_t duration = 0;

   if(version == 1)
   {
      creationTime = readBigEndianUInt64(buf+4);
      modTime = readBigEndianUInt64(buf+12);
      timescale = readBigEndianInt32(buf+20);
      duration = readBigEndianInt64(buf+24);
   }
   else if(version == 0)
   {
      creationTime = readBigEndianUInt32(buf+4);
      modTime = readBigEndianUInt32(buf+8);
      timescale = readBigEndianInt32(buf+12);
      duration = readBigEndianInt32(buf+16);
   }
   else
      return false;

   analysisResult->addValue(mediaDurationField, static_cast<int32_t>(duration/timescale));

   return true;
}

bool Mp4ThroughAnalyzer::parseMvhdBox(const char *buf, int64_t size, const std::string &typepath, int level)
{
   Q_UNUSED( typepath );
   Q_UNUSED( level );
   uint8_t version;
   uint32_t flags;
   parseFullBox(buf, size, &version, &flags);

   uint64_t creationTime = 0;
   uint64_t modTime = 0;
   uint32_t timescale = 0;
   int64_t duration = 0;

   if(version == 1)
   {
      creationTime = readBigEndianUInt64(buf+4);
      modTime = readBigEndianUInt64(buf+12);
      timescale = readBigEndianInt32(buf+20);
      duration = readBigEndianInt64(buf+24);
   }
   else if(version == 0)
   {
      creationTime = readBigEndianUInt32(buf+4);
      modTime = readBigEndianUInt32(buf+8);
      timescale = readBigEndianInt32(buf+12);
      duration = readBigEndianInt32(buf+16);
   }
   else
      return false;

   analysisResult->addValue(videoDurationField, static_cast<int32_t>(duration/timescale));

   return true;
}

bool Mp4ThroughAnalyzer::parseMetaBox(const char *buf, int64_t size, const std::string &typepath, int level)
{
   uint8_t version;
   uint32_t flags;
   parseFullBox(buf, size, &version, &flags);
   if(version == 0)
   {
      return readSubBoxes(buf+4, size-4, typepath, level+1);
   }
   else
      return false;
}

bool Mp4ThroughAnalyzer::parseDataBox(const char *buf, int64_t size, const std::string &typepath, int level)
{
   Q_UNUSED( level );
   std::string shortpath = typepath.substr(0, typepath.length()-10);
   std::string data(buf+8, size-8);
   if(shortpath == "moov.udta.meta.ilst")
   {
      std::string type = typepath.substr(typepath.length()-9, 4);
      if(type == "aART")
         analysisResult->addValue(audioAlbumArtistField, data);
      else if(type == "covr")
         analysisResult->addValue(contentThumbnailField, data);
      else if(type == "cprt")
         analysisResult->addValue(contentCopyrightField, data);
      else if(type == "apID")
         analysisResult->addValue(contentPurchaserField, data);
      else if(type == "purd")
         analysisResult->addValue(contentPurchaseDateField, data);
      else if(type == "keyw")
         analysisResult->addValue(contentKeywordField, data);
      else if(type == "desc")
         analysisResult->addValue(contentDescriptionField, data);
      else if(type == "purl")
         analysisResult->addValue(contentLinksField, data);
      else if(type == "\251nam")
         analysisResult->addValue(audioTitleField, data);
      else if(type == "\251ART")
         analysisResult->addValue(audioArtistField, data);
      else if(type == "\251wrt")
         analysisResult->addValue(audioComposerField, data);
      else if(type == "\251alb")
         analysisResult->addValue(audioAlbumField, data);
      else if(type == "\251cmt")
         analysisResult->addValue(contentCommentField, data);
      else if(type == "\251gen")
         analysisResult->addValue(audioGenreField, data);
      else if(type == "gnre")
      {
         std::ostringstream stream;
         stream << "(" << readBigEndianUInt16(buf+8) << ")";
         analysisResult->addValue(audioGenreField, stream.str());
      }
      else if(type == "\251day")
         analysisResult->addValue(contentCreationTimeField, data);
      else if(type == "\251too")
         analysisResult->addValue(contentGeneratorField, data);
      else if(type == "trkn" || type == "disk")
      {
         std::ostringstream stream;
         stream << readBigEndianUInt16(buf+10) << "/" << readBigEndianUInt16(buf+12);
         analysisResult->addValue(type == "disk" ? audioDiscNumberField : audioTrackNumberField, stream.str());
      }
      else if(type == "tmpo")
      {
         std::ostringstream stream;
         stream << readBigEndianUInt16(buf+8);
         //analysisResult->addValue(audioTempoField, stream.str());
         //fprintf(stderr, "tempo: %s\n",stream.str().c_str());
      }
      else if(type == "rtng")
      {
         analysisResult->addValue(userRatingField, readBigEndianUInt16(buf+8));
      }
      else if(type == "cpil")
      {
         //analysisResult->addValue(audioCompilationField, buf[8] ? "true" : "false");
      }
      else if(type == "pgap")
      {
         //analysisResult->addValue(audioGaplessField, buf[8] ? "true" : "false");
      }
      else if(type == "\251grp")
      {
         //analysisResult->addValue(audioGroupingField, data);
      }
      else if(type == "----")
      {
      }
      else
      {
#ifdef VERBOSE
         //fprintf(stderr, "%s: %s\n", typepath.c_str(), data.c_str());
#endif
      }

   }
   else
   {
#ifdef VERBOSE
         fprintf(stderr, "data with unknown path %s: %s\n", typepath.c_str(), data.c_str());
#endif
   }

   return true;

}

bool Mp4ThroughAnalyzer::parseFullBox(const char *buf, int64_t size, uint8_t *version, uint32_t *flags)
{
   Q_UNUSED( size );
   *flags = readBigEndianUInt32(buf);
   *flags &= 0xffffff;
   *version = *buf;
   return true;
}

bool
Mp4ThroughAnalyzer::parseBox(const char *buf, int64_t size, const std::string &typepath, int level)
{
   std::string type(typepath.substr(typepath.length()-4));

   if(type == "ftyp")
   {
         return parseFtypBox(buf, size, typepath, level+1);
   }
   else if(type == "mdhd")
   {
         return parseMdhdBox(buf, size, typepath, level+1);
   }
   else if(type == "mvhd")
   {
         return parseMvhdBox(buf, size, typepath, level+1);
   }
   else if(type == "hdlr")
   {
         return parseHdlrBox(buf, size, typepath, level+1);
   }
   else if(type == "hint")
   {
         return parseHintBox(buf, size, typepath, level+1);
   }
   else if(type == "stsd")
   {
         return parseStsdBox(buf, size, typepath, level+1);
   }
   else if(type == "meta")
   {
         return parseMetaBox(buf, size, typepath, level+1);
   }
   else if(type == "data")
   {
         return parseDataBox(buf, size, typepath, level+1);
   }
   else if(haveSubBoxes(type))
   {
      return readSubBoxes(buf, size, typepath, level+1);
   }
   return false;
}

InputStream*
Mp4ThroughAnalyzer::connectInputStream(InputStream* in) {
    if (in == 0) return in;
    const int32_t nreq = 8;
    const char* buf;
    int32_t nread = in->read(buf, nreq, nreq);
    in->reset(0);

    if (nread < nreq) {
        return in;
    }

    // only try this on mp4 and mov files
    if (!strncmp(buf+4, "moov", 4)) {
       isQuicktime = true;
    } else if(!strncmp(buf+4, "ftyp", 4)) {
    } else
    {
       return in;
    }

    int64_t filepos = 0;
    while(in->size() == -1 || filepos < in->size())
    {
        int32_t nreq = filepos + 2*4;
        if (nreq < 0) { // overflow
            return in;
        }
        nread = in->read(buf, nreq, nreq);
        in->reset(0);
        if (nread < nreq) {
            return in;
        }

        uint32_t boxlength = readBigEndianUInt32(buf+filepos); // this includes the 8 header bytes
        std::string type(buf+filepos+4,4);
        if(boxlength==0)
        {
           boxlength = in->size()-filepos;
        }
        nreq = filepos+boxlength;
        if (nreq < 0) { // overflow
            return in;
        }
        nread = in->read(buf, nreq, nreq);
        in->reset(0);
        if (nread < nreq) {
            return in;
        }
#ifdef VERBOSE
        //std::cerr << "length=" << length << ", type=" << t1 << t2 << t3 << t4 << std::endl;
        fprintf(stderr, "%s (%ld bytes at %ld)\n", type.c_str(), (long)boxlength, (long)filepos);
#endif
        parseBox(buf+filepos+8, boxlength-8, type, 0);

        filepos += boxlength;
    }

    if(isQuicktime)
        analysisResult->addValue(mimeTypeField, "video/quicktime");
    else if(haveVideo)
        analysisResult->addValue(mimeTypeField, "video/mp4");
    else if(haveAudio)
        analysisResult->addValue(mimeTypeField, "audio/mp4");

    return in;
}

class Mp4ThroughAnalyzerFactory
    : public Strigi::StreamThroughAnalyzerFactory {
friend class Mp4ThroughAnalyzer;
private:

    const char* name() const {
        return "Mp4ThroughAnalyzer";
    }
    Strigi::StreamThroughAnalyzer* newInstance() const {
        return new Mp4ThroughAnalyzer(this);
    }
    void registerFields(Strigi::FieldRegister &reg) {
        mimeTypeField = reg.registerField("content.mime_type", FieldRegister::stringType, 1, 0);
        audioGenreField = reg.registerField("content.genre", FieldRegister::stringType, 1, 0);
        audioTitleField = reg.registerField("audio.title", FieldRegister::stringType, 1, 0);
        audioTrackNumberField = reg.registerField("TODO_trackNumber", FieldRegister::stringType, 1, 0);
        audioDiscNumberField = reg.registerField("TODO_discNumber", FieldRegister::stringType, 1, 0);
        //audioComposerField = reg.registerField("audio.composer", FieldRegister::stringType, 1, 0);
        audioComposerField = reg.registerField("content.author", FieldRegister::stringType, 1, 0);
        contentCommentField = reg.registerField("content.comment", FieldRegister::stringType, 1, 0);
        audioArtistField = reg.registerField("audio.artist", FieldRegister::stringType, 1, 0);
        audioAlbumField = reg.registerField("audio.album", FieldRegister::stringType, 1, 0);
        audioAlbumArtistField = reg.registerField("TODO_audio.albumartist", FieldRegister::stringType, 1, 0);
        mediaCodecField = reg.registerField("media.codec", FieldRegister::stringType, 1, 0);
        contentPurchaserField = reg.registerField("content.links", FieldRegister::stringType, 1, 0); // this is necessary in order to transfer the drm'ed file to an ipod
        contentPurchaserField = reg.registerField("TODO_content.purchaser", FieldRegister::stringType, 1, 0); // this is necessary in order to transfer the drm'ed file to an ipod
        contentPurchaseDateField = reg.registerField("TODO_content.purchasedate", FieldRegister::datetimeType, 1, 0);
        contentKeywordField = reg.registerField("content.keyword", FieldRegister::stringType, 1, 0);
        contentDescriptionField = reg.registerField("content.description", FieldRegister::stringType, 1, 0);
        contentGeneratorField = reg.registerField("content.generator", FieldRegister::stringType, 1, 0);
        audioDurationField = reg.registerField("audio.duration", FieldRegister::integerType, 1, 0);
        mediaDurationField = reg.registerField("media.duration", FieldRegister::integerType, 1, 0);
        videoDurationField = reg.registerField("TODO_video.duration", FieldRegister::integerType, 1, 0);
        audioCodecField = reg.registerField("av.audio_codec", FieldRegister::stringType, 1, 0);
        videoCodecField = reg.registerField("av.video_codec", FieldRegister::stringType, 1, 0);
        contentCopyrightField = reg.registerField("content.copyright", FieldRegister::stringType, 1, 0);
        contentThumbnailField = reg.registerField("content.thumbnail", FieldRegister::binaryType, 1, 0);
        contentCreationTimeField = reg.registerField("content.creation_time", FieldRegister::datetimeType, 1, 0);
        contentMaintainerField = reg.registerField("content.maintainer", FieldRegister::stringType, 1, 0);
        contentIdField = reg.registerField("content.ID", FieldRegister::stringType, 1, 0);
        userRatingField = reg.registerField("user.rating", FieldRegister::integerType, 1, 0);
        imageWidthField = reg.registerField("image.width", FieldRegister::integerType, 1, 0);
        imageHeightField = reg.registerField("image.height", FieldRegister::integerType, 1, 0);

        channelsField = reg.registerField("audio.channel_count", FieldRegister::integerType, 1, 0);
        sampleRateField = reg.registerField("media.sample_rate", FieldRegister::integerType, 1, 0);
        sampleFormatField = reg.registerField("media.sample_format", FieldRegister::integerType, 1, 0);
    }
};

//Factory
class Mp4Factory : public AnalyzerFactoryFactory {
public:
    std::list<StreamThroughAnalyzerFactory*>
    streamThroughAnalyzerFactories() const {
        std::list<StreamThroughAnalyzerFactory*> af;
        af.push_back(new Mp4ThroughAnalyzerFactory());
        return af;
    }
};

STRIGI_ANALYZER_FACTORY(Mp4Factory)
