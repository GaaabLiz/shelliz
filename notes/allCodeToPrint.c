
  /$$$$$$  /$$   /$$ /$$$$$$$$ /$$       /$$       /$$$$$$ /$$$$$$$$
 /$$__  $$| $$  | $$| $$_____/| $$      | $$      |_  $$_/|_____ $$ 
| $$  \__/| $$  | $$| $$      | $$      | $$        | $$       /$$/ 
|  $$$$$$ | $$$$$$$$| $$$$$   | $$      | $$        | $$      /$$/  
 \____  $$| $$__  $$| $$__/   | $$      | $$        | $$     /$$/   
 /$$  \ $$| $$  | $$| $$      | $$      | $$        | $$    /$$/    
|  $$$$$$/| $$  | $$| $$$$$$$$| $$$$$$$$| $$$$$$$$ /$$$$$$ /$$$$$$$$
 \______/ |__/  |__/|________/|________/|________/|______/|________/
                                                                    
                                                                    
                                                                  
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

/* Pid del processo in foreground che deve essere terminato 
con un segnale */
pid_t tempPid = 0;

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
    safree.sa_handler = deallocateOnSignal;
    sigemptyset(&safree.sa_mask);
    safree.sa_flags = 0;
    safign.sa_handler = receiveSignal;
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









#include <stdio.h>
#include <stdlib.h>


/* ■
* Costante usata per abilitare la stampa di tutto il codice usato per debuggare.
* 0 -> nascondi tutte le stampe;
* 1 -> mostra tutte le stampe. */
#define DBG 0


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
#define CUSTOM 5        // Comando personalizzato


/* ■
* Definizione della lunghezza massima della riga di input. */
#define MAXBUF 512


/* ■
* Definizione del numero massimo di argomenti all'interno del comando */
#define MAXARG 512


/* ■
* Definizione del numero massimo processi in background. Usato per gestire
* l'array di pid che popolerà la variabile d'ambiente BPID. */
#define MAX_BG_CHILD 1024


/* ■
* Definizione del numero intero usato per indicare uno slot vuoto 
* nell'array d'appoggio per BPID. */
#define PID_EMPTY_SLOT -1


/* ■
* Dimensione della stringa d'appoggio usata per memorizzare la
* variabile d'ambiente BPID. */
#define MAX_BPID_SIZE 4096


/* ■
* Definizione del numero massimo di cifre che un pid può avere
* sottoforma di stringa. Essendo un pid di tipo PID_T, che in realtà
* è un intero, uso il numero massimo di cifre che un int ha.
* Questo valore è usato per costruire un vettore d'appoggio che
* verrà usato per convertire un pid (preso dal vettore) da intero
* a stringa. */
#define MAX_PID_CHARINT_SIZE 8


/* ■
* Definizione della variabile 'processtype' usata per distinguere la
* tipologia di processo che la runcommand dovrà chiamare. */
typedef int processtype;
#define PROCESS_FOREGROUND 1
#define PROCESS_BACKGROUND -1


/* ■
* Definizione del numero attuale di comandi personalizzati
* presenti nella shell. Usato per scorrere l'array 
* customCommands[]. */
#define CUSTOM_COMMANDS_COUNT 1



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

/**
 * @brief Stampa il prompt e legge l'intera riga carattere per carattere
 * inserendola in un buffer fino a trovare un EOF. 
 * 
 * @param p Il prompt da stampare all' inizio della riga da leggere.
 * @return int Ritorna EOF se viene rilevato, oppure restituisce il numero 
 * di caratteri letti e inseriti nel buffer. Notare che se ritorna EOF il
 * programma termina.
 */
int userin(char *p);

/**
 * @brief Stampa l'intero contenuto di inputbuf. Usato solo per scopi
 * di debug.
 * 
 * @param inputbuf L'array di char contenente la stringa digitata dall'utente.
 * @param dim La dimensione dell'array.
 */
void printInputbuf(char* inputbuf, int dim);

/**
 * @brief Processa un intera riga di input digitata dall'utente.
 * Questa funzione si occupa del riconoscimento e della gestione dei vari token
 * contenuti in inputbuf. Una volta gestiti tutti tramite gettok si passa
 * a runcommand tutte le informazioni necessarie per avviare il processo.
 */
