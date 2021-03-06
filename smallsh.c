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

/* Stringa di buffer temporanea usata per memorizzare il valore attuale
della variabile d'ambiente BPID. */
char * bpidstr = NULL;

/* Array d'appoggio usato per contenere tutti i pid dei processi in 
background. Usato per gestire tutti i pid che poi verranno aggiunti/
rimossi nella variabile d'ambiente BPID. */
int pidbuffer[MAX_BG_CHILD];

/* Tipologia del processo da eseguire in runcommand */
processtype pt;

/* Definizione di tutti gli handler dei segnali */
struct sigaction safree;    // Handler per deallocare la memoria
struct sigaction safign;    // Handler per processo padre in caso di figlio fg





/**
 * @brief Main del programma. 
 * @return int Return status
 */
int main() {

    /* Allocazione di memoria per la stringa prompt. */
    prompt = (char*) malloc(PROMPT_MAX_SIZE);

    /* Allocazione di memoria per la stringa d'appoggio a BPID */
    bpidstr = (char*) malloc(MAX_BPID_SIZE);

    /* Creazione degli handler personalizzati */
    safree.sa_handler = deallocateOnSignal; // SIGINT no fg
    sigemptyset(&safree.sa_mask);
    safree.sa_flags = 0;
    safign.sa_handler = receiveSignal;      // SIGINT fg
    sigemptyset(&safign.sa_mask);
    safign.sa_flags = 0;


    /* Ridefinizione del segnale SIGINT per la shell. Essendo un ciclo
    infinito questo programma, per terminare il tutto devo inviare un SIGINT
    (quando non sta facendo un foreground). Avendo usato diverse malloc,
    prima di uscire dalla shell devo deallocare la memoria. */
    sigaction(SIGINT, &safree, NULL); 

    /* Caricamento delle variabili d'ambiente nel prompt. */
    if(loadEnv(prompt)) {
        if(DBG) printf("[MAIN]: Enviroment variable succesfully loaded into prompt's string.\n");
        if(DBG) printf("[MAIN]: Prompt set to '%s' with size %ld/%d.\n", prompt, strlen(prompt), PROMPT_MAX_SIZE);
    }else if (loadEnv(prompt) == 0) {
        if(DBG) printf("[MAIN]: prompt's string exceeds PROMPT_MAX_SIZE (%d).\n", PROMPT_MAX_SIZE);
        if(DBG) printf("[MAIN]: Prompt set to '%s'.\n", prompt);
    }else {
        fprintf(stderr, "An error occurred while creating prompt's string from enviroment variables.\n");
        if(DBG) printf("[MAIN]: Prompt set to '%s'.\n", prompt);
    }

    /* Creazione della variabile d'ambiente BPID */
    if(putenv("BPID=EMPTY") == 0) {
        if(DBG) printf("[MAIN]: Enviroment variable BPID created succesfully!\n");
    }else {
        fprintf(stderr, "Some error occurred while creating BPID enviroment variable.");
        perror("BPID");
    }

    /* Inizializzazione array d'appoggio dei pid (per BPID) */
    for(int i = 0; i < MAX_BG_CHILD; i++) pidbuffer[i] = PID_EMPTY_SLOT;

    /* Definzione del ciclo infinito che gestisce la shell. Finche non viene
    rilevato un EOF oppure un segnale di terminazione viene continuamente 
    chiesto all'utente di inserire nuove linee di comando tramite la 
    funzione userin. Se tutto va bene, la funzione procline processa quello 
    che ha scritto l'utente. */
    while(userin(prompt) != EOF) procline();
    
    /* Deallocazione memoria nel caso si arrivi qui */
    free(prompt);
    free(bpidstr);

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

            /* Se gettok restituisce un CUSTOM vuoldire che l'argomento attuale
            (che è stato cmq inserito in arg) è un argomento che corrisponde
            a un comando personalizzato. */
            case CUSTOM:
                if(narg == 0) {
                    if(DBG) printArgArray(arg, narg);
                    executeCustomCommand(arg[0]);
                    toktype = EOL;
                } else {
                    if (narg < MAXARG) narg++;
                }
            break;
        }
    } while (toktype != EOL);  /* fine riga, procline finita */

}




