/****************************************************************************************
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
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

#ifndef TRANSCODING_FORMAT_H
#define TRANSCODING_FORMAT_H

#include "core/amarokcore_export.h"
#include "core/transcoding/TranscodingDefines.h"
#include "core/transcoding/TranscodingProperty.h"
#include "core/transcoding/TranscodingConfiguration.h"

#include <QIcon>
#include <QRegularExpression>
#include <QStringList>

namespace Transcoding
{

/**
 * This is an abstract base class that defines what an Amarok Transcoding Format should
 * look like.
 * All transcoding format descriptor classes must inherit from this class.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROKCORE_EXPORT Format //: public QObject
{
public:
    /**
     * Destructor.
     */
    virtual ~Format() {}

    /**
     * Returns the encoder identifier of the current format.
     * @note You should not need to reimplement this method when subclassing as long as you
     * initialize m_encoder in the constructor.
     * @return the encoder enum item
     */
    /*final*/ virtual Encoder encoder() { return m_encoder; }

    /**
     * Returns the file extension suggested by the current format. The string should be
     * compatible with Meta::Track::type()
     * @note You should not need to reimplement this method when subclassing as long as you
     * initialize m_fileExtension in the constructor.
     *
     * @return a QString with the file extension
     */
    /*final*/ virtual QString fileExtension() const { return m_fileExtension; }

    /**
     * Returns a human readable and translated name of the format.
     * @return a QString with the pretty name
     */
    virtual QString prettyName() const = 0;

    /**
     * Returns a human readable and translated description of the format.
     * @return a QString with the description
     */
    virtual QString description() const = 0;

    /**
     * Returns an icon that represents this format.
     * @return a QIcon with the icon
     */
    virtual QIcon icon() const = 0;

    /**
     * Returns a list of parameters to be passed to the FFmpeg binary to perform the
     * transcoding operation.
     * @return a QStringList with the parameters
     */
    virtual QStringList ffmpegParameters( const Configuration &configuration ) const = 0;

    /**
     * Checks if FFmpeg supports the current format.
     * @param ffmpegOutput one line of the output of the command "ffmpeg -codecs"
     * @return true if the format is supported by an encoder in FFmpeg on this output line,
     *         otherwise false
     */
    virtual bool verifyAvailability( const QString &ffmpegOutput ) const = 0;

    /**
     * Returns a list of properties that must be set in order to perform the transcoding
     * operation.
     * @note You should not need to reimplement this method when subclassing as long as you
     * initialize m_propertyList in the constructor.
     * @return the Transcodng::PropertyList with the Transcoding::Propertys
     */
    virtual const PropertyList & propertyList() const { return m_propertyList; }

protected:
    Encoder m_encoder;
    QString m_fileExtension;
    PropertyList m_propertyList;
};

}

#endif // TRANSCODING_FORMAT_H