void procline();

/**
 * @brief Legge un intero simbolo e lo inserisce in tokbuf (e quindi nel
 * vettore da passare alla runcommand). 
 * 
 * @param outptr L'array di stringhe da passare alla runcommand. 
 * @return int Il tipo del simbolo trovato e inserito nel buffer/arg.
 */
int gettok(char **outptr);	

/**
 * @brief Verifica se C è un carattere speciale (contenuto nell'array 
 * di caratteri speciali dichiarati all'inzio).
 * 
 * @param c Il carattere da controllare
 * @return int Ritorna 0 se è speciale (quindi c è uno dei caratteri
 * contenuti nell'array special), altrimenti ritorna 1.
 */
int inarg(char c);

/**
 * @brief Funzione usata solo per scopi di debug.
 * Converte un simbolo sottoforma di intero in char*
 * 
 * @param type int Valore intero che rappresenta il tipo
 * @return char* valore stringa che rappresenta il tipo
 */
char * convertTypeToString(int type);

/**
 * @brief Funzione usata solo per scopi di debug.
 * 
 * @param array il vettore degli argomenti da passare alla exec.
 * @param narg Il numero di argomenti presenti nel vettore.
 */
void printArgArray(char* array[], int narg);

/**
 * @brief Crea un nuovo processo passando alla exec il vettore di
 * argomenti creato con gettok. Notare che il comportamento sarà
 * diverso a secondo del tipo di processo: se sarà un processo 
 * aperto in foreground la shell aspetterà la terminazione del
 * processo lanciato tramite la wait. Se il processo è di tipo
 * background, verrà lanciato e non aspettato, quindi la shell
 * proseguirà a chiedere una nuova riga.
 * 
 * @param cline il vettore di argomenti da passare alla exec
 * @param pt Il tipo di processo da far partire
 */
void runcommand(char **cline, processtype pt);	

/**
 * @brief Funzione che sovrascrive il comportamento del
 * segnale di interruzzione SIGINT nel caso la shell lanci
 * un processo in foreground.
 * 
 * @param sig Intero che rappresenta il segnale.
 */
void receiveSignal(int sig);

/**
 * @brief Funzione da chiamare subito dopo aver aspettato
 * il processo in foreground, in modo tale da stampare
 * il valore di uscita del processo appena terminato.
 * 
 * @param wstatus Variabile che ho passato alla wait
 * per memorizzare lo status di uscita.
 */
void checkForegroundStatus(int wstatus);

/**
 * @brief Funzione che controlla tramite un ciclo se 
 * ci sono processi in background che hanno finito.
 * Se ce ne sono la funziona stampa informazioni
 * sulla loro terminazione.
 */
void checkbackgroundChild();

/**
 * @brief Aggiunge un pid alla variabile d'ambiente BPID
 * 
 * @param pid il pid da aggiungere 
 * @return int Restituisce 1 se l'aggiunta è andata a buon fine.
 * Restituisce -1 se si è verificato un errore.
 */
int addPidToBPID(int pid);

/**
 * @brief Rimuovi un pid dalla variabile d'ambiente BPID
 * 
 * @param pid il pid da rimuovere
 * @return int Restituisce 1 se il pid è stato rimosso. 
 * Restituisce -1 se si è verificato un errore (come pid non trovato).
 * Restituisce 0 se la variabile d'ambiente non ha nessun pid.
 */
int removePidToBPID(int pid);

/**
 * @brief Funzione che conta il numero di pid presenti nella variabile
 * d'ambiente (quindi in pratica conta il numero di processi in background)
 * 
 * @return int Il numero di pid nella variabile.
 */
int getBpidSize();

/**
 * @brief Cerca un pid all'interno della variabile BPID
 * 
 * @param pid Il pid da cercare
 * @return int Restituisce 1 se il pid cercato è presente.
 * Restituisce 0 se il pid cercato non è presente.
 */
int searchPidonBPID(int pid);

/**
 * @brief Aggiorna la stringa d'appoggio per la variabile BPID in
 * modo che contenga tutto il contenuto che deve essere settato
 * tramite la setenv(). Una volta riempita la stringa con tutti i
 * pid viene chiamata la setenv per aggiornare BPID.
 * 
 * @return int Restituisce 1 se tutto va a buon fine, -1 altrimenti
 */
