#ifndef AMAROK_FINGERPRINTCAPABILITY_H
#define AMAROK_FINGERPRINTCAPABILITY_H

#include "amarok_export.h"
#include "meta/Meta.h"
#include "meta/Capability.h"
#include "fingerprint/Fingerprint.h"

namespace Meta {
    class AMAROK_EXPORT FingerprintCapability : public Meta::Capability {
        Q_OBJECT

        public:
            virtual ~FingerprintCapability();

            static Type capabilityInterfaceType() { return Meta::Capability::Fingerprint; }

            virtual const Fingerprint::FingerprintPtr getFingerprint() const = 0;
            virtual Fingerprint::Similarity calcSimilarityTo( const Meta::TrackPtr ) const = 0;

    }; // class FingerprintCapability
} // namespace Meta

#endif //AMAROK_FINGERPRINTCAPABILITY_H
