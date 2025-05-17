# CHANGELOG

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