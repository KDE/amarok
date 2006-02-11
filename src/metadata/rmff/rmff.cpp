/***************************************************************************
 *   Copyright (C) 2005 Paul Cifarelli                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin St, 5th fl, Boston, MA 02110-1301,      *
 *   USA, or check http://www.fsf.org/about/contact.html                   *
 *                                                                         *
 *   Note that no RealNetworks code appears or is duplicated, copied, or   *
 +   used as a template in this code.  The code was written from scratch   *
 *   using the reference documentation found at:                           *
 *                                                                         *
 *   https://common.helixcommunity.org/nonav/2003/HCS_SDK_r5/helixsdk.htm  *
 *                                                                         *
 ***************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <taglib.h>
#include <id3v1tag.h>
#include <id3v2tag.h>
#include <tfile.h>
#include <fileref.h>
#include <iostream>

#include "rmff.h"

#define UNPACK4(a, buf, i) memcpy((void *)&a, (void *) &buf[i], 4),i+=4,a=ntohl(a)
#define UNPACK2(a, buf, i) memcpy((void *)&a, (void *) &buf[i], 2),i+=2,a=ntohs(a)

using namespace TagLib;
using namespace TagLib::RealMedia;

RMFFile::RMFFile(const char *filename) : File(filename), m_id3tag(0) 
{ 
   if (isOpen()) 
      m_id3tag = new ID3v1::Tag(this, length() - 128); 
}
   
RMFFile::~RMFFile() 
{ 
   delete m_id3tag; 
}

bool RMFFile::save() 
{ 
   ByteVector bv = m_id3tag->render(); //TODO finish this
   return false; 
} 


String RealMediaFF::title () const
{
   return !m_err && m_id3v1tag ? m_id3v1tag->tag()->title() : "";
}

String RealMediaFF::artist () const
{
   return !m_err && m_id3v1tag ? m_id3v1tag->tag()->artist() : "";
}

String RealMediaFF::album () const
{
   return !m_err && m_id3v1tag ? m_id3v1tag->tag()->album() : "";
}

String RealMediaFF::comment() const
{
   return !m_err && m_id3v1tag ? m_id3v1tag->tag()->comment() : "";
}

String RealMediaFF::genre() const
{
   return !m_err && m_id3v1tag ? m_id3v1tag->tag()->genre() : "";
}

TagLib::uint   RealMediaFF::year() const
{
   return !m_err && m_id3v1tag ? m_id3v1tag->tag()->year() : 0;
}

TagLib::uint   RealMediaFF::track() const
{
   return !m_err && m_id3v1tag ? m_id3v1tag->tag()->track() : 0;
}

// properties
int RealMediaFF::length () const
{
   return m_readProperties && !m_err && m_props ? m_props->duration : 0;
}

int RealMediaFF::bitrate () const
{
   return m_readProperties && !m_err && m_props ? m_props->avg_bit_rate : 0;
}

int RealMediaFF::sampleRate () const
{
   return 0;
}

int RealMediaFF::channels () const
{
   return 0; 
}


RealMediaFF::RealMediaFF(const char *file, bool readProperties, AudioProperties::ReadStyle /*propertiesStyle*/)
   : m_filename(0), m_head(0), m_tail(0), m_err(0), media_hdrs(0), m_id3v1tag(0), 
     m_flipYearInMetadataSection(0), m_readProperties(readProperties)
{
   m_filename = strdup(file);

   m_fd = open(m_filename, O_RDONLY);
   if (m_fd < 0)
   {
      m_err = -1;
      return;
   }

   // ok, for RM files, the properties are embedded, so we ignore propertiesStyle
   if (m_readProperties)
   {
      init();

      // and now for the really complicated stuff...
      if (initMetadataSection())
         std::cerr << "ERROR reading Metadata\n";
   }

   // now get the ID3v1 tag at the end of this file
   m_id3v1tag = new RMFFile(m_filename);
}


RealMediaFF::RealMediaFF(RealMediaFF &src)
   : m_filename(0), m_head(0), m_tail(0), m_err(0), media_hdrs(0), m_id3v1tag(0), 
     m_flipYearInMetadataSection(0), m_readProperties(src.m_readProperties)
{
   m_filename=strdup(src.m_filename);

   m_fd = open(m_filename, O_RDONLY);
   if (m_fd < 0)
   {
      m_err = -1;
      return;
   }

   // ok, for RM files, the properties are embedded, so we ignore propertiesStyle
   if (m_readProperties)
   {
      init();

      // and now for the really complicated stuff...
      if (initMetadataSection())
         std::cerr << "ERROR reading Metadata\n";
   }

   // now get the ID3v1 tag at the end of this file
   m_id3v1tag = new RMFFile(m_filename);
}

