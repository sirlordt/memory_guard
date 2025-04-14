# Biblioteca MemoryGuard

## Descripción General

MemoryGuard es una biblioteca C++ diseñada para capturar y manejar fallos de segmentación (SIGSEGV) de manera controlada. Proporciona un mecanismo similar a try-catch para manejar accesos inválidos a memoria, como desreferenciación de punteros nulos o acceso a direcciones de memoria inválidas, sin que la aplicación se bloquee.

## Características

- Captura y manejo de fallos de segmentación utilizando una sintaxis familiar tipo try-catch
- Implementación segura para aplicaciones multi-hilo
- Mensajes de error detallados que incluyen el tipo de violación de acceso a memoria
- Soporte tanto para desreferenciación de punteros nulos como para otros accesos inválidos a memoria
- Mínima sobrecarga de rendimiento cuando no ocurren excepciones

## Cómo Funciona

MemoryGuard utiliza la biblioteca `libsigsegv` para instalar un manejador de señales personalizado para señales SIGSEGV. Cuando ocurre un acceso inválido a memoria dentro de un bloque `_try`, el manejador de señales captura el fallo, registra información sobre la dirección del fallo y utiliza `siglongjmp` para devolver el control al bloque `_try`. La biblioteca entonces lanza una excepción personalizada `InvalidMemoryAccessException` que puede ser capturada utilizando la macro `_catch`.

## Uso

### Uso Básico

```cpp
#include <iostream>
#include "MemoryGuard.hpp"

void ejemplo() {
    _try {
        // Código que podría causar un fallo de segmentación
        int* ptr = nullptr;
        *ptr = 10; // Esto normalmente bloquearía el programa
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
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
    MemoryGuard::unregisterThreadHandler();
    
    return 0;
}
```

### Seguridad en Hilos

MemoryGuard está diseñado para ser seguro en entornos multi-hilo. Cada hilo registra su propio manejador, y la biblioteca mantiene contextos específicos para cada hilo para asegurar que los fallos de segmentación se manejen correctamente en aplicaciones multi-hilo.

```cpp
#include <iostream>
#include "MemoryGuard.hpp"
#include <thread>

void funcion_hilo(int id) {
    _try {
        // Código que podría causar un fallo de segmentación
        if (id % 2 == 0) {
            int* ptr = nullptr;
            *ptr = 10; // Esto normalmente bloquearía el programa
        }
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        std::cerr << "Hilo " << id << " capturó excepción: " << e.what() << std::endl;
    }
    
    // Importante: Liberar el registro del manejador cuando el hilo termina
    MemoryGuard::unregisterThreadHandler();
}

int main() {
    std::thread t1(funcion_hilo, 1);
    std::thread t2(funcion_hilo, 2);
    
    t1.join();
    t2.join();
    
    std::cout << "Todos los hilos han terminado correctamente" << std::endl;
    
    return 0;
}
```

### Diferentes Tipos de Violaciones de Acceso a Memoria

MemoryGuard puede manejar diferentes tipos de accesos inválidos a memoria:

```cpp
#include <iostream>
#include "MemoryGuard.hpp"

void probar_puntero_nulo() {
    _try {
        // Desreferenciación de puntero nulo
        std::cout << "Intentando acceder a un puntero nulo..." << std::endl;
        int* ptr_nulo = nullptr;
        *ptr_nulo = 10; // Esto lanzará una InvalidMemoryAccessException
        std::cout << "Esta línea no se ejecutará" << std::endl;
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
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
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
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
    MemoryGuard::unregisterThreadHandler();
    
    return 0;
}
```

## Cuándo Usar unregisterThreadHandler()

### Aplicaciones de Un Solo Hilo

En aplicaciones de un solo hilo, llamar a `MemoryGuard::unregisterThreadHandler()` al final del programa es técnicamente opcional ya que el sistema operativo recuperará toda la memoria cuando el proceso termine. Sin embargo, se recomienda encarecidamente hacerlo por las siguientes razones:

1. **Herramientas de Detección de Fugas de Memoria**: Herramientas como Valgrind reportarán fugas de memoria si no liberas explícitamente la memoria asignada por MemoryGuard antes de la terminación del programa.
2. **Gestión de Recursos**: Es una buena práctica limpiar todos los recursos que utiliza tu programa, incluso si el sistema operativo los recuperaría de todos modos.
3. **Aplicaciones de Larga Duración**: Si estás usando MemoryGuard en una aplicación de larga duración donde solo lo necesitas para secciones específicas, debes llamar a `unregisterThreadHandler()` después de esas secciones para liberar recursos inmediatamente.

```cpp
#include <iostream>
#include "MemoryGuard.hpp"

int main() {
    std::cout << "Iniciando ejemplo de un solo hilo..." << std::endl;
    
    // Sección donde necesitamos MemoryGuard
    _try {
        std::cout << "Sección protegida..." << std::endl;
        // Algún código que podría causar fallos de segmentación
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        std::cerr << "Excepción capturada: " << e.what() << std::endl;
    }
    
    // Si ya no necesitamos MemoryGuard, podemos liberar el registro del manejador
    MemoryGuard::unregisterThreadHandler();
    
    std::cout << "Continuando con el resto del programa..." << std::endl;
    // Resto del programa donde no necesitamos MemoryGuard
    
    return 0;
}
```

### Aplicaciones Multi-Hilo

En aplicaciones multi-hilo, es importante llamar a `MemoryGuard::unregisterThreadHandler()` al final de cada función de hilo para evitar fugas de recursos. Esto es especialmente importante para hilos de trabajo que se crean y destruyen con frecuencia.

Es crucial entender que la función main también se ejecuta en un hilo (el "hilo principal"). Si el hilo principal usa MemoryGuard directamente, también debe llamar a `unregisterThreadHandler()` antes de finalizar para evitar fugas de memoria que serían detectadas por herramientas como Valgrind.

```cpp
#include <iostream>
#include <thread>
#include "MemoryGuard.hpp"

void hilo_trabajador() {
    // Código del hilo usando MemoryGuard
    _try {
        // Algún código que podría causar fallos de segmentación
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        std::cerr << "Hilo trabajador capturó excepción: " << e.what() << std::endl;
    }
    
    // Limpiar recursos del hilo
    MemoryGuard::unregisterThreadHandler();
}

int main() {
    // Crear y ejecutar hilos trabajadores
    std::thread t(hilo_trabajador);
    
    // El hilo principal también usa MemoryGuard
    _try {
        // Algún código en el hilo principal que podría causar fallos de segmentación
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        std::cerr << "Hilo principal capturó excepción: " << e.what() << std::endl;
    }
    
    // Esperar a que el hilo trabajador termine
    t.join();
    
    // Limpiar recursos del hilo principal
    MemoryGuard::unregisterThreadHandler();
    
    return 0;
}
```

## Notas Importantes

1. En aplicaciones multi-hilo, siempre libere el registro de los manejadores de hilo cuando finalice el uso de los hilos mediante `MemoryGuard::unregisterThreadHandler()`.
2. En aplicaciones de un solo hilo, liberar el registro es opcional pero recomendado si MemoryGuard solo se usa en secciones específicas.
3. Las macros `_try` y `_catch` deben usarse juntas, similar a los bloques try-catch estándar.
4. MemoryGuard está diseñado para propósitos de desarrollo y depuración. En entornos de producción, generalmente es mejor corregir los problemas subyacentes de acceso a memoria en lugar de depender de la captura de fallos de segmentación.
5. La biblioteca tiene una pequeña sobrecarga de rendimiento debido al mecanismo de manejo de señales.

## Dependencias

- `libsigsegv`: Una biblioteca para manejar fallos de página en modo usuario

## Limitaciones

- MemoryGuard no puede recuperarse de todos los tipos de violaciones de acceso a memoria. Algunas corrupciones de memoria graves pueden seguir causando que el programa se bloquee.
- La biblioteca está diseñada principalmente para sistemas Linux/Unix y puede no funcionar correctamente en todas las plataformas.
- El uso de MemoryGuard no elimina la necesidad de prácticas adecuadas de gestión de memoria.