void runcommand(char **cline, processtype pt){
    pid_t pid;      // pid del processo
    int exitstat;   // status di uscita della wait
    int ret;        // valore di ritorno della wait

    pid = fork();
    if (pid == (pid_t) -1) {
        perror("smallsh: fork failed");
        return;
    }

    /* PROCESSO FIGLIO -----*/
    if (pid == (pid_t) 0) { 
        /* esegue il comando il cui nome e' il primo elemento di cline,passando 
        cline come vettore di argomenti */
        execvp(*cline,cline);
        perror(*cline);
        exit(EXIT_FAILURE);
    }


    /* PROCESSO PADRE ------*/

    /* Controlla la terminazione di tutti i processi in background e li segnala */
    checkbackgroundChild();

    /* Il base al tipo di processo avviato eseguo una determinata azione */
    switch (pt){

    case PROCESS_BACKGROUND:
        printf("Process '%s' open on background with pid %d.\n", *cline, (int)pid);
        if(addPidToBPID((int)pid) == -1) fprintf(stderr, "Error occurred while adding pid into BPID\n");
        sigaction(SIGINT, &safree, NULL); 
        break;

    case PROCESS_FOREGROUND:
        printf("Process '%s' open on foreground with pid %d.\n", *cline, (int)pid);

        /* Quando il processo è in foreground, l'interprete deve
        ignorare il segnale di interruzzione. Ma il figlio (che è in foreground)
        deve terminare! */
        sigaction(SIGINT, &safign, NULL);

        /* Aspetta il figlio appena creato e stampa info sulla terminazione*/
        ret = waitpid(pid, &exitstat, 0);
        if(DBG) printf("[RUNCOMMAND]: foreground process terminated with returned value = %d\n", ret);
        if(DBG) printf("[RUNCOMMAND]: status of waitpid = %d\n", exitstat);
        if (ret == -1) {
            //perror("waitpid");
            printf("\nForeground process interrupted by signal.\n\n");
        }else {
            checkForegroundStatus(exitstat);
        }
        
        break;
    
    default:
        fprintf(stderr, "Some error occurred while checking process type.\n");
        break;
    }
    
}




void executeCustomCommand(char * commandName) {

    /* Controlla la terminazione di tutti i processi in background e li segnala.
    Questo è necessario ripeterlo qua perchè era presente solo in runccomand. */
    checkbackgroundChild();
  
    /* Comando bp */
    if(strcmp(commandName, "bp") == 0) {
        fprintf(stdout, "BPID=%s\n\n", getenv("BPID"));
        if(DBG) printf("[PROCLINE]: Executed 'bp' custom command!\n");
    } 
    /* errore */
    else {
        if(DBG) printf("[PROCLINE]: Error! Unknown custom command!\n");
    } 
}




void checkForegroundStatus(int wstatus) {
    if(WIFEXITED(wstatus)) {
        printf("Exit value: %d\n\n", WEXITSTATUS(wstatus));
    } 
}




void checkbackgroundChild() {
    int pid;
    int status;

    /* Questo ciclo controlla se ci sono processi che sono terminati in
    background. Se non ci sono processi che sono usciti trami con il
    flag WNOHANG presente, la waitpid ritorna subito. Se ce un processo terminato
    si cattura e la funziona ritorna il suo pid. */
    do{
        pid = waitpid(-1, &status, WNOHANG);
        if(pid > 0) { /* necessario per capire se pid o altro */
            if(WIFEXITED(status)) {
                printf("Process with pid %d terminated with exit value %d.\n", pid, WEXITSTATUS(status));
            }else if (WIFSIGNALED(status)) {
                printf("Process with pid %d terminated with signal %d.\n", pid, WTERMSIG(status));
            }
            if(removePidToBPID((int)pid) == -1) fprintf(stderr, "Error occurred while removing pid from BPID\n");
        }
    } while (pid > 0);    
}




int addPidToBPID(int pid) {
    /* Prima di aggiungere un pid cerco se esiste già. In tal caso 
    lancio errore. */
    if(searchPidonBPID(pid) == 1) {
        if(DBG) printf("[BPID]: Error! pid %d can't be added because is already present!", pid);
        return -1;
    }
    /* Se tutto va bene inserisco nel vettore il pid */
    else {
        /* Prendo dim attuale per successivo confronto */
        int countBefore = getBpidSize();    

        /* Se non ce un posto libero (quindi vettore pieno) ritorno errore */
        int emptyCount = 0;
        for (size_t i = 0; i < MAX_BG_CHILD; i++){
            if(pidbuffer[i] == PID_EMPTY_SLOT) {
                emptyCount++;
            }
        }
        if(emptyCount == 0) {
            if(DBG) printf("[BPID]: Error! Pid can't be added! buffer is full!\n");
            return -1;
        }
        
        /* Prendo il primo posto libero nel vettore e gli metto il pid */
        for (size_t i = 0; i < MAX_BG_CHILD; i++){
            if(pidbuffer[i] == PID_EMPTY_SLOT) {
                pidbuffer[i] = pid;
                if(DBG) printf("[BPID]: added pid %d on position %d inside buffer.\n", pid, (int)i);
                break;
            }
        }

        /* Aggiorna la variabile d'ambiente con i valori dell'array */
        int countAfter = getBpidSize();
        if(updateBPID() && (countAfter == (countBefore+1))) {
            return 1;
        }else {
            return -1;
        }

    }
}




