/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.

#include <vector>

#include "c74_min_unittest.h"     // required unit-test header (defines main via Catch)
#include "tap.python_tilde.cpp"   // include the object source so we can instantiate it


// The mock kernel has no Max package on disk, so construction takes the
// "no runtime installed" path: the object must come up inert (silent output,
// no crash) rather than attempting to start Python. Real behavioral coverage
// (loading a class, generated attributes/messages, audio processing) requires
// the embedded interpreter and is exercised in Max against the help patcher.
SCENARIO("object instantiates without a Python runtime") {
    ext_main(nullptr);

    GIVEN("An instance of tap.python~ with no runtime available") {
        test_wrapper<python> an_instance;
        python&              my_object = an_instance;

        WHEN("audio is processed") {
            std::vector<double> input(64, 0.5);
            std::vector<double> output(64, 1.0);    // non-zero so we can tell clear() ran
            double*             inp[1]  = { input.data() };
            double*             outp[1] = { output.data() };
            audio_bundle        ina { inp, 1, static_cast<long>(input.size()) };
            audio_bundle        outa { outp, 1, static_cast<long>(output.size()) };

            my_object(ina, outa);

            THEN("the object outputs silence instead of crashing") {
                for (auto& s : output)
                    REQUIRE(s == 0.0);
            }
        }
    }
}
