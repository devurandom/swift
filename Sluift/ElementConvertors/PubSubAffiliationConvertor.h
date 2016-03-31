/*
 * Copyright (c) 2013-2016 Isode Limited.
 * All rights reserved.
 * See the COPYING file for more information.
 */

#pragma once

#include <Swiften/Base/Override.h>
#include <Swiften/Elements/PubSubAffiliation.h>

#include <Sluift/GenericLuaElementConvertor.h>

namespace Swift {
    class LuaElementConvertors;

    class PubSubAffiliationConvertor : public GenericLuaElementConvertor<PubSubAffiliation> {
        public:
            PubSubAffiliationConvertor();
            virtual ~PubSubAffiliationConvertor();

            virtual boost::shared_ptr<PubSubAffiliation> doConvertFromLua(lua_State*) SWIFTEN_OVERRIDE;
            virtual void doConvertToLua(lua_State*, boost::shared_ptr<PubSubAffiliation>) SWIFTEN_OVERRIDE;
            virtual boost::optional<Documentation> getDocumentation() const SWIFTEN_OVERRIDE;
    };
}