int updateBPID();

/**
 * @brief Funzione che controlla se un argomento che gettok ha
 * elaborato corrisponde a un comando personalizzato
 * 
 * @param str La stringa da controllare
 * @return int Restituisce 1 se la stringa in input corrisponde
 * a un comando personalizzato, 0 altrimenti.
 */
int checkIfCustom(char* str);

/**
 * @brief Esegue operazioni specifiche in base al nome del
 * comando personalizzato passato come argomento.
 * 
 * @param commandName La stringa che rappresenta il nome del comando
 */
void executeCustomCommand(char * commandName);

/**
 * @brief Funzione che viene chiamata quando l'interprete non sta
 * eseguendo un processo in foreground e viene terminato con un 
 * segnale di terminazione. QUello che fa è liberare la memoria
 * allocata per la stringa prompt e la stringa strbpid.
 * 
 * @param sig Il segnale ricevuto
 */
void deallocateOnSignal(int sig);









#include "smallsh.h"
#include <string.h>

/* Dichiarazione del buffer in cui verrà copiata la stringa digitata digitata
dall'utente. */
static char inpbuf[MAXBUF];   

/* Dichiarazione del buffer in cui verranno copiati i vari argomenti 
spezzati dal carattere di terminazione '\0' */
static char tokbuf[2*MAXBUF]; 

/* Dichiarazione dei puntatori per scorrere i buffers */
static char *ptr;   // Puntatore per scorrere inputbuf
static char *tok;   // Puntatore per scorrere tokenbuf

/* Array di caratteri che hanno una interpretazione "speciale" 
nei comandi della shell. */
static char special[]= {' ', '\t', ';', '\n', '\0', '&'};



int userin(char *p) {
  
    /* Dichiarazione di alcune variabili */
    int c;      // Variabile per leggere il singolo carattere della riga
    int count;  // Variabile per contare il numero di caratteri della riga

    /* Inizializzazione dei puntatori all'inizio dei rispettivi array di char */
    ptr = inpbuf;
    tok = tokbuf;

    /* Stampa il prompt */
    printf("%s ",p);

    /* Ora che sta per cominciare la lettura della riga, inizializzo count a 0.
    In particolare questa variabile verrà usata per controllare che non vengano 
    inseriti più di MAXBUF caratteri nella singola linea. */
    count = 0;

    /* Ciclo infinito che legge tutti i caratteri della riga fino a quando non 
    trova un EOF. */
    while(1) {

        /* Leggo carattere per carattere e se è un EOF lo ritorno  */
        if ((c = getchar()) == EOF) {
            if(DBG) printf("[USERIN]: EOF detected. Closing program...\n");
            return(EOF);
        }
        
        /* Se il carrattere letto non è un EOF, si copia il carattere letto 
        in inpbuf; questo vale se non si è ancora superato maxbuff. 
        Se si raggiunge e supera MAXBUF, non si scrive piu' in inpbuf, si 
        continua a leggere fino a newline (si veda sotto) */
        if (count < MAXBUF) inpbuf[count++] = c;

        /* Se si legge il newline (e non si è superato maxbuf), la riga in input 
        e' finita. Se è un new line si mette /0 per terminare la stringa e si 
        restituisce count che conta tutti i caratteri (/o compreso) */
        if (c == '\n' && count < MAXBUF) {
            inpbuf[count] = '\0';
            if(DBG) printInputbuf(inpbuf, count);
            if(DBG) printf("[USERIN]: Current line has %d character.\n", count);
            return(count);
        }

        /* Se e' stato superato MAXBUF (se si arriva qui è così), quando si arriva 
        al newline si avverte che la riga e' troppo lunga e si va a leggere 
        una nuova riga resettando count. */
        if (c == '\n') {
            fprintf(stderr, "Maximum command line character size exceeded. Try again.\n");
            count = 0;
            printf("%s ",p);
        }
    }

}



