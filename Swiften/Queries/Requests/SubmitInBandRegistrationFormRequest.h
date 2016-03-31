/*
 * Copyright (c) 2010-2016 Isode Limited.
 * All rights reserved.
 * See the COPYING file for more information.
 */

#pragma once

#include <boost/shared_ptr.hpp>

#include <Swiften/Base/API.h>
#include <Swiften/Base/boost_bsignals.h>
#include <Swiften/Elements/InBandRegistrationPayload.h>
#include <Swiften/Queries/Request.h>

namespace Swift {
    class SWIFTEN_API SetInBandRegistrationRequest : public Request {
        public:
            typedef boost::shared_ptr<SetInBandRegistrationRequest> ref;

            static ref create(const JID& to, InBandRegistrationPayload::ref payload, IQRouter* router) {
                return ref(new SetInBandRegistrationRequest(to, payload, router));
            }

        private:
            SetInBandRegistrationRequest(const JID& to, InBandRegistrationPayload::ref payload, IQRouter* router) : Request(IQ::Set, to, InBandRegistrationPayload::ref(payload), router) {
            }

            virtual void handleResponse(boost::shared_ptr<Payload> payload, ErrorPayload::ref error) {
                onResponse(payload, error);
            }

        public:
            boost::signal<void (boost::shared_ptr<Payload>, ErrorPayload::ref)> onResponse;
    };
}