RealMediaFF::~RealMediaFF()
{
   ::free(m_filename);

   Collectable *hdr = m_head, *next;
   while (hdr)
   {
      next = hdr->fwd;
      delete hdr;
      hdr = next;
   }

   delete m_id3v1tag;

   close(m_fd);
}

bool RealMediaFF::isEmpty() const 
{ 
   return m_id3v1tag->tag()->isEmpty(); 
}
         

void RealMediaFF::saveHeader(Collectable *hdr)
{
   hdr->fwd = 0;
   if (!m_head)
      m_head = m_tail = hdr;
   else
   {
      m_tail->fwd = hdr;
      m_tail = hdr;
   }
}


int RealMediaFF::init()
{
   int nbytes;
   unsigned char buf[65535];
   UINT32 object_id;
   UINT32 sz;
   UINT32 consumed = 0;

   off_t s;
   if ( (s = lseek(m_fd, 0, SEEK_SET)) )
   {
      m_err = -1;
      return m_err;
   }

   m_hdr = new File_Header_v0_v1;
   nbytes = getChunk(buf, 65536, m_hdr->s.object_id, m_hdr->s.size, consumed);
   if (nbytes < 0 || m_hdr->s.size != consumed || memcmp((void *)&m_hdr->s.object_id, ".RMF", 4))
   {
      //std::cerr << "SERIOUS ERROR - not likely a RealMedia file\n";
      m_err = -1;
      return m_err;
   }
   if (!getRealFileHeader(m_hdr, buf, m_hdr->s.object_id, m_hdr->s.size))
   {
      saveHeader(m_hdr);
      consumed = 0;
      nbytes = getChunk(buf, 65536, object_id, sz, consumed);
      if (nbytes < 0 || sz != consumed)
      {
         m_err = -1;
         return m_err;
      }

      while (!m_err && memcmp((void *)&object_id, "DATA", 4))
      {
         char oid[5];
         memcpy((void *)oid, (void *)&object_id, 4);
         oid[4] = 0;
         if (!memcmp((void *)&object_id, "PROP", 4))
         {
            m_props = new RMProperties;
            getRealPropertyHeader(m_props, buf, object_id, sz);
            saveHeader(m_props);
         }

         if (!memcmp((void *)&object_id, "MDPR", 4))
         {
            media_hdrs = new MediaProperties;
            getMediaPropHeader(media_hdrs, buf, object_id, sz);
            saveHeader(media_hdrs);
         }

         if (!memcmp((void *)&object_id, "CONT", 4))
         {
            m_contenthdr = new ContentDescription;
            getContentDescription(m_contenthdr, buf, object_id, sz);
            saveHeader(m_contenthdr);
         }

         consumed = 0;
         do
         {
            nbytes = getChunk(buf, 65536, object_id, sz, consumed);
         } while ( !m_err && memcmp((void *)&object_id, "DATA", 4) && (consumed < sz) );
      }
   }

   return 0;
}

int RealMediaFF::getHdr(unsigned char *buf, size_t sz, UINT32 &fourcc, UINT32 &csz)
{
   int nbytes = 0, i = 0;

   if (sz < (size_t)RMFF_HDR_SIZE)
      return 0;

   if ( (nbytes = read(m_fd, (void *) buf, RMFF_HDR_SIZE)) != RMFF_HDR_SIZE )
   {
      m_err = -1;

      return (nbytes);
   }

   memcpy((void *)&fourcc, buf, 4); i+=4;
   UNPACK4(csz,buf,i);

   return nbytes;
}

