#include <iostream>
#include <sigsegv.h>
#include <csetjmp>
#include <csignal>
#include <stdexcept>
#include <functional>
#include <thread>
#include <vector>

// Estado local a cada hebra
thread_local static sigjmp_buf __segv_jmpbuf;
thread_local static bool __segv_active = false;

// Manejador SIGSEGV
int __segv_handler(void* addr, int serious)
{
    if (__segv_active)
    {
        siglongjmp(__segv_jmpbuf, 1);
    }
    return 0; // SIGSEGV_RETURN_FAILURE
}

// Inicializador único
static void __segv_install_handler_once()
{
    static bool installed = false;
    if (!installed)
    {
        if (sigsegv_install_handler(__segv_handler) < 0)
        {
            std::cerr << "Fallo al instalar el manejador de SIGSEGV" << std::endl;
            std::exit(1);
        }
        installed = true;
    }
}

// Función interna que lanza excepción si salimos con siglongjmp
inline void __segv_try_block(const std::function<void()> &block)
{
    __segv_install_handler_once();

    __segv_active = true;

    if (sigsetjmp(__segv_jmpbuf, 1) == 0)
    {
        block(); // Ejecuta el bloque "_try"
    }
    else
    {
        __segv_active = false;
        throw std::runtime_error("SIGSEGV atrapado: acceso inválido a memoria.");
    }

    __segv_active = false;
}

// Macros _try y _catch
#define _try \
    try      \
    { __segv_try_block([&]()

#define _catch(type, var)                                                                                                                  \
                                                                                                                                        ); \
    }                                                                                                                                      \
    catch (const type &var)

void thread_function(int id)
{
    try
    {
        _try
        {
            std::cout << "Hebra " << id << ": Dentro del bloque _try" << std::endl;
            if (id % 2 == 0)
            {
                int *ptr = nullptr;
                std::cout << "Hebra " << id << ": Intentando acceder a *ptr" << std::endl;
                *ptr = 10; // Esto generará SIGSEGV en hebras pares
            }
            else
            {
                std::cout << "Hebra " << id << ": No accederé a un puntero nulo." << std::endl;
            }
            std::cout << "Hebra " << id << ": Fuera del bloque _try (si no hubo excepción)." << std::endl;
        }
        _catch(std::runtime_error, e)
        {
            std::cerr << "Hebra " << id << ": ¡Excepción capturada!: " << e.what() << std::endl;
        }
    }
    catch (...)
    {
        std::cerr << "Hebra " << id << ": Se capturó otra excepción." << std::endl;
    }
    std::cout << "Hebra " << id << ": Terminando." << std::endl;
}

int main()
{
    const int num_threads = 4;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(thread_function, i);
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    std::cout << "Todas las hebras han terminado." << std::endl;

    return 0;
}
