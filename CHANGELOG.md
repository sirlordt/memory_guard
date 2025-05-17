# CHANGELOG

## 2025-05-16 19:49 PDT

### Modified Files

#### Project Rename: memory_guard → try_catch_guard

- Renamed all files and references to reflect the new project name.

#### src/memory_guard.hpp → src/try_catch_guard.hpp
- Renamed the file.
- Changed the include guards from `MEMORY_GUARD_HPP` to `TRY_CATCH_GUARD_HPP`.
- Changed the namespace from `memory_guard` to `try_catch_guard`.
- Updated internal macros to use the new namespace.

#### src/main.cpp
- Updated all references from `#include "memory_guard.hpp"` to `#include "try_catch_guard.hpp"`.
- Changed all references from `memory_guard::` to `try_catch_guard::`.

#### tests/memory_guard_tests.cpp → tests/try_catch_guard_tests.cpp
- Renamed the file.
- Updated all references from `#include "memory_guard.hpp"` to `#include "try_catch_guard.hpp"`.
- Changed all references from `memory_guard::` to `try_catch_guard::`.
- Updated test case names from "MemoryGuard" to "TryCatchGuard".
- Updated test tags from "[memory_guard]" to "[try_catch_guard]".

#### tests/CMakeLists.txt
- Updated all references from `memory_guard_tests` to `try_catch_guard_tests`.

#### README.md, DOC.en.md, DOC.es.md
- Updated all documentation references to use the new project name, file name, and namespace.
- Changed "MemoryGuard" to "TryCatchGuard" throughout.
- Changed `memory_guard.hpp` to `try_catch_guard.hpp`.
- Changed `memory_guard::` to `try_catch_guard::`.

## 2025-05-16 18:32 PDT

### Modified Files

#### src/MemoryGuard.hpp → src/memory_guard.hpp
- Renamed the file to lowercase.
- Changed the namespace from `MemoryGuard` to `memory_guard`.
- Updated internal macros to use the new namespace.

#### src/main.cpp
- Updated all references from `#include "MemoryGuard.hpp"` to `#include "memory_guard.hpp"`.
- Changed all references from `MemoryGuard::` to `memory_guard::`.

#### tests/memory_guard_tests.cpp
- Updated all references from `#include "MemoryGuard.hpp"` to `#include "memory_guard.hpp"`.
- Changed all references from `MemoryGuard::` to `memory_guard::`.

#### README.md, DOC.en.md, DOC.es.md
- Updated all documentation references to use the new file name and namespace (`memory_guard.hpp`, `memory_guard::`).