// Emulação de uma interface serial com E/S de caracteres.

#ifndef __HW_SERIAL__
#define __HW_SERIAL__

// Envia um caractere à interface serial (bloqueante).
// Return: NOERROR ou ERROR.
int hw_putchar(char c);

// Recebe um caractere da interface serial (não bloqueante).
// Return: caractere ASCII ou 0 se não há nada a ser lido.
char hw_getchar();

#endif
