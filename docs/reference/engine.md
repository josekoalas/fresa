# [`engine`](https://github.com/josekoalas/fresa/blob/main/core/engine.h)

This is the main class of **fresa**. Handles initialization, update and cleaning, calling all required systems.

## `run`

```cpp
void fresa::run()
```

This is the main entrypoint of the engine. First it perform the tests defined in the configuration file (if enabled with `FRESA_ENABLE_TESTS`). Then calls [`init`](#init) before running the update loop. Finally it stops all systems and returns control to the caller function.

## `init`

```cpp
void fresa::detail::init()
```

Called from [`run`](#run) handles system initialization and all work needed before the main update loop.

## `update`

```cpp
bool fresa::detail::update()
```

Main update loop of the application. The simulation update is decoupled from the frame time, instead being increased in discrete steps of `dt`. This allows the engine to run independent of frame rate. The implementation is very similar to the one described in the [fix your timestep](https://gafferongames.com/post/fix_your_timestep) article.

## `stop`

```cpp
void fresa::detail::stop()
```

Called from [`run`](#run) when execution ends. Clear systems in LIFO order (using an `std::stack`).