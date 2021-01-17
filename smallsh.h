#include <stdio.h>
#include <stdlib.h>


/* ■
* Costante usata per abilitare la stampa di tutto il codice usato per debuggare.
* 0 -> nascondi tutte le stampe;
* 1 -> mostra tutte le stampe. */
#define DBG 1


/* ■
* Costante che determina la lunghezza massima del prompt da stampare in ogni
* riga. Se la dimensione eccede viene semplicemente trocata. */
#define PROMPT_MAX_SIZE 128


/* ■
* Costante che determina come deve essere il prompt solo ed esclusivamente se il
* normale caricamento del prompt tramite le variabili d'ambiente restituisce
* un errore. */
#define DEFAULT_PROMPT "> "


/* ■
* Definizioni di tutti i tipi di simboli usati nella riga di comando.*/
#define EOL 1           // Simbolo: "End of line" 
#define ARG 2		    // Simbolo: "Argument" 
#define AMPERSAND 3 	// Simbolo: "&" 
#define SEMICOLON 4	    // Simbolo: ";" 


/* ■
* Definizioni di tutte le funzioni usate nel programma. */

/**
* @brief Carica il prompt passato come parametro con i valori delle
* variabili d'ambiente USER e HOME.
* 
* @param prompt il puntatore al primo char della stringa
* 
* @return int Ritorna 1 se è stato possibile scrivere tutte le variabili
* d'ambiente nel prompt senza superare la PROMPT_MAX_SIZE.
* Ritorna 0 se dimensione totale del prompt non è sufficiente per memorizzare
* tutto; in tale caso il prompt è costituito solo dalla variabile USER.
* Ritorna -1 se si è verificato un errore; in tale caso si setta come prompt
* ciò che c'è in DEFAULT_PROMPT.
*/
int loadEnv(char* prompt);