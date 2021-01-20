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
 * @brief Stampa in stdout la variabile d'ambiente BPID
 * 
 * @return int 1 se la stampa ha successo, -1 se si è verificato un errore.
 */
int printBPID();

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