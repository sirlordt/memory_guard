# Biblioteca TryCatchGuard

## Descripción General

TryCatchGuard es una biblioteca C++ diseñada para capturar y manejar fallos de segmentación (SIGSEGV) de manera controlada. Proporciona un mecanismo similar a try-catch para manejar accesos inválidos a memoria, como desreferenciación de punteros nulos o acceso a direcciones de memoria inválidas, sin que la aplicación se bloquee.

## Características

- Captura y manejo de fallos de segmentación utilizando una sintaxis familiar tipo try-catch
- Implementación segura para aplicaciones multi-hilo
- Mensajes de error detallados que incluyen el tipo de violación de acceso a memoria
- Soporte tanto para desreferenciación de punteros nulos como para otros accesos inválidos a memoria
- Soporte para bloques try anidados con propagación adecuada de excepciones
- Mínima sobrecarga de rendimiento cuando no ocurren excepciones

## Cómo Funciona

TryCatchGuard utiliza el manejo de señales estándar de C++ para instalar un manejador de señales personalizado para señales SIGSEGV. Cuando ocurre un acceso inválido a memoria dentro de un bloque `_try`, el manejador de señales captura el fallo, registra información sobre la dirección del fallo y utiliza `longjmp` para devolver el control al bloque `_try`. La biblioteca entonces lanza una excepción personalizada `InvalidMemoryAccessException` que puede ser capturada utilizando la macro `_catch`.

La implementación utiliza una pila de buffers de salto para soportar bloques try anidados, permitiendo que las excepciones sean capturadas en el nivel apropiado. El almacenamiento local de hilos asegura que cada hilo tenga su propio contexto, haciendo que la biblioteca sea segura para hilos.

## Uso

### Uso Básico

```cpp
#include <iostream>
#include "try_catch_guard.hpp|try_catch_guard::"

void ejemplo() {
    _try {
        // Código que podría causar un fallo de segmentación
        int* ptr = nullptr;
        *ptr = 10; // Esto normalmente bloquearía el programa
    }
    _catch(try_catch_guard.hpp|try_catch_guard::InvalidMemoryAccessException, e) {
        // Manejar la excepción
        std::cerr << "Excepción capturada: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Iniciando ejemplo básico..." << std::endl;
    
    ejemplo();
    
    std::cout << "¡Ejemplo completado con éxito!" << std::endl;
    
    // Se recomienda llamar a unregisterThreadHandler() para evitar fugas de memoria
    // que serían detectadas por herramientas como Valgrind
    try_catch_guard.hpp|try_catch_guard::unregisterThreadHandler();
    
    return 0;
}
```

### Uso con Excepciones Estándar de C++

TryCatchGuard es compatible con el mecanismo de excepciones estándar de C++. Puedes lanzar y capturar excepciones estándar o personalizadas dentro de bloques `_try`/`_catch` de TryCatchGuard. Esto te permite combinar la protección contra fallos de segmentación con el manejo normal de excepciones de C++.

```cpp
#include <iostream>
#include <stdexcept>
#include "try_catch_guard.hpp|try_catch_guard::"

class MiExcepcionPersonalizada : public std::exception {
public:
    const char* what() const noexcept override {
        return "Esta es mi excepción personalizada";
    }
};

void ejemplo_excepciones_mixtas() {
    _try {
        std::cout << "Dentro del bloque _try..." << std::endl;
        
        // Puedes lanzar excepciones estándar de C++ dentro de un bloque _try
        if (rand() % 3 == 0) {
            throw std::runtime_error("Error de tiempo de ejecución estándar");
        }
        else if (rand() % 3 == 1) {
            throw MiExcepcionPersonalizada();
        }
        else {
            // O causar un fallo de segmentación
            int* ptr = nullptr;
            *ptr = 10; // Esto generará SIGSEGV
        }
    }
    _catch(try_catch_guard.hpp|try_catch_guard::InvalidMemoryAccessException, e) {
        // Esta captura solo las excepciones de acceso inválido a memoria
        std::cerr << "Excepción de TryCatchGuard capturada: " << e.what() << std::endl;
    }
    
    // Puedes usar bloques catch estándar para capturar otras excepciones
    catch(const MiExcepcionPersonalizada& e) {
        std::cerr << "Excepción personalizada capturada: " << e.what() << std::endl;
    }
    catch(const std::exception& e) {
        std::cerr << "Excepción estándar capturada: " << e.what() << std::endl;
    }
    catch(...) {
        std::cerr << "Excepción desconocida capturada" << std::endl;
    }
    
    // Limpiar recursos
    try_catch_guard.hpp|try_catch_guard::unregisterThreadHandler();
}

int main() {
    srand(time(nullptr));
    ejemplo_excepciones_mixtas();
    return 0;
}
```

