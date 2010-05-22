/* PhotoFlow - animated image viewer for mobile devices
 *
 * Copyright (C) 2008 Ariya Hidayat (ariya.hidayat@gmail.com)
 * Copyright (C) 2007 Ariya Hidayat (ariya.hidayat@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA
 *
 */

#ifndef IMAGE_THUMBNAIL_H
#define IMAGE_THUMBNAIL_H
#include <qimage.h>
#include <qobject.h>
#include <qsize.h>
#include <qthread.h>

#include <qmutex.h>
#include <qwaitcondition.h>

#include "core/meta/Meta.h"
class ImageLoader : public QThread
{
public:
  ImageLoader();

  ~ImageLoader();

  // returns FALSE if worker is still busy and can't take the task
  bool busy() const;

  void generate(int index, Meta::AlbumPtr iAlbum, QSize size);

  int index() const { return idx; }

  QImage result() const { return img; }

protected:
  void run();

private:
  QMutex mutex;
  QWaitCondition condition;

  bool restart;
  bool working;
  int idx;
  Meta::AlbumPtr m_album;
  QSize size;
  QImage img;
};

class PlainImageLoader
{
public:
  PlainImageLoader();

  bool busy() const { return false; }

  void generate(int index, Meta::AlbumPtr iAlbum, QSize size);

  int index() const { return idx; }

  QImage result() const { return img; }
	
  static QPixmap GetPixmap( Meta::AlbumPtr iAlbum);
  static QImage loadAndResize( Meta::AlbumPtr iAlbum, QSize size );
private:
  int idx;
  QImage img;
};
#endif // IMAGE_THUMBNAIL_H

