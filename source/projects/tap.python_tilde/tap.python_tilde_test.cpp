/// @file
/// @copyright  Copyright 2022 Timothy Place. All rights reserved.
/// @license           Use of this source code is governed by the MIT License found in the License.md file.

#include "c74_min_unittest.h"
#include "tap.python_tilde.cpp"


SCENARIO("object produces correct output") {
    ext_main(nullptr);

    GIVEN("An instance of our object") {

        test_wrapper<python>    an_instance;
        python&                 my_object = an_instance;

        // check that default attr values are correct
        // REQUIRE((my_object.greeting == symbol("hello world")));

        // now proceed to testing various sequences of events
        /*
        WHEN("a 'bang' is received") {
            my_object.bang();
            THEN("our greeting is produced at the outlet") {
                auto& output = *c74::max::object_getoutput(my_object, 0);
                REQUIRE((output.size() == 1));
                REQUIRE((output[0].size() == 1));
                REQUIRE((output[0][0] == symbol("hello world")));
            }
        }
        */
    }
}