También puedes anidar bloques try-catch estándar dentro de bloques `_try`/`_catch` de TryCatchGuard:

```cpp
#include <iostream>
#include <stdexcept>
#include "try_catch_guard.hpp|try_catch_guard::"

void ejemplo_anidado() {
    _try {
        std::cout << "Bloque _try externo..." << std::endl;
        
        try {
            std::cout << "Bloque try estándar interno..." << std::endl;
            throw std::runtime_error("Error dentro del try estándar");
        }
        catch(const std::exception& e) {
            std::cerr << "Capturado en catch estándar: " << e.what() << std::endl;
            
            // Podemos causar un fallo de segmentación dentro de un catch estándar
            // y será capturado por el _catch externo
            int* ptr = nullptr;
            *ptr = 10;
        }
    }
    _catch(try_catch_guard.hpp|try_catch_guard::InvalidMemoryAccessException, e) {
        std::cerr << "Capturado en _catch: " << e.what() << std::endl;
    }
    
    try_catch_guard.hpp|try_catch_guard::unregisterThreadHandler();
}
```

### Seguridad en Hilos

TryCatchGuard está diseñado para ser seguro en entornos multi-hilo. Cada hilo registra su propio manejador, y la biblioteca mantiene contextos específicos para cada hilo para asegurar que los fallos de segmentación se manejen correctamente en aplicaciones multi-hilo.

```cpp
#include "try_catch_guard.hpp|try_catch_guard::"
#include <thread>
#include <mutex>

// Mutex global para sincronización de salida de consola
std::mutex console_mutex;

// Función auxiliar para salida de consola sincronizada
template<typename... Args>
void synchronized_print(Args&&... args) {
    std::lock_guard<std::mutex> lock(console_mutex);
    (std::cout << ... << args) << std::endl;
}

void funcion_hilo(int id) {
    _try {
        // Código que podría causar un fallo de segmentación
        if (id % 2 == 0) {
            int* ptr = nullptr;
            synchronized_print("Hilo ", id, ": Intentando acceder a un puntero nulo");
            *ptr = 10; // Esto normalmente bloquearía el programa
        }
    }
    _catch(try_catch_guard.hpp|try_catch_guard::InvalidMemoryAccessException, e) {
        synchronized_print("Hilo ", id, " capturó excepción: ", e.what());
    }
    
    // Importante: Liberar el registro del manejador cuando el hilo termina
    try_catch_guard.hpp|try_catch_guard::unregisterThreadHandler();
}

int main() {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(funcion_hilo, i);
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "Todos los hilos han terminado correctamente" << std::endl;
    
    return 0;
}
```

### Bloques Try Anidados

TryCatchGuard soporta bloques try anidados, permitiendo escenarios de manejo de errores más complejos:

```cpp
#include <iostream>
#include "try_catch_guard.hpp|try_catch_guard::"

void ejemplo_bloques_try_anidados() {
    _try {
        std::cout << "Bloque _try externo: Iniciando ejecución" << std::endl;
        
        // Bloque _try interno
        _try {
            std::cout << "Bloque _try interno: Iniciando ejecución" << std::endl;
            
            // Generar una excepción en el bloque interno
            int* ptr = nullptr;
            *ptr = 10; // Esto generará SIGSEGV
            
            std::cout << "Bloque _try interno: Esta línea no debería ejecutarse" << std::endl;
        }
        _catch(try_catch_guard.hpp|try_catch_guard::InvalidMemoryAccessException, excepcionInterna) {
            std::cerr << "Bloque _catch interno: Excepción capturada: " << excepcionInterna.what() << std::endl;
        }
        
        std::cout << "Bloque _try externo: Después del bloque _try interno" << std::endl;
        
        // Ahora generar una excepción en el bloque externo
        int* ptr = nullptr;
        *ptr = 20; // Esto generará SIGSEGV
        
        std::cout << "Bloque _try externo: Esta línea no debería ejecutarse" << std::endl;
    }
    _catch(try_catch_guard.hpp|try_catch_guard::InvalidMemoryAccessException, excepcionExterna) {
        std::cerr << "Bloque _catch externo: Excepción capturada: " << excepcionExterna.what() << std::endl;
    }
    
    std::cout << "¡Ejemplo completado con éxito!" << std::endl;
    
    // Limpiar recursos
    try_catch_guard.hpp|try_catch_guard::unregisterThreadHandler();
}
```