int RealMediaFF::getChunk(unsigned char *buf, size_t sz, UINT32 &fourcc, UINT32 &csz, UINT32 &alreadyconsumed)
{
   int nbytes = 0, i = 0, readamount;
   csz = 0;

   if (!alreadyconsumed)
   {
      if ( (nbytes = getHdr(buf, sz, fourcc, csz)) != RMFF_HDR_SIZE )
      {
         m_err = -1;
         alreadyconsumed += nbytes > 0 ? nbytes : 0;
         return nbytes;
      }
      alreadyconsumed += RMFF_HDR_SIZE;
      readamount = csz - RMFF_HDR_SIZE;
      i = RMFF_HDR_SIZE;
   }
   else
      readamount = csz - alreadyconsumed;

   if ( (nbytes = read(m_fd, (void *) &buf[i], readamount > (int)sz - i ? (int)sz - i : readamount )) != readamount )
   {
      if (nbytes < 0)
      {
         m_err = -1;
      }
      else
         alreadyconsumed += nbytes;

      return (nbytes<0 ? i : i + nbytes);
   }

   alreadyconsumed += nbytes;
   return (csz);
}

int RealMediaFF::getRealFileHeader(File_Header_v0_v1 *hdr, const unsigned char *buf, UINT32 object_id, int sz)
{
   int i = 0;

   // RealMedia header
   hdr->s.object_id = object_id;
   hdr->s.size = sz;

   i = RMFF_HDR_SIZE;
   UNPACK2(hdr->object_version, buf, i);

   if ( !strncmp((const char *) &hdr->s.object_id, ".RMF", 4) && 
        (hdr->object_version == 0 || hdr->object_version == 1) )
   {
      UNPACK4(hdr->file_version, buf, i);
      UNPACK4(hdr->num_headers, buf, i);
   }
   return 0;
}

int RealMediaFF::getRealPropertyHeader(RMProperties *props, const unsigned char *buf, UINT32 object_id, int sz)
{
   int i = 0;

   // Properties
   props->s.object_id = object_id;
   props->s.size = sz;

   i = RMFF_HDR_SIZE;
   UNPACK2(props->object_version, buf, i);
   
   if ( !strncmp((const char *)&props->s.object_id,"PROP",4) && (props->object_version == 0) )
   {
      UNPACK4(props->max_bit_rate, buf, i);
      UNPACK4(props->avg_bit_rate, buf, i);
      UNPACK4(props->max_packet_size, buf, i);
      UNPACK4(props->avg_packet_size, buf, i);
      UNPACK4(props->num_packets, buf, i);
      UNPACK4(props->duration, buf, i);
      UNPACK4(props->preroll, buf, i);
      UNPACK4(props->index_offset, buf, i);
      UNPACK4(props->data_offset, buf, i);
      UNPACK2(props->num_streams, buf, i);
      UNPACK2(props->flags, buf, i);
   }
   return 0;
}


