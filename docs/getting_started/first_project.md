# a simple program

If you are using the [template](building.md#using-a-template) then you already have a minimum `main.cpp` file that should look something like this:

```cpp
#include "engine.h"

int main (int argv, char** args) {
    fresa::run();
    return 0;
}
```

All you need to run **fresa** is to import the engine header and call `fresa::run()`.

If you wish to configure how the engine behaves, you can look into the [configuration file](../reference/config.md).

_This page will be updated when components are added to the engine to explain how to create your own functionality._