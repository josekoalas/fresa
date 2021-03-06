# fresa :strawberry:

<p>
  <a href="https://github.com/josekoalas/fresa/blob/main/CHANGELOG.md"><img alt="changelog" src="https://img.shields.io/badge/changelog-0.4.5-4673db?style=pl"/></a>
</p>

A tiny game engine made in `c++20`.

## documentation :books:

The documentation is available at [josekoalas.github.io/fresa](https://josekoalas.github.io/fresa). It includes detailed instruction on how to set up a project and reference on how to use the engine. The engine also includes plentiful comments inside the code, so hopefully it is easy to read.

## features :sparkles:

_This is a reimplementation of **fresa** from scratch. The motivation behind it is to focus on clarity and documentation from the beginning. You can see the source for the fully-functioning legacy version [here](https://github.com/josekoalas/fresa-legacy). This new version is not up to par feature-wise with the original project yet._

- _simple entity-component system (in progress)_
- game loop and system manager
- multithread job system
- logging and debug tools
- macro-less unit testing framework

**next up**

- events
- reflection
- gpu driven renderer

## building :hammer:

For in depth information on how to build the engine, please refer to the [build instructions](https://josekoalas.github.io/fresa/0.4/getting_started/building). However, it is possible to quickly get started using the project template, [mermelada](https://github.com/josekoalas/mermelada):

```
git clone --recurse-submodules https://github.com/josekoalas/mermelada
cd mermelada
cmake -S . -B build
cmake --build build
```

The build system of choice is [`cmake`](https://cmake.org), and the build is tested with `clang 14.0` and `gcc 12.1`. `MSVC` support is comming as soon as the fix for [this](https://developercommunity.visualstudio.com/t/Lambda-inside-lambda-cant-detect-conste/10085564) bug report I submitted is released.

## contributing :herb:

You are welcome to contribute to **fresa** in any way you can. If you want to do so, you can either open an issue or a pull request on github for a feature on the roadmap or bug fix, or contact me through [email](mailto:jose.pazos.perez@rai.usc.es) or [telegram](https://t.me/josekoalas) for further discussion.

When contributing, please be sure to include documentation on what you are doing and try to follow the naming conventions of the rest of the project.

## license :pencil:

This project is licensed under [GNU GPLv3](LICENSE.md).