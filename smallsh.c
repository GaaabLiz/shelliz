#include "smallsh.h"  
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


/* Scritta da stampare a ogni chiamata di userin (A ogni riga di comando).
All'avvio del programma viene caricata questa variabile con opportuni valori
delle variabili d'ambiente dell'utente. Viene inizialmente settata a null
in quanto verrà poi allocata nel main la memoria necessaria. */
char *prompt = NULL;

/* Tipologia del processo da eseguire in runcommand */
processtype pt;


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

    /* Definzione del ciclo infinito che gestisce la shell. Finche non viene
    rilevato un EOF oppure un segnale di terminazione viene continuamente 
    chiesto all'utente di inserire nuove linee di comando tramite la 
    funzione userin. Se tutto va bene, la funzione procline processa quello 
    che ha scritto l'utente. */
    while(userin(prompt) != EOF) procline();
    
    return EXIT_SUCCESS;
}




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



void procline(void){

    /* Dichiarazioni di alcune variabili */
    char *arg[MAXARG+1];    /* array di puntatori per runcommand */
    int toktype;  	        /* tipo del simbolo nel comando */
    int narg;		        /* numero di argomenti attuali */

    /* Inizializzo il numero di argomenti 0 a zero perchè siamo all'inizio
    del processamento della riga attuale. */
    narg=0;

    /* Ciclo che processa ogni singolo argomento da passare a runcommand */
    do {

        /* Chiamare la funzione gettok che mette un simbolo in arg[narg].
        Una volta inserito il simbolo in arg, si esegue un'azione a seconda 
        del tipo di simbolo inserito. */
        switch (toktype = gettok(&arg[narg])) {
            
            /* Se argomento: passa al prossimo simbolo, vuoldire che l'intero argomento è già
            presente in arg, ora devo incrementare narg perchè potrebbero esserci altri argomenti.
            (ovviamente se narg < MAXARG) */
            case ARG:
            if(DBG)printf("[PROCLINE]: gettok has retuned ARG; increasing narg...\n");
            if (narg < MAXARG) narg++;
            break;
            
            /* se fine riga, ';' o '&', esegue il comando ora contenuto in arg,mettendo NULL 
            per indicare la fine degli argomenti che servono a execvp */
            case EOL:
            case SEMICOLON:
            case AMPERSAND:

            /* Calcolo tipo di processo */
            pt = (toktype == AMPERSAND) ? PROCESS_BACKGROUND : PROCESS_FOREGROUND;
            if(DBG) printf("[PROCLINE]: Current process type is '%s'.\n", (toktype == AMPERSAND) ? "Background" : "Foreground");

            /* Controllo argomenti ed eseguo comando */
            if (narg != 0) {
                arg[narg] = NULL;
                if(DBG) printArgArray(arg, narg);
                runcommand(arg, pt);
            }else {
                //fprintf(stderr, "Some error occurred with the number of argument to pass at exec.");
            }
            
            /* Se fine riga (toktype = EOF) si esce dal ciclo e la procline è terminata.
            Altrimenti (toktype = & || toktype = ;) bisogna ricominciare a 
            riempire arg dall'indice 0 */
            if (toktype != EOL) {
                if(DBG) printf("[PROCLINE]: Toktype is SEMICOLON or AMPERSAND, so i need to reset narg.\n");
                narg = 0;
            } else {
                if(DBG) printf("[PROCLINE]: Toktype is EOF. Processing of current input string is over.\n");
            } 
            break;
        }
    } while (toktype != EOL);  /* fine riga, procline finita */

}


void runcommand(char **cline, processtype pt){
    pid_t pid;      // pid del processo
    int exitstat;   // status di uscita della wait
    //int ret;      // valore di ritorno della wait

    pid = fork();
    if (pid == (pid_t) -1) {
        perror("smallsh: fork failed");
        return;
    }

    /* processo figlio */
    if (pid == (pid_t) 0) { 	
        /* esegue il comando il cui nome e' il primo elemento di cline,passando 
        cline come vettore di argomenti */
        execvp(*cline,cline);
        perror(*cline);
        exit(EXIT_FAILURE);
    }else { /* processo padre */
        if(pt == PROCESS_BACKGROUND) {
            printf("[RUNCOMMAND]: Process %s open on background with pid %d.\n", *cline, (int)pid);        
        }else {
            printf("[RUNCOMMAND]: Process %s open on foreground with pid %d.\n", *cline, (int)pid);
            
            /* Quando il processo è in foreground, l'interprete deve
            ignorare il segnale di interruzzione. */
            struct sigaction sa;
            sa.sa_handler = ignoreSigint;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGINT,&sa,NULL);

            /* Aspetta il figlio appena creato */
            pid = waitpid(pid, &exitstat, 0);
        }

        /* Ripristino il funzionamento del segnale di interruzzione, in quanto
        è possibile, come nel caso del foregound, che è stato ridefinito. */
        struct sigaction sa;
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT,&sa,NULL);
    }

    //if (ret == -1) perror("wait");
}


void printArgArray(char* array[], int narg) {
  for (int i = 0; i <= narg; i++){
    printf("Arg[%d] = ", i);
    printf("'");
    printf("%s", array[i]);
    printf("'\n");
  }
}


void ignoreSigint(int sig) {
    printf(" Received signal %d (SIGINT). Ignoring...\n", sig);
}