int RealMediaFF::getMediaPropHeader(MediaProperties *media_hdr, const unsigned char *buf, UINT32 object_id, int sz)
{
   int i = 0;

   // Properties
   media_hdr->s.object_id = object_id;
   media_hdr->s.size = sz;

   i = RMFF_HDR_SIZE;
   UNPACK2(media_hdr->object_version, buf, i);

   if ( !strncmp((const char *)&media_hdr->s.object_id, "MDPR", 4) && media_hdr->object_version == 0)
   {
      UNPACK2(media_hdr->stream_number, buf, i);
      UNPACK4(media_hdr->max_bit_rate, buf, i);
      UNPACK4(media_hdr->avg_bit_rate, buf, i);
      UNPACK4(media_hdr->max_packet_size, buf, i);
      UNPACK4(media_hdr->avg_packet_size, buf, i);
      UNPACK4(media_hdr->start_time, buf, i);
      UNPACK4(media_hdr->preroll, buf, i);
      UNPACK4(media_hdr->duration, buf, i);
      media_hdr->stream_name_size = buf[i]; i++;
      memcpy(media_hdr->stream_name, &buf[i], media_hdr->stream_name_size);
      media_hdr->stream_name[media_hdr->stream_name_size] = 0;
      i += media_hdr->stream_name_size;
      media_hdr->mime_type_size = buf[i]; i++;
      memcpy(media_hdr->mime_type, &buf[i], media_hdr->mime_type_size);
      i += media_hdr->mime_type_size;
      UNPACK4(media_hdr->type_specific_len, buf, i);
      if (media_hdr->type_specific_len)
      {
         media_hdr->type_specific_data = new UINT8[media_hdr->type_specific_len];
         memcpy(media_hdr->type_specific_data, &buf[i], media_hdr->type_specific_len);

         if (!strncmp((const char *)media_hdr->mime_type, "logical-fileinfo", 16))
         {
            media_hdr->lstr = new LogicalStream;
            UNPACK4(media_hdr->lstr->size, buf, i);
            UNPACK2(media_hdr->lstr->object_version, buf, i);
            if (media_hdr->lstr->object_version == 0)
            {
               UNPACK2(media_hdr->lstr->num_physical_streams, buf, i);
               if (media_hdr->lstr->num_physical_streams > 0)
               {
                  media_hdr->lstr->physical_stream_numbers = new UINT16[ media_hdr->lstr->num_physical_streams ];
                  media_hdr->lstr->data_offsets = new UINT32[ media_hdr->lstr->num_physical_streams ];
                  for (int j=0; j<media_hdr->lstr->num_physical_streams; j++)
                  {
                     UNPACK2(media_hdr->lstr->physical_stream_numbers[j], buf, i);
                  }
                  for (int j=0; j<media_hdr->lstr->num_physical_streams; j++)
                  {
                     UNPACK4(media_hdr->lstr->data_offsets[j], buf, i);
                  }
               }

               UNPACK2(media_hdr->lstr->num_rules, buf, i);
               if (media_hdr->lstr->num_rules > 0)
               {
                  media_hdr->lstr->rule_to_physical_stream_number_map =  new UINT16[ media_hdr->lstr->num_rules ];
                  for (int j=0; j<media_hdr->lstr->num_rules; j++)
                  {
                     UNPACK2(media_hdr->lstr->rule_to_physical_stream_number_map[j], buf, i);
                  }
               }
               UNPACK2(media_hdr->lstr->num_properties, buf, i);
               if (media_hdr->lstr->num_properties > 0)
               {
                  media_hdr->lstr->properties = new NameValueProperty[ media_hdr->lstr->num_properties ];
                  for (int j=0; j<media_hdr->lstr->num_properties; j++)
                  {
                     UNPACK4(media_hdr->lstr->properties[j].size, buf, i);
                     UNPACK2(media_hdr->lstr->properties[j].object_version, buf, i);
                     if (media_hdr->lstr->properties[j].object_version == 0)
                     {
                        media_hdr->lstr->properties[j].name_length = buf[i]; i++;
                        if (media_hdr->lstr->properties[j].name_length)
                        {
                           media_hdr->lstr->properties[j].name = new UINT8[ media_hdr->lstr->properties[j].name_length + 1];
                           memcpy((void *)media_hdr->lstr->properties[j].name, (void *)&buf[i], 
                                  media_hdr->lstr->properties[j].name_length);
                           media_hdr->lstr->properties[j].name[ media_hdr->lstr->properties[j].name_length ] = 0;
                           i+=media_hdr->lstr->properties[j].name_length;
                        }

                        UNPACK4(media_hdr->lstr->properties[j].type, buf, i);
                        UNPACK2(media_hdr->lstr->properties[j].value_length, buf, i);
                        if (media_hdr->lstr->properties[j].value_length)
                        {
                           media_hdr->lstr->properties[j].value_data = new UINT8[ media_hdr->lstr->properties[j].value_length + 1];
                           memcpy((void *)media_hdr->lstr->properties[j].value_data, (void *)&buf[i], 
                                  media_hdr->lstr->properties[j].value_length);                           
                           media_hdr->lstr->properties[j].value_data[ media_hdr->lstr->properties[j].value_length ] = 0;
                           i+=media_hdr->lstr->properties[j].value_length;
                        }  
                     }
                  }
               }
            }
            else
               media_hdr->lstr = 0;
         }
      }
      else
         media_hdr->type_specific_data = 0;
   }
   else
   {
      m_err = -1;
      return m_err;
   }

   return 0;
}


