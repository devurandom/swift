/*
 * Copyright (c) 2014-2016 Isode Limited.
 * All rights reserved.
 * See the COPYING file for more information.
 */

#include <boost/smart_ptr/make_shared.hpp>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <Swiften/Base/DateTime.h>
#include <Swiften/Elements/Delay.h>
#include <Swiften/Elements/MAMResult.h>
#include <Swiften/Elements/Message.h>
#include <Swiften/Serializer/PayloadSerializers/FullPayloadSerializerCollection.h>
#include <Swiften/Serializer/PayloadSerializers/MAMQuerySerializer.h>

using namespace Swift;

class MAMQuerySerializerTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(MAMQuerySerializerTest);
        CPPUNIT_TEST(testSerialize);
        CPPUNIT_TEST_SUITE_END();

    public:
        void testSerialize() {
            MAMQuerySerializer serializer;

            boost::shared_ptr<Form> parameters(boost::make_shared<Form>());

            boost::shared_ptr<FormField> fieldType = boost::make_shared<FormField>(FormField::TextSingleType);
            fieldType->setName("FORM_TYPE");
            fieldType->addValue("urn:xmpp:mam:0");
            parameters->addField(fieldType);

            boost::shared_ptr<FormField> fieldStart = boost::make_shared<FormField>(FormField::TextSingleType);
            fieldStart->setName("start");
            fieldStart->addValue("2010-08-07T00:00:00Z");
            parameters->addField(fieldStart);

            boost::shared_ptr<ResultSet> set = boost::make_shared<ResultSet>();
            set->setMaxItems(10);

            boost::shared_ptr<MAMQuery> query(boost::make_shared<MAMQuery>());
            query->setQueryID(std::string("id0"));
            query->setNode(std::string("node1"));
            query->setForm(parameters);
            query->setResultSet(set);

            std::string expectedResult =
                "<query node=\"node1\" queryid=\"id0\" xmlns=\"urn:xmpp:mam:0\">"
                    "<x type=\"form\" xmlns=\"jabber:x:data\">"
                        "<field type=\"text-single\" var=\"FORM_TYPE\">"
                            "<value>urn:xmpp:mam:0</value>"
                        "</field>"
                        "<field type=\"text-single\" var=\"start\">"
                            "<value>2010-08-07T00:00:00Z</value>"
                        "</field>"
                    "</x>"
                    "<set xmlns=\"http://jabber.org/protocol/rsm\">"
                        "<max>10</max>"
                    "</set>"
                "</query>";

            CPPUNIT_ASSERT_EQUAL(expectedResult, serializer.serialize(query));
        }

    private:
        FullPayloadSerializerCollection serializers;
};

CPPUNIT_TEST_SUITE_REGISTRATION(MAMQuerySerializerTest);