### Diferentes Tipos de Violaciones de Acceso a Memoria

TryCatchGuard puede manejar diferentes tipos de accesos inválidos a memoria:

```cpp
#include <iostream>
#include "try_catch_guard.hpp|try_catch_guard::"

void probar_puntero_nulo() {
    _try {
        // Desreferenciación de puntero nulo
        std::cout << "Intentando acceder a un puntero nulo..." << std::endl;
        int* ptr_nulo = nullptr;
        *ptr_nulo = 10; // Esto lanzará una InvalidMemoryAccessException
        std::cout << "Esta línea no se ejecutará" << std::endl;
    }
    _catch(try_catch_guard.hpp|try_catch_guard::InvalidMemoryAccessException, e) {
        std::cerr << "Excepción de puntero nulo capturada: " << e.what() << std::endl;
    }
}

void probar_direccion_invalida() {
    _try {
        // Acceso a dirección de memoria inválida
        std::cout << "Intentando acceder a una dirección de memoria inválida..." << std::endl;
        int* ptr_malo = reinterpret_cast<int*>(0xDEADBEEF);
        *ptr_malo = 20; // Esto también lanzará una InvalidMemoryAccessException
        std::cout << "Esta línea no se ejecutará" << std::endl;
    }
    _catch(try_catch_guard.hpp|try_catch_guard::InvalidMemoryAccessException, e) {
        std::cerr << "Excepción de dirección inválida capturada: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Iniciando ejemplos de violaciones de memoria..." << std::endl;
    
    probar_puntero_nulo();
    probar_direccion_invalida();
    
    std::cout << "¡Todos los ejemplos completados con éxito!" << std::endl;
    
    // Se recomienda llamar a unregisterThreadHandler() para evitar fugas de memoria
    // que serían detectadas por herramientas como Valgrind
    try_catch_guard.hpp|try_catch_guard::unregisterThreadHandler();
    
    return 0;
}
```

## Cuándo Usar unregisterThreadHandler()

### Aplicaciones de Un Solo Hilo

En aplicaciones de un solo hilo, llamar a `try_catch_guard.hpp|try_catch_guard::unregisterThreadHandler()` al final del programa es técnicamente opcional ya que el sistema operativo recuperará toda la memoria cuando el proceso termine. Sin embargo, se recomienda encarecidamente hacerlo por las siguientes razones:

1. **Herramientas de Detección de Fugas de Memoria**: Herramientas como Valgrind reportarán fugas de memoria si no liberas explícitamente la memoria asignada por TryCatchGuard antes de la terminación del programa.
2. **Gestión de Recursos**: Es una buena práctica limpiar todos los recursos que utiliza tu programa, incluso si el sistema operativo los recuperaría de todos modos.
3. **Aplicaciones de Larga Duración**: Si estás usando TryCatchGuard en una aplicación de larga duración donde solo lo necesitas para secciones específicas, debes llamar a `unregisterThreadHandler()` después de esas secciones para liberar recursos inmediatamente.

```cpp
#include <iostream>
#include "try_catch_guard.hpp|try_catch_guard::"

int main() {
    std::cout << "Iniciando ejemplo de un solo hilo..." << std::endl;
    
    // Sección donde necesitamos TryCatchGuard
    _try {
        std::cout << "Sección protegida..." << std::endl;
        // Algún código que podría causar fallos de segmentación
    }
    _catch(try_catch_guard.hpp|try_catch_guard::InvalidMemoryAccessException, e) {
        std::cerr << "Excepción capturada: " << e.what() << std::endl;
    }
    
    // Si ya no necesitamos TryCatchGuard, podemos liberar el registro del manejador
    try_catch_guard.hpp|try_catch_guard::unregisterThreadHandler();
    
    std::cout << "Continuando con el resto del programa..." << std::endl;
    // Resto del programa donde no necesitamos TryCatchGuard
    
    return 0;
}
```

### Aplicaciones Multi-Hilo

