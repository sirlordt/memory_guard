# CAMBIOS

## 2025-05-16 18:32 PDT

### Archivos modificados

#### src/MemoryGuard.hpp → src/memory_guard.hpp
- Archivo renombrado a minúsculas.
- Namespace cambiado de `MemoryGuard` a `memory_guard`.
- Macros internas actualizadas para usar el nuevo namespace.

#### src/main.cpp
- Todas las referencias de `#include "MemoryGuard.hpp"` actualizadas a `#include "memory_guard.hpp"`.
- Todas las referencias de `MemoryGuard::` cambiadas a `memory_guard::`.

#### tests/memory_guard_tests.cpp
- Todas las referencias de `#include "MemoryGuard.hpp"` actualizadas a `#include "memory_guard.hpp"`.
- Todas las referencias de `MemoryGuard::` cambiadas a `memory_guard::`.

#### README.md, DOC.en.md, DOC.es.md
- Todas las referencias en la documentación actualizadas para reflejar el nuevo nombre de archivo y namespace (`memory_guard.hpp`, `memory_guard::`).