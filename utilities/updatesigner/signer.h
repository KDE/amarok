/****************************************************************************************
 * Copyright (c) 2009 Jakob Kummerow <jakob.kummerow@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef signer_H
#define signer_H

#include <QtCore/QObject>
#include <QtCrypto>

class signer : public QObject
{
	Q_OBJECT
	public:
		signer();
		virtual ~signer();
		bool setParams(int argc, char** argv);
	
	public slots:
		void keygen();
		void signFile();
		void checkSignature();
		
	private:
		QString m_fileName;
		QString m_pubkey;
};

#endif // signer_H