En aplicaciones multi-hilo, es importante llamar a `try_catch_guard.hpp|try_catch_guard::unregisterThreadHandler()` al final de cada función de hilo para evitar fugas de recursos. Esto es especialmente importante para hilos de trabajo que se crean y destruyen con frecuencia.

Es crucial entender que la función main también se ejecuta en un hilo (el "hilo principal"). Si el hilo principal usa TryCatchGuard directamente, también debe llamar a `unregisterThreadHandler()` antes de finalizar para evitar fugas de memoria que serían detectadas por herramientas como Valgrind.

```cpp
#include <iostream>
#include <thread>
#include "try_catch_guard.hpp|try_catch_guard::"

void hilo_trabajador() {
    // Código del hilo usando TryCatchGuard
    _try {
        // Algún código que podría causar fallos de segmentación
    }
    _catch(try_catch_guard.hpp|try_catch_guard::InvalidMemoryAccessException, e) {
        std::cerr << "Hilo trabajador capturó excepción: " << e.what() << std::endl;
    }
    
    // Limpiar recursos del hilo
    try_catch_guard.hpp|try_catch_guard::unregisterThreadHandler();
}

int main() {
    // Crear y ejecutar hilos trabajadores
    std::thread t(hilo_trabajador);
    
    // El hilo principal también usa TryCatchGuard
    _try {
        // Algún código en el hilo principal que podría causar fallos de segmentación
    }
    _catch(try_catch_guard.hpp|try_catch_guard::InvalidMemoryAccessException, e) {
        std::cerr << "Hilo principal capturó excepción: " << e.what() << std::endl;
    }
    
    // Esperar a que el hilo trabajador termine
    t.join();
    
    // Limpiar recursos del hilo principal
    try_catch_guard.hpp|try_catch_guard::unregisterThreadHandler();
    
    return 0;
}
```

## Detalles de Implementación

### Manejo de Señales

TryCatchGuard utiliza las facilidades estándar de manejo de señales de C++ para instalar un manejador de señales personalizado para señales SIGSEGV. El manejador de señales se instala una vez por proceso utilizando la función `installGlobalHandlerOnce()`, que configura una acción de señal utilizando `sigaction()`.

### Contexto Específico de Hilo

Cada hilo que utiliza TryCatchGuard tiene su propio contexto, almacenado en una variable local de hilo (`currentThreadContext`). Este contexto incluye:

- Una pila de buffers de salto para bloques try anidados
- Una bandera que indica si el manejador está activo
- Un puntero a función para el manejador de señales específico del hilo

### Pila de Buffers de Salto

Para soportar bloques try anidados, TryCatchGuard mantiene una pila de buffers de salto. Cuando se ingresa a un bloque `_try`, se empuja un nuevo buffer de salto a la pila. Cuando ocurre un fallo de segmentación, el manejador de señales utiliza el buffer de salto superior para devolver el control al bloque `_try` más reciente.

### Propagación de Excepciones

Cuando se captura un fallo de segmentación, TryCatchGuard lanza una excepción personalizada `InvalidMemoryAccessException` con un mensaje de error detallado. Esta excepción puede ser capturada utilizando la macro `_catch`, que funciona de manera similar a un bloque catch estándar de C++.

## Notas Importantes

1. En aplicaciones multi-hilo, siempre libere el registro de los manejadores de hilo cuando finalice el uso de los hilos mediante `try_catch_guard.hpp|try_catch_guard::unregisterThreadHandler()`.
2. En aplicaciones de un solo hilo, liberar el registro es opcional pero recomendado si TryCatchGuard solo se usa en secciones específicas.
3. Las macros `_try` y `_catch` deben usarse juntas, similar a los bloques try-catch estándar.
4. TryCatchGuard está diseñado para propósitos de desarrollo y depuración. En entornos de producción, generalmente es mejor corregir los problemas subyacentes de acceso a memoria en lugar de depender de la captura de fallos de segmentación.
5. La biblioteca tiene una pequeña sobrecarga de rendimiento debido al mecanismo de manejo de señales.

## Pruebas y Sanitizadores

Al ejecutar las pruebas, es posible que notes errores inusuales de acceso a memoria mostrados en la pantalla. Esto se debe a que el conjunto de pruebas está configurado con varios sanitizadores habilitados en el archivo `tests/CMakeLists.txt`:

```cmake
# Enable Address Sanitizer and Undefined Behavior Sanitizer
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
```