int RealMediaFF::getContentDescription(ContentDescription *cont, const unsigned char *buf, UINT32 object_id, int sz)
{
   int i = 0;

   // Properties
   cont->s.object_id = object_id;
   cont->s.size = sz;

   i = RMFF_HDR_SIZE;
   UNPACK2(cont->object_version, buf, i);

   if ( !strncmp((const char *)&cont->s.object_id, "CONT", 4) && cont->object_version == 0)
   {
      UNPACK2(cont->title_len, buf, i);
      cont->title = new UINT8[cont->title_len + 1];
      memcpy((void *)cont->title, (void *)&buf[i], cont->title_len); i+=cont->title_len;
      m_title = (char *)cont->title;
      m_title[cont->title_len] = 0;

      UNPACK2(cont->author_len, buf, i);
      cont->author = new UINT8[cont->author_len + 1];
      memcpy((void *)cont->author, (void *)&buf[i], cont->author_len); i+=cont->author_len;
      m_author = (char *)cont->author;
      m_author[cont->author_len] = 0;

      UNPACK2(cont->copyright_len, buf, i);
      cont->copyright = new UINT8[cont->copyright_len + 1];
      memcpy((void *)cont->copyright, (void *)&buf[i], cont->copyright_len); i+=cont->copyright_len;
      m_copyright = (char *)cont->copyright;
      m_copyright[cont->copyright_len] = 0;

      UNPACK2(cont->comment_len, buf, i);
      cont->comment = new UINT8[cont->comment_len + 1];
      memcpy((void *)cont->comment, (void *)&buf[i], cont->comment_len); i+=cont->comment_len;
      m_comment = (char *)cont->comment;
      m_comment[cont->comment_len] = 0;
   }
   else
   {
      m_err = -1;
      return m_err;
   }

   return 0;
}


int RealMediaFF::seekChunk(UINT32 object_id)
{
   if (!m_err)
   {
      off_t s, tot;
      UINT32 oid = 0, sz = 0;
      unsigned char buf[255];
      int nbytes = 0;

      if ( (s = lseek(m_fd, 0, SEEK_SET)) != 0)
         return -1;

      tot = 0;
      while( (nbytes = getHdr(buf, 255, oid, sz)) == RMFF_HDR_SIZE && memcmp((void *)&oid, (void *)&object_id, 4) )
      {
         tot += sz;
         if (sz > (unsigned) RMFF_HDR_SIZE)
         {
            if ( (s = lseek(m_fd, sz - RMFF_HDR_SIZE, SEEK_CUR)) != tot )
               return -1;
         }
         else
            return -1; // bail in this case, since the chuck sz includes the header size
      }
      if ( (s = lseek(m_fd, -RMFF_HDR_SIZE, SEEK_CUR)) != tot )
         return -1;

      return s;
   }
   return -1;
}

int RealMediaFF::getMDProperties(MDProperties *props, const unsigned char *buf)
{
   int i = 0;

   int start = i;

   UNPACK4(props->size, buf, i);
   UNPACK4(props->type, buf, i);
   UNPACK4(props->flags, buf, i);
   UNPACK4(props->value_offset, buf, i);
   UNPACK4(props->subproperties_offset, buf, i);
   UNPACK4(props->num_subproperties, buf, i);
   UNPACK4(props->name_length, buf, i);
   props->name = new UINT8[ props->name_length + 1 ];
   memcpy((void *)props->name, (void *)&buf[i], props->name_length);
   props->name[ props->name_length ] = 0;
   i+=props->name_length;
   
   i = start + props->value_offset;
   UNPACK4(props->value_length, buf, i);
   props->value = new UINT8[ props->value_length ];
   memcpy( (void *) props->value, (void *)&buf[i], props->value_length );

   if ( (props->type == MPT_ULONG) || (props->type == MPT_FLAG && props->value_length == 4) )
   {
      // wOOt! the Year is a ULONG, and its stored little endian?! my guess is this is a bug in 
      // RealPlayer 10 for Windows (where I created my test files)
      // This hack is intended to ensure that we at least interpret the Year properly. 
      if (!strcmp((char *)props->name, "Year"))
      {
         if ( *(unsigned long *)props->value > 65536 )
         {
            *(unsigned long *)(props->value) = ntohl(*(unsigned long *)(props->value));
            m_flipYearInMetadataSection = true;
         }
         else
            m_flipYearInMetadataSection = false;
      }
      else
         *(unsigned long *)(props->value) = ntohl(*(unsigned long *)(props->value));
   }
      
   i += props->value_length;
   
   i = start + props->subproperties_offset;
   props->subproperties_list = new PropListEntry[ props->num_subproperties ];
   for (int j=0; j<(int)props->num_subproperties; j++)
   {
      UNPACK4(props->subproperties_list[j].offset, buf, i);
      UNPACK4(props->subproperties_list[j].num_props_for_name, buf, i);
   }
   
   props->subproperties = new MDProperties[ props->num_subproperties ];
   for (int j=0; j<(int)props->num_subproperties; j++)
   {
      i = start + props->subproperties_list[j].offset;
      getMDProperties(&props->subproperties[j], &buf[i]);
   }
  
   return 0;
}

