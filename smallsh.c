#include "smallsh.h"  
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>


/* Scritta da stampare a ogni chiamata di userin (A ogni riga di comando).
All'avvio del programma viene caricata questa variabile con opportuni valori
delle variabili d'ambiente dell'utente. Viene inizialmente settata a null
in quanto verrà poi allocata nel main la memoria necessaria. */
char *prompt = NULL;


/**
 * @brief Main del programma. 
 * @return int Return status
 */
int main() {

    /* Allocazione di memoria per la stringa prompt. */
    prompt = (char*) malloc(PROMPT_MAX_SIZE);

    /* Caricamento delle variabili d'ambiente nel prompt. */
    if(loadEnv(prompt)) {
        if(DBG) printf("[MAIN]: Enviroment variable succesfully loaded into prompt's string.\n");
        if(DBG) printf("[MAIN]: Prompt set to '%s' with size %ld/%d.\n", prompt, strlen(prompt), PROMPT_MAX_SIZE);
    }else if (loadEnv(prompt) == 0) {
        if(DBG) printf("[MAIN]: prompt's string exceeds PROMPT_MAX_SIZE (%d).\n", PROMPT_MAX_SIZE);
        if(DBG) printf("[MAIN]: Prompt set to '%s'.\n", prompt);
    }else {
        if(DBG) printf("[MAIN]: An error occurred while creating prompt's string.\n");
        if(DBG) printf("[MAIN]: Prompt set to '%s'.\n", prompt);
    }

    
    return 0;
}



/** ■ **/
int loadEnv(char * prompt) {
    int returnStatus = -1;          // Valore di ritorno
    int symbolAuxLenght = 3;        // Numero di simboli ausiliari ('%', ':')
    char* symbol1 = "%";            // Simbolo percentuale inizio prompt       
    char* symbol2 = ":";            // Simbolo separatore   
    char* envHome = getenv("HOME"); // Variabile d'ambiente HOME
    char* envUser = getenv("USER"); // Variabile d'ambiente USER

    /* Se variabili d'ambiente nulle ritorna -1 per segnalare errore */
    if( (envHome == NULL) || (envUser == NULL)) {
        strcpy(prompt, DEFAULT_PROMPT);
        return returnStatus;
    } 

    /* Se variabili d'ambiente settate, calcolo loro dimensione */
    size_t envHomeLenght = strlen(envHome);
    size_t envUserLenght = strlen(envUser);

    /* Se dimensione = 0, ritorna errore */
    if( (envHomeLenght == (size_t) 0) || (envUserLenght == (size_t) 0)) {
        strcpy(prompt, DEFAULT_PROMPT);
        return returnStatus;
    } 

    /* Se dimensione totale minore di PROMPT_MAX_SIZE, allora carica il prompt
    nella stringa puntata da prompt. Se dimensione non sufficiente si prende 
    solamente la variabile d'ambiente USER. */
    if( (envHomeLenght + envUserLenght + symbolAuxLenght + 1) <= PROMPT_MAX_SIZE) {
        strcpy(prompt, symbol1);
        strcat(prompt, envUser);
        strcat(prompt, symbol2);
        strcat(prompt, envHome);
        strcat(prompt, symbol2);
        returnStatus = 1;
    } else {
        strcpy(prompt, symbol1);
        strcat(prompt, envUser);
        strcat(prompt, symbol2);
        returnStatus = 0;
    }
    
    return returnStatus;
    
}