int gettok(char **outptr)	{
    
    /* Variabile per gestire il valore di ritorno. Indica il tipo di pezzo
    trovato.*/
    int type;

    /* si piazza *outptr in modo che punti al primo byte dove si cominicera'
    a scrivere il simbolo letto. In altre parole si copia l'indirizzo di 
    memoria di tok (e quiindi di tokenbuf) nella prima posizione dell'array 
    degli argomenti della exec.*/  
    *outptr = tok;


    /* Ciclo per ignorare eventuali spazi o tabulazioni contenuti inputbuf */
    if(DBG) printf("[GETTOK]: Start processing current symbol. ------------\n");
    while (*ptr == ' ' || *ptr == '\t') {
        if(DBG) printf("[GETTOK]: Found a space. Will be ignored!\n");
        if(DBG) printf("[GETTOK]: Increasing ptr.\n");
        ptr++;
        if(DBG) printf("[GETTOK]: Now ptr point to character '%c' of inputbuf.\n", *ptr);
    }

    /* Quando ho ignorato tutti gli spazi, ptr punta al primo carattere utile.
    Copio il primo carattere del simbolo in tok. */
    *tok++ = *ptr;
    if (DBG) printf("[GETTOK]: Found first useful char of current symbol.\n");
    if (DBG) printf("[GETTOK]: First useful char copied from ptr to tok (%c).\n", *ptr);

    /* A seconda del carattere decide il tipo di simbolo da restituire all procline */
    switch(*ptr++){

        case '\n':
            if (DBG) printf("[GETTOK]: Current value of Ptr (before increment) point to EOF.\n");
            type = EOL; 
        break;

        case ';':
            if (DBG) printf("[GETTOK]: Current value of Ptr (before increment) point to SEMICOLON.\n");
            type = SEMICOLON; 
        break;

        case '&':
            if (DBG) printf("[GETTOK]: Current value of Ptr (before increment) point to AMPERSAND.\n");
            type = AMPERSAND; 
        break;

        /* Se si entra in default, sarà sicuramente un argomento. In tal caso
        devo aggiungere in tokbuf (e quindi in arg) l'intero argomento e non
        un singolo carattere; quindi considerò parte dell'argomento tutti i
        caratteri fino a trovare un carattere speciale e li aggiungo in arg.*/
        default:
            if (DBG) printf("[GETTOK]: Current value of Ptr (before increment) point to ARGOMENT.\n");
            type = ARG;            
            if (DBG) printf("[GETTOK]: Found first char of an argument; now i'll try to read all character from same arg until getting a special character.\n");
            while(inarg(*ptr)) {
                if (DBG) printf("[GETTOK]: Copying current value of ptr ('%c') in tok.\n", *ptr);
                *tok++ = *ptr++;
                // *ccmd++ = *ptr++;
            }
        break;
    }

    /* aggiunge \0 al fondo */
    if(DBG) printf("[GETTOK]: Add null char to tok.\n");
    *tok++ = '\0';

    /* Controllo se argomento controllato corrisponde a comando personalizzato */
    if(checkIfCustom(tokbuf) == 1) {
        if(DBG) printf("[GETTOK]: Actual argument is a special custom commands!\n");
        type = CUSTOM;
    }

    /* Restituisco a procline il tipo di simbolo aggiunto in arg */ 
    if(DBG) printf("[GETTOK]: Returning to procline symbol %s.\n", convertTypeToString(type));
    return(type);

}



int checkIfCustom(char* str) {
    const char *customCommands[] = {"bp"};

    for (size_t i = 0; i < CUSTOM_COMMANDS_COUNT; i++){
        if(strcmp(str, customCommands[i]) == 0) return 1;
    }
    return 0;
}



int inarg(char c){
    char *wrk;
    for (wrk = special; *wrk != '\0'; wrk++)
        if (c == *wrk) return(0);
    return(1);
}



void printInputbuf(char* array, int dim) {
    for (int i = 0; i < dim; i++)
        printf("[USERIN]: Inputbuf[%d] = '%c'\n", i, array[i]);
}



char * convertTypeToString(int type) {
  switch (type)
  {
    case 1:
    return "EOF";
    break;

    case 2:
    return "ARG";
    break;

    case 3:
    return "AMPERSAND";
    break;

    case 4:
    return "SEMICOLON";
    break;

    case 5:
    return "CUSTOM";
    break;
  
  default:
  return NULL;
    break;
  }
}