int RealMediaFF::initMetadataSection()
{
   UINT32 object_id;
   off_t s;
   int nbytes;
   unsigned char buf[65536];
   UINT32 consumed;

   memcpy((void *)&object_id, "RMMD", 4);
   if ( (s = seekChunk(object_id)) < 0 )
   {
      m_err = -1;
      return m_err;
   }

   m_md = new MetadataSection;
   consumed = 0;
   nbytes = getChunk(buf, 65536, m_md->s.object_id, m_md->s.size, consumed);
   if (nbytes < 0 || m_md->s.size != consumed || memcmp((void *)&m_md->s.object_id, "RMMD", 4))
   {
      //std::cerr << "SERIOUS ERROR - not able to find the chunk I just seek'd to!\n";
      m_err = -1;
      return m_err;
   }
   // Properties
   int i = RMFF_HDR_SIZE;
   memcpy((void *)&m_md->object_id, (void *)&buf[i], 4); i+=4;
   UNPACK4(m_md->object_version, buf, i);
   if ( !strncmp((const char *)&m_md->s.object_id, "RMMD", 4) )
   {
      if (!getMDProperties(&m_md->properties, &buf[i]))
         saveHeader(m_md);
   }
   else
   {
      m_err = -1;
      return m_err;
   }

   return 0;
}

#ifdef TESTING

void RealMediaFF::printRealFileHeader(std::ostream &os)
{
   char object_id[5];

   if (m_hdr)
   {
      strncpy(object_id, (const char *)&m_hdr->s.object_id, 4);
      object_id[4]=0;

      os << "HDR object_id:      " << object_id << std::endl;
      os << "HDR size:           " << m_hdr->s.size << std::endl;
      os << "HDR object version: " << m_hdr->object_version << std::endl;
      os << "HDR file version:   " << m_hdr->file_version << std::endl;
      os << "HDR num headers:    " << m_hdr->num_headers << std::endl;
   }
}


void RealMediaFF::printRealPropHeader(std::ostream &os)
{      
   char object_id[5];

   if (m_props)
   {
      strncpy(object_id, (const char *)&m_props->s.object_id, 4);
      object_id[4]=0;

      os << "PROPS object_id: " << object_id << std::endl;
      os << "PROPS size: " << m_props->s.size << std::endl;
      os << "PROPS object_version: " << m_props->object_version << std::endl;
   
      os << "PROPS max_bit_rate:    " << m_props->max_bit_rate << std::endl;
      os << "PROPS avg_bit_rate:    " << m_props->avg_bit_rate << std::endl;
      os << "PROPS max_packet_size: " << m_props->max_packet_size << std::endl;
      os << "PROPS avg_packet_size: " << m_props->avg_packet_size << std::endl;
      os << "PROPS num_packets:     " << m_props->num_packets << std::endl;
      os << "PROPS duration:        " << m_props->duration << std::endl;
      os << "PROPS preroll:         " << m_props->preroll << std::endl;
      os << "PROPS index_offset:    " << m_props->index_offset << std::endl;
      os << "PROPS data_offset:     " << m_props->data_offset << std::endl;
      os << "PROPS num_streams:     " << m_props->num_streams << std::endl;
      os << "PROPS flags:           " << m_props->flags << std::endl;
   }
}


