#include <stdio.h>
#include <stdlib.h>

/* Costante usata per abilitare la stampa di tutto il codice usato per debuggare.
0 -> nascondi tutte le stampe;
1 -> mostra tutte le stampe. */
#define DBG 0           

/* Definizioni di tutti i tipi di simboli usati nella riga di comando.*/
#define EOL 1           // Simbolo: "End of line" 
#define ARG 2		    // Simbolo: "Argument" 
#define AMPERSAND 3 	// Simbolo: "&" 
#define SEMICOLON 4	    // Simbolo: ";" 