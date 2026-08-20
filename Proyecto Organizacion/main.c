#include <stdio.h>

// Declaración externa de la función de pruebas
// Se declara aquí porque main.c no incluye tests.h; basta con avisar
// al compilador que esta función existe en otro archivo.
void run_all_tests();

int main() {
    printf("Iniciando simulador MIPS...\n");
    // Ejecuta toda la batería de pruebas unitarias definida en tests.c.
    // Cada prueba carga una instrucción en memoria, corre las 5 etapas
    // del pipeline y verifica (con assert) que el resultado sea el
    // esperado.
    run_all_tests();

    return 0;
    
}
