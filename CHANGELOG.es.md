# CAMBIOS

## 2025-05-16 19:49 PDT

### Archivos modificados

#### Renombre del proyecto: memory_guard → try_catch_guard

- Renombrados todos los archivos y referencias para reflejar el nuevo nombre del proyecto.

#### src/memory_guard.hpp → src/try_catch_guard.hpp
- Archivo renombrado.
- Cambiados los guards de inclusión de `MEMORY_GUARD_HPP` a `TRY_CATCH_GUARD_HPP`.
- Namespace cambiado de `memory_guard` a `try_catch_guard`.
- Macros internas actualizadas para usar el nuevo namespace.

#### src/main.cpp
- Todas las referencias de `#include "memory_guard.hpp"` actualizadas a `#include "try_catch_guard.hpp"`.
- Todas las referencias de `memory_guard::` cambiadas a `try_catch_guard::`.

#### tests/memory_guard_tests.cpp → tests/try_catch_guard_tests.cpp
- Archivo renombrado.
- Todas las referencias de `#include "memory_guard.hpp"` actualizadas a `#include "try_catch_guard.hpp"`.
- Todas las referencias de `memory_guard::` cambiadas a `try_catch_guard::`.
- Nombres de casos de prueba actualizados de "MemoryGuard" a "TryCatchGuard".
- Etiquetas de prueba actualizadas de "[memory_guard]" a "[try_catch_guard]".

#### tests/CMakeLists.txt
- Todas las referencias de `memory_guard_tests` actualizadas a `try_catch_guard_tests`.

#### README.md, DOC.en.md, DOC.es.md
- Todas las referencias en la documentación actualizadas para usar el nuevo nombre del proyecto, nombre de archivo y namespace.
- Cambiado "MemoryGuard" a "TryCatchGuard" en todo el texto.
- Cambiado `memory_guard.hpp` a `try_catch_guard.hpp`.
- Cambiado `memory_guard::` a `try_catch_guard::`.

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