/*
 * ASF compatible decoder.
 * Copyright (c) 2000, 2001 Fabrice Bellard.
 * Modifications (Header parsing only version) Copyright (c) 2005 Umesh Shankar.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//#include "avformat.h"
//#include "avi.h"
//#include "mpegaudio.h"
#include "asf.h"
#include "wmafile.h"
#include "wmatag.h"

using namespace TagLib;

#undef NDEBUG
#include <assert.h>

#define FRAME_HEADER_SIZE 17
// Fix Me! FRAME_HEADER_SIZE may be different. 

static const GUID index_guid = {
    0x33000890, 0xe5b1, 0x11cf, { 0x89, 0xf4, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xcb },
};

/**********************************/
/* decoding */

//#define DEBUG

static uint64_t get_le64(WMA::File *f)
{
    ByteVector bv = f->readBlock(8);
    return bv.toLongLong(false);  
}

static unsigned int get_le32(WMA::File *f)
{
    ByteVector bv = f->readBlock(4);
    return bv.toUInt(false);
}

static unsigned int get_le16(WMA::File *f)
{
    ByteVector bv = f->readBlock(2);
    return bv.toShort(false);
}

static int get_byte(WMA::File *f)
{
    ByteVector bv = f->readBlock(1);
    return bv[0];
}

static void get_guid(WMA::File *f, GUID *g)
{
    int i;

    g->v1 = get_le32(f);
    g->v2 = get_le16(f);
    g->v3 = get_le16(f);
    for(i=0;i<8;i++)
        g->v4[i] = get_byte(f);
}

#if 0
static void get_str16(WMA::File *f, String& s)
{
    int len; //, c;
    //  char *q;

    len = get_le16(f);

    ByteVector bv = f->readBlock(len * 2); // len is number of wide characters (see below)
    bv.append( ByteVector::fromShort(0) ); // NULL terminator
    s = String((wchar_t *) bv.data(), String::UTFLE16);

    //   q = buf;
    //   while (len > 0) {
    //     c = get_le16(f);
    //     if ((q - buf) < buf_size - 1)
    //             *q++ = c;
    //     len--;
    //   }
    //   *q = '\0';
}
#endif

static void get_str16_nolen(WMA::File *f, int len, String& s)
{
    ByteVector bv = f->readBlock(len); // len here is apparently in bytes (see code below)
    bv.append( ByteVector::fromShort(0) ); // NULL terminator
    s = String(bv, String::UTF16LE);

    //   int c;
    //   char *q;

    //   q = buf;
    //   while (len > 0) {
    //     c = get_le16(f);
    //     if ((q - buf) < buf_size - 1)
    //             *q++ = c;
    //     len-=2;
    //   }
    //   *q = '\0';
}



/* We could be given one of the three possible structures here:
 * WAVEFORMAT, PCMWAVEFORMAT or WAVEFORMATEX. Each structure
 * is an expansion of the previous one with the fields added
 * at the bottom. PCMWAVEFORMAT adds 'WORD wBitsPerSample' and
 * WAVEFORMATEX adds 'WORD  cbSize' and basically makes itself
 * an openended structure.
 */
void WMA::File::getWavHeader(int size)
{
    int id;

    if ( properties ) {
        id = get_le16(this);
        properties->m_channels = get_le16(this);
        properties->m_sampleRate = get_le32(this);
        properties->m_bitrate = (get_le32(this) * 8) / 1000; // in kb/s
        /*codec->block_align = */get_le16(this);
    } else {
        seek(2 + 2 + 4 + 4 + 2, TagLib::File::Current);
    }

    if (size == 14) {  /* We're dealing with plain vanilla WAVEFORMAT */
        //      codec->bits_per_sample = 8;
    } else
        /* codec->bits_per_sample = */ get_le16(this);

    if (size > 16) {  /* We're obviously dealing with WAVEFORMATEX */
        unsigned int extradata_size = get_le16(this);
        if (extradata_size > 0) {
            if (extradata_size > size - 18)
                extradata_size = size - 18;
            seek(extradata_size, TagLib::File::Current); // Just skip the extra data
        } else {
            extradata_size = 0;
        }

        /* It is possible for the chunk to contain garbage at the end */
        if (size - extradata_size - 18 > 0)
            seek(size - extradata_size - 18, TagLib::File::Current);
    }
}