int removePidToBPID(int pid) {
    
    /* Controllo dimensione != 0 */
    if(getBpidSize() == 0) return 0;

    /* Controllo se presente */
    if(searchPidonBPID(pid) == 0) return -1;

    /* Prendo dimensione attuale */
    int countBefore = getBpidSize();

    /* Tentativo rimozione */
    for (size_t i = 0; i < MAX_BG_CHILD; i++){
        if(pidbuffer[i] == pid) {
            pidbuffer[i] = PID_EMPTY_SLOT;
            int countAfter = getBpidSize();
            if(updateBPID() && (countAfter == (countBefore-1))) {
                if(DBG) printf("[BPID]: pid %d removed from the buffer and env updated!.\n", pid);
                return 1;
            }else {
                if(DBG) printf("[BPID]: Some error occurred while updating and deleting pid %d.\n", pid);
                return -1;
            }
        }
    }
    return -1;
    
}




int updateBPID() {
    
    /* per prima cosa setto subito la variabile BPID a EMPTY, se poi ci sono
    pid nel vettore allora poi inserisco la stringa d'appoggio */
    if(putenv("BPID=EMPTY") == 0) { 
        if(getBpidSize() == 0) {    // NOn ci sono pid. HO finito
            if(DBG) printf("[BPID]: Updated to empty\n");
        }else {

            /* Libero la memoria della precedente stringa d'appoggio */
            free(bpidstr);

            /* Alloco nuova memoria per contenere nuova stringa d'appoggio */
            bpidstr = malloc(MAX_BPID_SIZE);

            /* Avendo allocato nuova memoria devo riscrivere a che variabile
            d'ambiente mi riferisco */
            strcpy(bpidstr, "BPID=");

            /* Prendo tutti i pid nel vettore d'appoggio e gli concateno
            uno ad uno alla stringa. */
            for (size_t i = 0; i < MAX_BG_CHILD; i++){
                char bfint[MAX_PID_CHARINT_SIZE];
                if(pidbuffer[i] != -1) {
                    if(i != 0) strcat(bpidstr, ":");
                    sprintf(bfint, "%d", pidbuffer[i]);
                    strcat(bpidstr, bfint);
                }
            }

            /* Setto la stringa d'appoggio come variabile d'ambiente BPID.
            Controllo eventuale errore */
            if(putenv(bpidstr) != 0) perror("bpid");
        }
        if(DBG) printf("[MAIN]: Enviroment variable BPID updated succesfully!\n");
        return 1;
    }else {
        fprintf(stderr, "Some error occurred while updating BPID enviroment variable.");
        perror("BPID");
        return -1;
    }
}




int searchPidonBPID(int pid) {
    for (size_t i = 0; i < MAX_BG_CHILD; i++){
        if(pidbuffer[i] == pid) return 1;
    }
    return 0;
}




int getBpidSize() {
    int count = 0;
    for (size_t i = 0; i < MAX_BG_CHILD; i++){
        if(pidbuffer[i] != PID_EMPTY_SLOT) count++;
    }
    return count;
}




void printArgArray(char* array[], int narg) {
  for (int i = 0; i <= narg; i++){
    printf("Arg[%d] = ", i);
    printf("'");
    printf("%s", array[i]);
    printf("'\n");
  }
}




void receiveSignal(int sig) {
    /* Segnalo arrivo segnale */
    if (DBG) printf("\nShell Received signal %d.\n", sig);
    
    /* Ridefinisco comportamento default SIGINT padre */
    sigaction(SIGINT,&safree,NULL);
}




void deallocateOnSignal(int sig) {
    free(prompt);
    free(bpidstr);
    if(DBG) printf("Shell received signal %d.\n", sig);
    fprintf(stdout, "\nClosing shell...\n");
    exit(EXIT_SUCCESS);
}