Estos sanitizadores incluyen:
- **AddressSanitizer (ASan)**: Detecta errores de memoria como desbordamientos de buffer, uso después de liberación y fugas de memoria
- **UndefinedBehaviorSanitizer (UBSan)**: Detecta comportamiento indefinido como desbordamiento de enteros, desreferenciación de punteros nulos, etc.
- **LeakSanitizer**: Detecta fugas de memoria

El entorno de prueba también está configurado con variables de entorno especiales para permitir que TryCatchGuard maneje fallos de segmentación mientras los sanitizadores están activos:

```cmake
set_tests_properties(memory_guard_tests PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=handle_segv=0:allow_user_segv_handler=1:detect_leaks=0"
)
```

### Ejemplo de Salida de Pruebas

Al ejecutar las pruebas, verás numerosos mensajes de error de tiempo de ejecución como los siguientes:

```
Randomness seeded to: 4243962057
.../tests/memory_guard_tests.cpp:17:14: runtime error: store to null pointer of type 'int'
.../tests/memory_guard_tests.cpp:76:22: runtime error: store to null pointer of type 'int'
.../tests/memory_guard_tests.cpp:115:34: runtime error: store to null pointer of type 'int'
Outer try block stack size: 1
Outer jmpbuf address: 0x514000000c40
Inner try block stack size: 2
Inner jmpbuf address: 0x514000000d08
.../tests/memory_guard_tests.cpp:212:22: runtime error: store to null pointer of type 'int'
Stack size after inner catch: 1
Current jmpbuf address after inner catch: 0x514000000c40
Stack size after inner block: 1
Final jmpbuf address: 0x514000000c40
...
Throwing a standard C++ exception inside _try block...
Caught standard exception: Standard C++ exception
Throwing a custom exception inside _try block...
Caught custom exception: Custom exception message
Testing exception type 0...
.../tests/memory_guard_tests.cpp:856:26: runtime error: store to null pointer of type 'int'
Caught memory access exception: Invalid null pointer access exception
Testing exception type 1...
Caught standard exception: Standard C++ exception
Testing exception type 2...
Caught custom exception: Custom exception message
...
===============================================================================
All tests passed (73 assertions in 18 test cases)
```

**Importante**: Estos mensajes de "runtime error" son esperados y no indican problemas reales con el código. Son generados por el UndefinedBehaviorSanitizer cuando detecta desreferenciaciones de punteros nulos, que es exactamente lo que muchas de nuestras pruebas están haciendo intencionalmente para verificar que TryCatchGuard capture correctamente estos problemas.

La línea final "All tests passed" confirma que TryCatchGuard está funcionando correctamente, capturando y manejando con éxito todas las violaciones de acceso a memoria intencionales.

Estos sanitizadores son herramientas de desarrollo valiosas que ayudan a identificar posibles problemas, pero pueden producir una salida detallada que podría parecer errores incluso cuando las pruebas están pasando con éxito. Los errores que ves a menudo son casos de prueba intencionales que verifican la capacidad de TryCatchGuard para capturar y manejar violaciones de acceso a memoria.

## Limitaciones

- **Bloques Try Anidados**: La biblioteca soporta completamente bloques try anidados hasta 3 niveles de anidación, lo cual ha sido probado exhaustivamente. Aunque niveles más profundos de anidación no han sido explícitamente probados, se espera que la implementación funcione correctamente con más niveles de anidación (con aproximadamente un 70% de confianza). El mecanismo de pila de buffers de salto debería teóricamente manejar cualquier número de niveles de anidación.
- **Compatibilidad de Plataforma**: La biblioteca está diseñada principalmente para sistemas Linux/Unix y puede no funcionar correctamente en todas las plataformas.
- **Limitaciones de Recuperación**: TryCatchGuard no puede recuperarse de todos los tipos de violaciones de acceso a memoria. Algunas corrupciones de memoria graves pueden seguir causando que el programa se bloquee.
- **Conflictos de Manejadores de Señales**: La biblioteca puede entrar en conflicto con otras bibliotecas que instalan sus propios manejadores de señales SIGSEGV. El script incluido `modify_catch2.sh` aborda esto para el framework de pruebas Catch2.
- **Sobrecarga de Rendimiento**: Hay una pequeña sobrecarga de rendimiento debido al mecanismo de manejo de señales, especialmente en aplicaciones multi-hilo.