int WMA::File::asfReadHeader()
{
    ASFContext asf;
    GUID g;
    int size, i;
    int64_t gsize;

    get_guid(this, &g);
    if (memcmp(&g, &asf_header, sizeof(GUID)))
        goto fail;
    get_le64(this);
    get_le32(this);
    get_byte(this);
    get_byte(this);
    //  memset(&asf->asfid2avid, -1, sizeof(asf->asfid2avid));
    for(;;) {
        get_guid(this, &g);
        gsize = get_le64(this);
        // #ifdef DEBUG
        //     printf("%08Lx: ", url_ftell(f) - 24);
        //     print_guid(&g);
        //     printf("  size=0x%Lx\n", gsize);
        // #endif
        if (gsize < 24)
            goto fail;
        if (!memcmp(&g, &file_header, sizeof(GUID))) {
            get_guid(this, &g);
            // asf.hdr.file_size		= 
            get_le64(this);
            // asf.hdr.create_time	=
            get_le64(this);
            // asf.hdr.packets_count	=
            get_le64(this);
            // asf.hdr.play_time	
            int64_t play_time = get_le64(this);
            // play_time is in 100ns = 10^-7s units
            if (properties) 
                properties->m_length = (play_time / 10000000L);
            // asf.hdr.send_time		=
            get_le64(this);
            // asf.hdr.preroll		=
            get_le32(this);
            // asf.hdr.ignore		=
            get_le32(this);
            // asf.hdr.flags		=
            get_le32(this);
            // asf.hdr.min_pktsize	=
            get_le32(this);
            // asf.hdr.max_pktsize	=
            get_le32(this);
            // asf.hdr.max_bitrate	=
            get_le32(this);
            // asf.packet_size = asf.hdr.max_pktsize;
            // asf.nb_packets = asf.hdr.packets_count;
        } else if (!memcmp(&g, &stream_header, sizeof(GUID))) {
            int type, total_size, type_specific_size;
            unsigned int tag1;
            int64_t pos1, pos2;

            pos1 = tell();

            get_guid(this, &g);
            if (!memcmp(&g, &audio_stream, sizeof(GUID))) {
                //        type = CODEC_TYPE_AUDIO;
            } else {
                //debug("TagLib::WMA: File contains non-audio streams!");
                return false;
            }

            get_guid(this, &g);
            total_size = get_le64(this);
            type_specific_size = get_le32(this);
            get_le32(this);
            get_le16(this) & 0x7f; /* stream id */

            get_le32(this);
            //       st->codec.codec_type = type;
            //       /* 1 fps default (XXX: put 0 fps instead) */
            //       st->codec.frame_rate = 1000; 
            //       st->codec.frame_rate_base = 1;

            //      if (type == CODEC_TYPE_AUDIO) {
            getWavHeader(type_specific_size);
            //        st->need_parsing = 1;
            /* We have to init the frame size at some point .... */
            pos2 = tell();
            if (gsize > (pos2 + 8 - pos1 + 24)) {
                /* asf_st.ds_span = */ get_byte(this);
                /* asf_st.ds_packet_size = */ get_le16(this);
                // asf_st.ds_chunk_size = 
                get_le16(this);
                //  asf_st.ds_data_size =
                get_le16(this);
                // asf_st.ds_silence_data =
                get_byte(this);
            }

            pos2 = tell();
            seek(gsize - (pos2 - pos1 + 24), TagLib::File::Current);
        } else if (!memcmp(&g, &data_header, sizeof(GUID))) {
            break;
        } else if (!memcmp(&g, &comment_header, sizeof(GUID))) {
            int len1, len2, len3, len4, len5;

            len1 = get_le16(this);
            len2 = get_le16(this);
            len3 = get_le16(this);
            len4 = get_le16(this);
            len5 = get_le16(this);

            if (wmaTag) {
                String s;
                char buf[1];

                get_str16_nolen(this, len1, s);
                wmaTag->setTitle(s);

                get_str16_nolen(this, len2, s);
                wmaTag->setArtist(s);

                get_str16_nolen(this, len3, s); // Copyright notice

                get_str16_nolen(this, len4, s);
                wmaTag->setComment(s);

                seek(len5, TagLib::File::Current);
            } else {
                seek(len1+len2+len3+len4+len5, TagLib::File::Current);
            }
        } else if (!memcmp(&g, &extended_content_header, sizeof(GUID))) {
            int desc_count, i;

            desc_count = get_le16(this);
            for(i=0;i<desc_count;i++)
            {
                int name_len,value_type,value_len = 0;
                int64_t value_num;
                String name, value;

                name_len = get_le16(this);
                get_str16_nolen(this, name_len, name);
                value_type = get_le16(this);
                value_len = get_le16(this);

                //debug (name);
                //                printf ("value_type is %d; name_len is %d\n", value_type, name_len);

                if ((value_type == 0) || (value_type == 1)) // unicode or byte
                {
                    get_str16_nolen(this, value_len, value);
                    //debug ("string value:");
                    //debug(value);
                    if ( wmaTag ) {
                        if (strcmp(name.toCString(),"WM/AlbumTitle")==0) { wmaTag->setAlbum(value); }
                        if (strcmp(name.toCString(),"WM/Genre")==0) { wmaTag->setGenre(value); }
                        if (strcmp(name.toCString(),"WM/Year")==0) { wmaTag->setYear(value.toInt()); }
                    }
                }
                if ((value_type >= 2) || (value_type <= 5)) // boolean or DWORD or QWORD or WORD
                {
                    if (value_type==2) value_num = get_le32(this);
                    if (value_type==3) value_num = get_le32(this);
                    if (value_type==4) value_num = get_le64(this);
                    if (value_type==5) value_num = get_le16(this);

                    //printf("numerical value: %d\n", value_num);                          
                    if (wmaTag) {
                        if (strcmp(name.toCString(),"WM/Track")==0) wmaTag->setTrack(value_num + 1);
                        if (strcmp(name.toCString(),"WM/TrackNumber")==0) wmaTag->setTrack(value_num);
                    }
                }
            }
#if 0
        } else if (!memcmp(&g, &head1_guid, sizeof(GUID))) {
            int v1, v2;
            get_guid(f, &g);
            v1 = get_le32(this);
            v2 = get_le16(this);
        } else if (!memcmp(&g, &codec_comment_header, sizeof(GUID))) {
            int len, v1, n, num;
            char str[256], *q;
            char tag[16];

            get_guid(this, &g);
            print_guid(&g);

            n = get_le32(this);
            for(i=0;i<n;i++) {
                num = get_le16(this); /* stream number */
                get_str16(this, str, sizeof(str));
                get_str16(this, str, sizeof(str));
                len = get_le16(this);
                q = tag;
                while (len > 0) {
                    v1 = get_byte(this);
                    if ((q - tag) < sizeof(tag) - 1)
                        *q++ = v1;
                    len--;
                }
                *q = '\0';
            }
#endif
            // FIXME: Can we eliminate all further reads?

            // FIXME: implement EOF check
            //     } else if (url_feof(f)) {
            //       goto fail;
        } else {
            seek(gsize - 24, TagLib::File::Current);
        }
        }
        get_guid(this, &g);
        get_le64(this);
        get_byte(this);
        get_byte(this);
        // FIXME: implement EOF check
        //   if (url_feof(f))
        //           goto fail;
        //   asf->data_offset = url_ftell(f);
        //   asf->packet_size_left = 0;

        return true;

fail:
        return false;
        //   for(i=0;i<s->nb_streams;i++) {
        //     AVStream *st = s->streams[i];
        //     if (st) {
        //       av_free(st->priv_data);
        //       av_free(st->codec.extradata);
        //     }
        //     av_free(st);
        //   }
        //   return -1;
    }
