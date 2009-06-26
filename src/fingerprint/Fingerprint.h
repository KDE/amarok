/*
    Copyright (c) 2008 Soren Harward <stharward@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include "amarok_export.h"

#ifdef HAVE_FINGERPRINT
#include <gtacfeat/fingerprint.h>
#else
#define GTACFEAT_INVALID_SIMILARITY -1.0
#endif

#include <ksharedptr.h>
#include <QString>
#include <QVector>

namespace Fingerprint {
    class Fingerprint;

    typedef double Similarity;
    typedef KSharedPtr<Fingerprint> FingerprintPtr;
    typedef QVector<double> Vector;
    const Similarity INVALID_SIMILARITY = GTACFEAT_INVALID_SIMILARITY;

    class AMAROK_EXPORT Fingerprint : public QSharedData {
        public:
            Fingerprint(const QString&);
            ~Fingerprint();

            bool exists() const;
            Similarity calcSimilarity(const FingerprintPtr) const;
            const QString toString() const;

        private:
            // the format version of the fingerprint
            int version;

            // the fingerprint vector itself
            Vector v;

            // parse the fingerprint string that comes out of file's metadata
            Vector parseString(const QString&) const;

    }; // class Fingerprint
} // namespace Fingerprint

#endif // defined FINGERPRINT_H