void RealMediaFF::printMediaPropHeaders(std::ostream &os)
{
   int i = 0;
   char object_id[5];
   MediaProperties *media_hdr = (MediaProperties *)m_head;

   while (media_hdr)
   {
      strncpy(object_id, (const char *)&media_hdr->s.object_id, 4);
      object_id[4]=0;

      if (!strncmp(object_id, "MDPR", 4))
      {
         os << "MEDIA HDR" << i << " object_id:       " << object_id << std::endl;
         os << "MEDIA HDR" << i << " size:            " << media_hdr->s.size << std::endl;
         os << "MEDIA HDR" << i << " max_bit_rate:    " << media_hdr->max_bit_rate << std::endl;
         os << "MEDIA HDR" << i << " avg_bit_rate:    " << media_hdr->avg_bit_rate << std::endl;
         os << "MEDIA HDR" << i << " max_packet_size: " << media_hdr->max_packet_size << std::endl;
         os << "MEDIA HDR" << i << " avg_packet_size: " << media_hdr->avg_packet_size << std::endl;
         os << "MEDIA HDR" << i << " start_time:      " << media_hdr->start_time << std::endl;
         os << "MEDIA HDR" << i << " preroll:         " << media_hdr->preroll << std::endl;
         os << "MEDIA HDR" << i << " duration:        " << media_hdr->duration << std::endl;
         os << "MEDIA HDR" << i << " stream_name:     " << media_hdr->stream_name << std::endl;
         os << "MEDIA HDR" << i << " mime type:       " << media_hdr->mime_type << std::endl;


         if (media_hdr->lstr)
         {
            os << "MEDIA HDR" << i << " LOGSTR info size: " << media_hdr->lstr->size << std::endl;
            os << "MEDIA HDR" << i << " LOGSTR info num_physical_streams: " << media_hdr->lstr->num_physical_streams << std::endl;
            os << "MEDIA HDR" << i << " LOGSTR info num_rules: " << media_hdr->lstr->num_rules << std::endl;
            os << "MEDIA HDR" << i << " LOGSTR info num_properties: " << media_hdr->lstr->num_properties << std::endl;
            for (int j=0; media_hdr->lstr->properties && j<media_hdr->lstr->num_properties; j++)
            {
               if (media_hdr->lstr->properties[j].name)
                  os << "MEDIA HDR" << i << " LOGSTR info prop name: " << media_hdr->lstr->properties[j].name << std::endl;
               os << "MEDIA HDR" << i << " LOGSTR info prop type: " << media_hdr->lstr->properties[j].type << std::endl;
               os << "MEDIA HDR" << i << " LOGSTR info prop value_length: " << media_hdr->lstr->properties[j].value_length << std::endl;
               if (media_hdr->lstr->properties[j].value_data)
               {
                  if (media_hdr->lstr->properties[j].type == 0)
                     os << "MEDIA HDR" << i << " LOGSTR info prop value: " << 
                        *(unsigned long *)media_hdr->lstr->properties[j].value_data << std::endl;
                  else if (media_hdr->lstr->properties[j].type == 2)
                     os << "MEDIA HDR" << i << " LOGSTR info prop value: " << media_hdr->lstr->properties[j].value_data << std::endl;
                  else
                     os << "MEDIA HDR" << i << " LOGSTR info prop value: <binary>\n";
               }
            }
         }

         i++;
      }
      media_hdr = (MediaProperties *)media_hdr->fwd;
   }
}


void RealMediaFF::printContentDescription(std::ostream &os)
{
   char object_id[5];

   if (m_contenthdr)
   {
      strncpy(object_id, (const char *)&m_contenthdr->s.object_id, 4);
      object_id[4]=0;

      os << "CONT object_id: " << object_id << std::endl;
      os << "CONT title(" << m_contenthdr->title_len << "):\t\t<" << m_contenthdr->title << ">" << std::endl;
      os << "CONT author(" << m_contenthdr->author_len << "):\t\t<" << m_contenthdr->author << ">" << std::endl;
      os << "CONT copyright(" << m_contenthdr->copyright_len << "):\t\t<" << m_contenthdr->copyright << ">" << std::endl;
      os << "CONT comment(" << m_contenthdr->comment_len << "):\t\t<" << m_contenthdr->comment << ">" << std::endl;
   }
}

void RealMediaFF::printID3v1Tag(std::ostream &os)
{
   if (m_id3v1tag)
   {
      os << "ID3 tag     : " << ID3v1::Tag::fileIdentifier() << std::endl;
      os << "ID3 title   : " << m_id3v1tag->tag()->title() << std::endl;
      os << "ID3 artist  : " << m_id3v1tag->tag()->artist() << std::endl;
      os << "ID3 album   : " << m_id3v1tag->tag()->album() << std::endl;
      os << "ID3 year    : " << m_id3v1tag->tag()->year() << std::endl;
      os << "ID3 comment : " << m_id3v1tag->tag()->comment() << std::endl;
      os << "ID3 track   : " << m_id3v1tag->tag()->track() << std::endl;
      os << "ID3 genre   : " << m_id3v1tag->tag()->genre() << std::endl;
   }
}


