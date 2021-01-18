#include "smallsh.h"

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
            }
        break;
    }

  /* aggiunge \0 al fondo e restiuisce a procline il tipo di simbolo aggiunto in arg*/
  if(DBG) printf("[GETTOK]: Add null char to tok.\n");
  *tok++ = '\0';
  if(DBG) printf("[GETTOK]: Returning to procline symbol %s.\n", convertTypeToString(type));
  return(type);

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
  
  default:
  return NULL;
    break;
  }
}
