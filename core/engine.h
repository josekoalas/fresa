//* engine
//      handles initialization, update and stop
//      central point of the engine, calls all the other subsystems
#pragma once

namespace fresa
{
    //* call from main.cpp
    void run();

    //* different stages of the engine, referenced from run()
    namespace detail {
        void init();
        bool update();
        void stop();
    }
}