void RealMediaFF::printMDProperties(std::ostream &os, char *nam, MDProperties *props)
{
   char name[8192];

   strcpy(name, nam);
   os << "MDP subproperties for: " << name << std::endl;

   os << "MD properties.size: " << props->size << std::endl;
   os << "MD properties.type: " << props->type << std::endl;
   os << "MD properties.flags: " << props->flags << std::endl;
   os << "MD properties.value_offset: " << props->value_offset << std::endl;
   os << "MD properties.subproperties_offset: " << props->subproperties_offset << std::endl;
   os << "MD properties.num_subproperties: " << props->num_subproperties << std::endl;
   os << "MD properties.name_length: " << props->name_length << std::endl;
   os << "MD properties.name: " << (char *)props->name << std::endl;
   
   os << "MD properties.value_length: " << props->value_length << std::endl;

   switch (props->type)
   {
      case MPT_TEXT:
      case MPT_TEXTLIST:
      case MPT_URL:
      case MPT_DATE:
      case MPT_FILENAME:
         os << "MD properties.value: " << (char *)props->value << std::endl;
         break;
      case MPT_FLAG:
         if (props->value_length == 4)
            os << "MD properties.value: " << *(unsigned long *)props->value << std::endl;
         else
            os << "MD properties.value: " << *props->value << std::endl;
         break;
      case MPT_ULONG:
         os << "MD properties.value: " << *(unsigned long *)props->value << std::endl;
         break;
      case MPT_BINARY:
         os << "MD properties.value: <binary>" << std::endl;
         break;
      case MPT_GROUPING:
         os << "MD properties.value: <grouping>" << std::endl;
         break;
      case MPT_REFERENCE:
         os << "MD properties.value: <reference>" << std::endl;
         break;
   }

   if (props->num_subproperties)
   {
      strcat(name, (char *)props->name);
      strcat(name, "/");
   }
   for (int j=0; j<props->num_subproperties; j++)
   {
      os << "MD properties.sub_properties_list[" << j << "].offset: " << 
         props->subproperties_list[j].offset << std::endl;
      os << "MD properties.sub_properties_list[" << j << "].num_props_for_name: " << 
         props->subproperties_list[j].num_props_for_name << std::endl;

      os << std::endl;

      printMDProperties(os, name, &props->subproperties[j]);
   }
}


void RealMediaFF::printMetadataSection(std::ostream &os)
{
   char name[8192];
   char oid[5];

   memcpy((void *)oid, (void *)&m_md->s.object_id, 4);
   oid[4] = 0;

   os << "MetadataSection: ";
   os << "MS object_id: " << oid << std::endl;
   os << "MS SIZE: " << m_md->s.size << std::endl;
   os << "MD object_id: " << (char *)&m_md->object_id << std::endl;
   os << "MD object_version: " << m_md->object_version << std::endl;
   os << std::endl;

   strcpy(name, "");
   printMDProperties(os, name, &m_md->properties);
}


std::ostream &RealMediaFF::operator<<(std::ostream &os)
{
   if (m_readProperties)
   {
      printRealFileHeader(os);
      printRealPropHeader(os);
      printMediaPropHeaders(os);
      printContentDescription(os);
      printMetadataSection(os);
   }
   printID3v1Tag(os);

   return os;
}

std::ostream &operator<<(std::ostream &os, RealMediaFF &rmff)
{
   rmff.operator<<(os);

   return os;
}


int main(int argc, char *argv[])
{
   char *m_filen;

   if (argc > 1)
      m_filen = argv[1];
   else
      m_filen = "./Drown.ra";

   RealMediaFF rmff(m_filen);

   if (!rmff.err())
      std::cout << rmff;

   /*
   UINT32 oid = 0;
   memcpy( (void *)&oid, (void *) ".RMF", 4);
   off_t pos = rmff.seekChunk(oid);
   std::cout << "POS=" << pos << std::endl;

   memcpy( (void *)&oid, (void *) "MDPR", 4);
   pos = rmff.seekChunk(oid);
   std::cout << "POS=" << pos << std::endl;

   memcpy( (void *)&oid, (void *) "RMMD", 4);
   pos = rmff.seekChunk(oid);
   std::cout << "POS=" << pos << std::endl;
   */
}
#endif
