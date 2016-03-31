/*
 * Copyright (c) 2015 Isode Limited.
 * All rights reserved.
 * See the COPYING file for more information.
 */

#pragma once

#include <boost/shared_ptr.hpp>

#include <Swiften/Base/API.h>
#include <Swiften/Elements/Payload.h>

namespace Swift {
    class SWIFTEN_API CarbonsPrivate : public Payload {
        public:
            typedef boost::shared_ptr<CarbonsPrivate> ref;

        public:
            virtual ~CarbonsPrivate();
    };
}
