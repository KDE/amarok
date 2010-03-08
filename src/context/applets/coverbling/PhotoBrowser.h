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

#ifndef PHOTO_BROWSER_H
#define PHOTO_BROWSER_H

#include "pictureflow.h"
#include "meta/Meta.h"

class PhotoBrowser: public PictureFlow
{
  Q_OBJECT

public:
  PhotoBrowser(QWidget* parent = 0);

  virtual ~PhotoBrowser();

  void fillAlbums(Meta::AlbumList albums);
public slots:
        void    skipToFirst();
	void	skipToLast();
	void	fastForward();
	void	fastBackward();
	void 	skipToSlide(int iSlide); 
private slots:
  void preload();
  void updateImageData();

private:
  class Private;
  Private *d;
};  

#endif // PHOTO_BROWSER_H

