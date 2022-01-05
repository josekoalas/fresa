### changelog :sparkles:

### 0.3 - duck :duck:

---

#### 0.3.6 - 

**added**
- imgui support

**changed**
- refactored update lists

**fixed**
- hopefully fixed sdl_keycodes not being recognised on linux and web

#### 0.3.5 - make make make and make - _24 12 2021_

**added**
- performance timings to physics and rendering
- tried to add vulkan timestamps 
- camera struct with projection and view matrices

**changed**
- disable validation layers on release
- filesystem path helper for adding base path (in emscripten it is different)

**fixed**
- fixed vulkan render pass dependency flag validation error
- temporary fix for msvc compilers, disabled logging since there was a compilation error
- unsupported validations layers no longer crash the program, just issue a warning
- fixed shader compiling for web
- fixed an error with opengl input attachments, that seemed to work but it was a coincidence

#### 0.3.4 - all about that state c: - _18 12 2021_

**added**
- compile time type name generator
- component list documentation
- state machine events can now be linked with new event system
- render system list caller inside r_graphics.h

**changed**
- removed macros from state machine name generation and changed it to compile time names
- changed state machine `is()` function to remove accidental RTTI

**fixed**
- fixed vulkan only working when the xcode address sanitizer was on (errors with use of memory after return)
- fixed emscripten window and float precision

#### 0.3.3 - hey there! this is an event - _14 12 2021_

**added**
- publish events
- observers and permanent callbacks
- new variant helper functions
- improved component id generation based on variant index

**changed**
- improved event handling
- reworked input
- refactored type headers

#### 0.3.2 - component nightmare - _13 12 2021_

**added**
- heterogeneous poly container

**changed**
- improved `getID<Component>` and added special checks
- now system definition is independent of the engine, they can be registered in the update lists by template instantiation

#### 0.3.1 - out to the world - _11 12 2012_

**added**
- readme and new changelog (kinda meta)

**changed**
- improved documentation
- refactored config struct

#### 0.3.0 - the rendering update - _09 12 2021_

**added**
- brand new rendering api with complete vulkan support
- shader reflection with spirv-cross
- multiple subpasses and attachments
- many more changes to the engine

---

_previously fresa was tied to another of my projects, lume, so the changelog was shared between them. this changelog therefore starts at version 0.3, when fresa was made completely standalone._