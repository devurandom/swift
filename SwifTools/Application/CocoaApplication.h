/*
 * Copyright (c) 2010 Isode Limited.
 * All rights reserved.
 * See the COPYING file for more information.
 */

#pragma once

namespace Swift {
    class CocoaApplication {
        public:
            CocoaApplication();
            ~CocoaApplication();

        private:
            class Private;
            Private* d;
    };
}
