/*
Server
*/
// include
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include "chat_message.h"


// prototype
int check(int exp, char *msgErr);
void createSocket(int *listener, struct sockaddr_in *svAddr);
void connectionAccept(fd_set *masterSet, int *fdMax, int listener, struct sockaddr_in *clAddr);
void helpMessage();
void startMessage();
void insertInList(char* username, int port, int fd);
void listDevice();
int isOnline(char* username);
int getDevicePort(char* username);
int getDeviceFd(char* username);
int deviceDisconnect(int fd);
int getOnlineNumber();
void sendAckRead(char* username, int i);

void signUpMessage(int i);
void inMessage(int i);

void textMessage(int i);

void onlineListMessage(int i);

void hanging(int i);
void showMessage(int i);
void requestPort(int i);

// global
device *deviceListHead = NULL;  //puntatore alla lista device
int serverPort;


// funzione per il controllo di errore
int check(int exp, char *msgErr){
    if(exp == -1){
        perror(msgErr);
        exit(1);
    }

    return exp;
}

// funzione per la creazione del socket di ascolto
void createSocket(int *listener, struct sockaddr_in *svAddr){
    // creazione socket ascolto
    check((*listener = socket(AF_INET,SOCK_STREAM,0)), "Socket");

    // creazione indirizzo server
    memset(svAddr,0,sizeof(*svAddr));
    svAddr->sin_family = AF_INET;
    svAddr->sin_port = htons(serverPort);
    // inet_pton(AF_INET, IND, &svAddr->sin_addr);
    svAddr->sin_addr.s_addr = INADDR_ANY;

    // bind
    check(bind(*listener,(struct sockaddr *) svAddr,sizeof(struct sockaddr)),"Bind");
    // listen
    check(listen(*listener, 10), "Listen");

    printf("\nServer waiting for client on port %d\n",serverPort);
	fflush(stdout);
}

// funzione per la creazione dei socket di comunicazione
void connectionAccept(fd_set *masterSet, int *fdMax, int listener, struct sockaddr_in *clAddr){
    
    int addrlen, newFd;

    printf("Nuovo client rilevato.\n");
    fflush(stdout);
    // creo il socket di comunicazione
    addrlen = sizeof(struct sockaddr_in);
    check((newFd = accept(listener,(struct sockaddr*) clAddr,(socklen_t*) &addrlen)),"accept");
    // aggiungo il descrittore al set dei socket monitorati
    FD_SET(newFd, masterSet);
    // Aggiorno il massimo descrittore
    if(newFd > *fdMax){*fdMax = newFd;}

    printf("new connection from %s on port %d \n",inet_ntoa(clAddr->sin_addr), ntohs(clAddr->sin_port));
}

// funzione per stampare le informazioni di help
void helpMessage(){
    puts("\n1) help --> mostra i dettagli dei comandi");
    puts("2) list --> mostra un elenco degli utenti connessi");
    puts("3) esc --> chiude il server\n");
    fflush(stdout);
}

// funzione per stampare il menu di avvio
void startMessage(){
    puts("\n***************************** SERVER STARTED *********************************");
    puts("Digita un comando:");
    helpMessage();
}

// funzione per inserire in coda alla lista
void insertInList(char* username, int port, int fd){
    
    device *newDevice = (device*)malloc(sizeof(device));
    device *last = deviceListHead;

    // creo il timestamp
    time_t curTime;
    char curTimeStamp[TIMESTAMP_LENGTH];
    time(&curTime);
    strcpy(curTimeStamp, ctime(&curTime));
    // metto il fine stringa
    if(curTimeStamp[strlen(curTimeStamp)-1] == '\n') curTimeStamp[strlen(curTimeStamp)-1] = '\0';

    // inserisco i dati del nuovo device
    strcpy(newDevice->username, username);
    newDevice->port = port;
    newDevice->fd = fd;
    strcpy(newDevice->timestampIN, curTimeStamp);
    newDevice->timestampOUT[0] = '\0';
    newDevice->next = NULL;

    // se lista vuota inserisco in testa
    if(deviceListHead == NULL){
        deviceListHead = newDevice;
        return;
    }
    // inserisco in coda
    while (last->next != NULL) last = last->next;
    
    last->next = newDevice;
    return;
}

// funzione per stampare gli utenti online
void listDevice(){

    device *curr = deviceListHead;
    bool found = false;
    
    while(curr != NULL){
        // se e' online stampo le info
        if(curr->timestampOUT[0] == '\0'){
            found = true;
            printf("%s*%s*%d \n", curr->username, curr->timestampIN, curr->port);
        }
        curr = curr->next;
    }
    // messaggio se non ci sono utenti online
    if(!found){
        puts("Non ci sono utenti online!");
        return;
    }

    return;
}

// funzione che indica se l'username e' online
int isOnline(char* username){

    device *curr = deviceListHead;
    int ret = -1;

    while(curr != NULL){
        // cerco l'username
        if(strcmp(curr->username, username) == 0){
            // cerco se e' online
            if(curr->timestampOUT[0] == '\0'){
                return 1;
            }
            ret = 0;
        }
        curr = curr->next;
    }
    return ret;
}

// funzione che restituisce la porta dell'username
int getDevicePort(char* username){
    device *curr = deviceListHead;

    while(curr != NULL){
        // trovo tutti quelli online
        if(curr->timestampOUT[0] == '\0'){
            // cerco l'username
            if(strcmp(curr->username, username) == 0){
                return curr->port;
            }
        }
        curr = curr->next;
    }
    return -1;
}

// funzione che restituisce il Fd dell'username
int getDeviceFd(char* username){
    device *curr = deviceListHead;

    while(curr != NULL){
        // trovo tutti quelli online
        if(curr->timestampOUT[0] == '\0'){
            // cerco l'username
            if(strcmp(curr->username, username) == 0){
                return curr->fd;
            }
        }
        curr = curr->next;
    }
    return -1;
}

// funzione per segnare la disconnessione di un device
int deviceDisconnect(int fd){
    device *curr = deviceListHead;

    // creo il timestamp
    time_t curTime;
    char curTimeStamp[TIMESTAMP_LENGTH];
    time(&curTime);
    strcpy(curTimeStamp, ctime(&curTime));
    // metto il fine stringa
    if(curTimeStamp[strlen(curTimeStamp)-1] == '\n') curTimeStamp[strlen(curTimeStamp)-1] = '\0';

    while(curr != NULL){
        // trovo tutti quelli online
        if(curr->timestampOUT[0] == '\0'){
            // cerco il file descriptor
            if(curr->fd == fd){
                // inserisco il timestamp corrente
                strcpy(curr->timestampOUT, curTimeStamp);
                return 1;
            }
        }
        curr = curr->next;
    }
    return 0;
}

// funzione che ritorna il numero di utenti online
int getOnlineNumber(){
    
    device *curr = deviceListHead;
    int n = 0;

    while(curr != NULL){
        // se e' online incremento
        if(curr->timestampOUT[0] == '\0'){
            n++;
        }
        curr = curr->next;
    }

    return n;
}

// funzione per inviare le notifiche di lettura pendenti
void sendAckRead(char* username, int i){

    // file dove sono salvate tutte le notifiche di lettura
    FILE* fAck;
    char path[USERNAME_LENGTH*2];
    // struttura per pacchetto da leggere da file e da inviare
    pk_ack_read pkRead;
    // buffer per inviare in modalita text
    char bufferSendACK[sizeof(pkRead)];

    // inizializzo il percorso del file
    strcpy(path, "Server/");
    strcat(path, username);
    strcat(path, "/ackRead.txt");

    // apro il file in lettura
    if( (fAck = fopen(path, "r")) != NULL){
        
        // leggo tutto il file e invio
        while(fread(&pkRead, sizeof(pkRead), 1, fAck)){
            
            // conversione a stringa del messaggio
            sprintf(bufferSendACK, "%d %s", pkRead.mt, pkRead.username);
            // invio risposta al client
            check( send(i, (void*) bufferSendACK, sizeof(bufferSendACK), 0) , "Errore invio Notifica" );
            printf("Ho inviato a %s-> %d %s\n", username, pkRead.mt, pkRead.username);

        }
        // chiudo e elimino il file
        fclose(fAck);
        remove(path);
    }
}



int main(int argc, char const *argv[])
{
    // set descrittori da monitorare
    fd_set masterSet;
    // set descrittori pronti
    fd_set readSet;
    int fdMax, i;
    int listener;
    struct sockaddr_in svAddr, clAddr;
    

    // azzeramento set
    FD_ZERO(&masterSet);
    FD_ZERO(&readSet);

    // controllo presenza porta
    if(argc == 2){
        // conversione porta
        serverPort = atoi(argv[1]);
    }else{
        serverPort = SERVER_PORT;
    }
    
    // creazione socket
    createSocket(&listener, &svAddr);


    // messaggio di avvio server
    startMessage();
    // creazione cartella server se non esiste gia'
    if(mkdir("Server", 0777) == -1){
        perror("Errore Creazione cartella");
    }else{
        puts("cartella creata correttamente");
    }



    // aggiunta listener al set masterSet
    FD_SET(listener, &masterSet);
    // aggiunta dello stdin al set masterSet
    FD_SET(STDIN_FILENO, &masterSet);
    // aggiorno il massimo
    fdMax = listener;
    // main loop
    while(1){
        // inizializzo il set manipolato dalla select
        readSet = masterSet;
        // select in lettura senza timeout 
        check((select(fdMax+1,&readSet,NULL,NULL,NULL)),"Select");
        // scorro i descrittori pronti in lettura
        
        for ( i = 0; i <= fdMax; i++)
        {
            // se "i" pronto
            if(FD_ISSET(i,&readSet)){

                
                if(i == listener){  // se "i" listener
            
                    //richiesta di connessione, creare un nuovo socket di comunicazione
                    connectionAccept(&masterSet,&fdMax,listener,&clAddr);
                
                }else if(i == STDIN_FILENO){    // se "i" stdin

                    char comando[10];
                    
                    scanf("%9s", comando);

                    if(strcmp(comando, "help") == 0){
                        
                        // comando help
                        helpMessage();
                        
                    }else if (strcmp(comando, "list") == 0){
                        
                        // comando list
                        listDevice();

                    }else if (strcmp(comando, "esc") == 0){
                        
                        // comando esc
                        puts("Disconnessione server!");
                        exit(1);

                    }else{

                        puts("Comando inesistente");
                        
                    }
                    
                }else{  // ricezione messaggio dai client

                    pk_type pkType;
                    // buffer per memorizzare il tipo di messaggio ricevuto
                    char buff[4];
                    int nBytesRecvd;
                    
                    
                    // recv con MSG_PEEK non rimuove il dato dalla coda, prossima recv legge gli stessi dati
                    if((nBytesRecvd = recv(i, (void*) buff, sizeof(pkType), MSG_PEEK)) <= 0){
                        // se ricevo 0 e' una disconnessione
                        if(nBytesRecvd == 0){
                            printf("socket %d hung up\n", i);
                            deviceDisconnect(i);
                        
                        }else{  // -1 e' un errore
                            perror("Recv Error");
                        }
                        // tolgo il dato ricevuto dalla coda
                        recv(i, (void*) buff, sizeof(pkType), 0);
                        // chiudo il socket e rimuovo dal set
                        close(i);
                        FD_CLR(i, &masterSet);
                        // passo all'iterazione successiva
                        continue;
                    }

                    // parsing e memorizzazione del tipo di messaggio ricevuto
                    sscanf(buff,"%d" ,&pkType.mt);

                    switch (pkType.mt){
                    case MSG_SIGNUP:{
                        signUpMessage(i);
                    }break;

                    case MSG_IN:{
                        inMessage(i);
                    }break;
                    
                    case MSG_TEXT:{
                        textMessage(i);
                    }break;

                    case MSG_ONLINELIST:{
                        onlineListMessage(i);
                    }break;

                    case MSG_HANGING:{
                        hanging(i);
                    }break;

                    case MSG_SHOW:{
                        showMessage(i);
                    }break;

                    case MSG_PORT_REQ:{
                        requestPort(i);
                    }break;

                    default:{
                        puts("Errore di invio pacchetto!");

                    }break;
                    }
                }
            }
        }
        
    }

    return 0;
}


// funzione per la ricezione dei messaggi di signup
void signUpMessage(int i){

    // struttura per signup
    pk_signup pkSignup;
    // buffer per memorizzare il messaggio di signup
    char bufferRecv[sizeof(pkSignup)];
    // puntatore al file di signup
    FILE* fSignUp;
    char username[USERNAME_LENGTH], password[USERNAME_LENGTH];
    // pacchetto da inviare
    pk_info pkInfoResponse;
    // buffer per inviare in modalita text
    char bufferSend[sizeof(pkInfoResponse)];

    
    puts("Pacchetto signup ricevuto");
    check( recv(i, (void*) bufferRecv, sizeof(bufferRecv), 0), "Recv Error MSG_SIGNUP" );
    // parsing e memorizzazione del messaggio di signup
    sscanf(bufferRecv, "%d %s %s", &pkSignup.mt, pkSignup.username, pkSignup.password);
    printf("Ricevuto -> %d %s %s \n",  pkSignup.mt, pkSignup.username, pkSignup.password);
    
    // preparo il pacchetto di risposta
    pkInfoResponse.mt = MSG_OK;
    strcpy(pkInfoResponse.info, "Acconunt registered successfully!");
    
    // apro il file di signup in modalita lettura e append
    if((fSignUp = fopen("Server/SignUp.txt", "a+")) == NULL){
        printf("Error opening SignUp file!");
        exit(1);
    }
    // leggo il conteuto 
    while( fscanf(fSignUp, "%s %s", username, password) != EOF ){
        // se account gia presente modifico il messaggio di risposta
        if(strcmp(username, pkSignup.username) == 0){
            pkInfoResponse.mt = MSG_ERR;
            strcpy(pkInfoResponse.info, "Username already used!");
            if(strcmp(password,pkSignup.password) == 0){
                strcpy(pkInfoResponse.info, "Account already registered!");
            }
            break;
        }
    }
    // se account non presente lo inserisco nel file
    if(pkInfoResponse.mt != MSG_ERR){
        fprintf(fSignUp, "%s %s\n", pkSignup.username, pkSignup.password);
    }
    // chiusura file
    fclose(fSignUp);

    printf("response -> %d %s\n", pkInfoResponse.mt, pkInfoResponse.info);
    // conversione a stringa del messaggio
    sprintf(bufferSend, "%d %s", pkInfoResponse.mt, pkInfoResponse.info);
    // invio risposta al client
    check( send(i, (void*) bufferSend, sizeof(bufferSend), 0) , "Errore invio risposta SignUp" );

}

// funzione per la ricezione dei messaggi di in
void inMessage(int i){
    
    // struttura per in
    pk_in pkIn;
    // buffer per memorizzare il messaggio di in
    char bufferRecv[sizeof(pkIn)];
    // puntatore al file di signup
    FILE* fSignUp;
    char username[USERNAME_LENGTH], password[USERNAME_LENGTH];
    // pacchetto da inviare
    pk_info pkInfoResponse;
    // buffer per inviare in modalita text
    char bufferSend[sizeof(pkInfoResponse)];

    puts("Pacchetto In ricevuto");
    check( recv(i, (void*) bufferRecv, sizeof(bufferRecv), 0), "Recv Error MSG_IN" );
    // parsing e memorizzazione del messaggio di in
    sscanf(bufferRecv, "%d %s %s %d", &pkIn.mt, pkIn.username, pkIn.password, &pkIn.port);
    printf("Ricevuto -> %d %s %s %d \n",  pkIn.mt, pkIn.username, pkIn.password, pkIn.port);

    // preparo il pacchetto di risposta
    pkInfoResponse.mt = MSG_ERR;
    strcpy(pkInfoResponse.info, "Account does not exist!");
    
    // apro il file di signup in modalita lettura
    if((fSignUp = fopen("Server/SignUp.txt", "r")) != NULL){
       // leggo il conteuto 
        while( fscanf(fSignUp, "%s %s", username, password) != EOF ){
            // se account gia presente modifico il messaggio di risposta
            if(strcmp(username, pkIn.username) == 0){
                strcpy(pkInfoResponse.info, "Incorrect password!");
                if(strcmp(password,pkIn.password) == 0){
                    pkInfoResponse.mt = MSG_OK;
                    strcpy(pkInfoResponse.info, "Logged in!");
                }
                break;
            }
        }
        // chiusura file
        fclose(fSignUp); 
    }
    
    // se account presente lo inserisco nella lista
    if(pkInfoResponse.mt == MSG_OK){
        insertInList(pkIn.username, pkIn.port, i);
    }

    printf("response -> %d %s\n", pkInfoResponse.mt, pkInfoResponse.info);
    // conversione a stringa del messaggio
    sprintf(bufferSend, "%d %s", pkInfoResponse.mt, pkInfoResponse.info);
    // invio risposta al client
    check( send(i, (void*) bufferSend, sizeof(bufferSend), 0) , "Errore invio risposta In" );

    // invio le notifiche di lettura che ho salvato mentre username era offline
    if(pkInfoResponse.mt == MSG_OK){
        sendAckRead(pkIn.username, i);
    }

}

// funzione per la ricezione dei messaggi di text
void textMessage(int i){

    int ret;
    // struttura per text
    pk_text pkText;
    // buffer per memorizzare il messaggio di text
    char bufferRecv[sizeof(pkText)];
    // pacchetto da inviare
    pk_server_ack pkAckResponse;
    // buffer per inviare in modalita text
    char bufferSend[sizeof(pkAckResponse)];
    


    puts("Pacchetto text ricevuto");

    check( recv(i, (void*) bufferRecv, sizeof(bufferRecv), 0), "Recv Error MSG_TEXT" );
    // parsing e memorizzazione del messaggio text
    sscanf(bufferRecv, "%d %s %d %s %d %[^\n]", &pkText.mt, pkText.userSource, &pkText.portSource, pkText.userDest, &pkText.portDest, pkText.text);
    printf("ho ricevuto ---> %d %s %d %s %d %s \n", pkText.mt, pkText.userSource, pkText.portSource, pkText.userDest, pkText.portDest, pkText.text);



    ret = isOnline(pkText.userDest);

    if(ret == 1){   // il destinatario e' online
        
        int portDest = getDevicePort(pkText.userDest);
        int fdDest = getDeviceFd(pkText.userDest);
        // invio il messaggio al dest
        check( send(fdDest, (void*) bufferRecv, sizeof(bufferRecv), 0) , "Errore inoltro messaggio text al destinatario" );
        // ack send
        pkAckResponse.mt = MSG_ACK_SEND;
        pkAckResponse.portDest = portDest;

    }else if(ret == -1){ // il destinatario non e' nella lista
        
        FILE* fSignUp;
        char username[USERNAME_LENGTH], password[USERNAME_LENGTH];
        // apro il file di signup in modalita lettura
        if((fSignUp = fopen("Server/SignUp.txt", "r")) != NULL){
        // leggo il conteuto 
            while( fscanf(fSignUp, "%s %s", username, password) != EOF ){
                // se account gia presente allora e' offline
                if(strcmp(username, pkText.userDest) == 0){
                    ret = 0;
                    break;
                }
            }
            // chiusura file
            fclose(fSignUp); 
        }

        if(ret == -1){
            // l'username non esiste
            pkAckResponse.mt = MSG_ERR;
            pkAckResponse.portDest = 0;
        }

    }

    if(ret == 0){   // il destinatario e' offline
        // info per il salvataggio del messaggio
        hangingMessage newMessage;
        char path[USERNAME_LENGTH*2];
        FILE* fp;
        
        // creo il timestamp
        time_t curTime;
        char curTimeStamp[TIMESTAMP_LENGTH];
        time(&curTime);
        strcpy(curTimeStamp, ctime(&curTime));
        // metto il fine stringa
        if(curTimeStamp[strlen(curTimeStamp)-1] == '\n') curTimeStamp[strlen(curTimeStamp)-1] = '\0';

        // creo la cartella se non c'e' gia
        strcpy(path, "Server/");
        strcat(path, pkText.userDest);
        mkdir(path, 0777);
        // percorso del file di hanging
        strcat(path, "/hanging.txt");

        // inserisco i dati nella struttura
        strcpy(newMessage.userSource, pkText.userSource);
        strcpy(newMessage.timestamp, curTimeStamp);
        strcpy(newMessage.text, pkText.text);

        // apro il file in modalita append
        if((fp = fopen(path, "a")) == NULL){
            printf("Error opening file!");
            exit(1);
        }

        fwrite(&newMessage, sizeof(newMessage), 1, fp);
        fclose(fp);


        // ack store
        pkAckResponse.mt = MSG_ACK_STORE;
        pkAckResponse.portDest = 0;
    }

    printf("Response to source -> %d %d\n", pkAckResponse.mt, pkAckResponse.portDest);
    // conversione a stringa del messaggio
    sprintf(bufferSend, "%d %d", pkAckResponse.mt, pkAckResponse.portDest);
    // invio risposta ack al source
    check( send(i, (void*) bufferSend, sizeof(bufferSend), 0) , "Errore invio risposta Ack" );
}

// funzione per inviare la lista degli utenti online
void onlineListMessage(int i){

    int n;
    device *curr = deviceListHead;
    // struttura per list
    pk_onlinelist pkOnlineRecive;
    // buffer per memorizzare il messaggio di list
    char bufferRecv[sizeof(pkOnlineRecive)];
    // pacchetto da inviare
    pk_onlinelist_number pkNumberResponse;
    // buffer per inviare in modalita text
    char buffer[sizeof(pkNumberResponse)];
    // pacchetto da inviare
    pk_onlinelist pkOnlineSend;
    // buffer per inviare in modalita text
    char bufferSend[sizeof(pkOnlineSend)];

    puts("Pacchetto List ricevuto");

    check( recv(i, (void*) bufferRecv, sizeof(bufferRecv), 0), "Recv Error MSG_ONLINELIST" );
    // parsing e memorizzazione del messaggio list
    sscanf(bufferRecv, "%d %s", &pkOnlineRecive.mt, pkOnlineRecive.username);
    printf("ho ricevuto ---> %d %s \n", pkOnlineRecive.mt, pkOnlineRecive.username);

    n = getOnlineNumber();  // per sapere il numero di utenti online
    n--;    // tolgo il device che ha fatto la richiesta

    pkNumberResponse.mt = MSG_ONLINELIST_N;
    pkNumberResponse.number = n;

    printf("Response to source -> %d %d\n", pkNumberResponse.mt, pkNumberResponse.number);
    // conversione a stringa del messaggio
    sprintf(buffer, "%d %d", pkNumberResponse.mt, pkNumberResponse.number);
    // invio risposta number al source
    check( send(i, (void*) buffer, sizeof(buffer), 0) , "Errore invio risposta Number" );

    
    while(curr != NULL){
        // guardo gli utenti online
        if(curr->timestampOUT[0] == '\0'){
            // se non e' quello che ha fatto richiesta invio il messaggio
            if(strcmp(curr->username, pkOnlineRecive.username) != 0){

                pkOnlineSend.mt = MSG_ONLINELIST;
                strcpy(pkOnlineSend.username, curr->username);

                printf("Response to source -> %d %s\n", pkOnlineSend.mt, pkOnlineSend.username);
                // conversione a stringa del messaggio
                sprintf(bufferSend, "%d %s", pkOnlineSend.mt, pkOnlineSend.username);
                // invio risposta al source
                check( send(i, (void*) bufferSend, sizeof(bufferSend), 0) , "Errore invio risposta" );

            }
        }
        curr = curr->next;
    }
}


// funzione per inviare la lista di hanging
void hanging(int i){

    // struttura per ricevere il messaggio
    pk_info pkInfoRecive;
    // buffer per memorizzare il messaggio
    char bufferRecv[sizeof(pkInfoRecive)];
    // file di signup
    FILE* fSignUp;
    char username[USERNAME_LENGTH], password[USERNAME_LENGTH];
    // file di hanging
    FILE* fHanging;
    char path[USERNAME_LENGTH*2];
    // pacchetto da inviare
    pk_hanging pkHanging;
    // buffer per inviare in modalita text
    char bufferSend[sizeof(pkHanging)];


    puts("pacchetto hanging ricevuto");

    check( recv(i, (void*) bufferRecv, sizeof(bufferRecv), 0), "Recv Error MSG_HANGING" );
    // parsing e memorizzazione del messaggio
    sscanf(bufferRecv, "%d %s", &pkInfoRecive.mt, pkInfoRecive.info);
    printf("Ricevuto -> %d %s \n", pkInfoRecive.mt, pkInfoRecive.info);

    // inizializzo il percorso del file di hanging
    strcpy(path, "Server/");
    strcat(path, pkInfoRecive.info);
    strcat(path, "/hanging.txt");

    // apro il file di hanging in lettura
    if( (fHanging = fopen(path, "r")) != NULL){
        
        // apro il file di signup in lettura
        if((fSignUp = fopen("Server/SignUp.txt", "r")) != NULL){
            // leggo il conteuto 
            while( fscanf(fSignUp, "%s %s", username, password) != EOF ){

                hangingMessage hm;
                int number = 0;
                char lastTimestamp[TIMESTAMP_LENGTH];


                // leggo tutto il contenuto di hanging.txt
                while(fread(&hm, sizeof(hm), 1, fHanging)){
                    // cerco l'username prelevato dal file signup.txt
                    if(strcmp(username, hm.userSource) == 0){
                        number++;
                        strcpy(lastTimestamp, hm.timestamp);
                    }
                }

                if(number > 0){
                    printf("Invio al device: %s %d %s \n", username, number, lastTimestamp);
                    // preparo il pacchetto di risposta
                    pkHanging.mt = MSG_HANGING_RESPONSE;
                    strcpy(pkHanging.username, username);
                    pkHanging.number = number;
                    strcpy(pkHanging.timestamp, lastTimestamp);
                    // conversione a stringa del messaggio
                    sprintf(bufferSend, "%d %s %d %s", pkHanging.mt, pkHanging.username, pkHanging.number, pkHanging.timestamp);
                    // invio risposta al client
                    check( send(i, (void*) bufferSend, sizeof(bufferSend), 0) , "Errore invio risposta hanging" );
                }
                
                // devo rileggere il file di hanging dall'inizio
                rewind(fHanging);

            }
            // chiusura file signup
            fclose(fSignUp); 
        }
        // chiusura file hanging
        fclose(fHanging);
    }

    // invio al device il messaggio di fine comando hanging
    // preparo il pacchetto di risposta
    pkHanging.mt = MSG_END;
    strcpy(pkHanging.username, "0");
    pkHanging.number = 0;
    strcpy(pkHanging.timestamp, "0");
    // conversione a stringa del messaggio
    sprintf(bufferSend, "%d %s %d %s", pkHanging.mt, pkHanging.username, pkHanging.number, pkHanging.timestamp);
    // invio risposta al client
    check( send(i, (void*) bufferSend, sizeof(bufferSend), 0) , "Errore invio risposta hanging" );
    

}


// funzione per inviare i messaggi pendenti
void showMessage(int i){

    bool inviato = false;
    // struttura per ricevere il messaggio
    pk_show pkShowRecive;
    // buffer per memorizzare il messaggio
    char bufferRecv[sizeof(pkShowRecive)];
    // file di hanging
    FILE* fHanging;
    char path[USERNAME_LENGTH*2];
    // file temporaneo
    FILE* fTemp;
    char tempPath[] = "Server/temp.txt";
    // pacchetto da inviare
    pk_show_response pkShowResponse;
    // buffer per inviare in modalita text
    char bufferSend[sizeof(pkShowResponse)];


    puts("pacchetto show ricevuto");

    check( recv(i, (void*) bufferRecv, sizeof(bufferRecv), 0), "Recv Error MSG_SHOW" );
    // parsing e memorizzazione del messaggio di in
    sscanf(bufferRecv, "%d %s %s", &pkShowRecive.mt, pkShowRecive.userRequest, pkShowRecive.userHanging);
    printf("Ricevuto -> %d %s %s \n", pkShowRecive.mt, pkShowRecive.userRequest, pkShowRecive.userHanging);

    // inizializzo il percorso del file di hanging
    strcpy(path, "Server/");
    strcat(path, pkShowRecive.userRequest);
    strcat(path, "/hanging.txt");

    

    // apro il file di hanging in lettura
    if( (fHanging = fopen(path, "r")) != NULL){

        hangingMessage hm;

        // apro il file temporaneo in scrittura
        if((fTemp = fopen(tempPath, "w")) == NULL){
            printf("Error opening file temp!");
            exit(1);
        }

        // leggo tutto il contenuto di hanging.txt
        while(fread(&hm, sizeof(hm), 1, fHanging)){
            // cerco i messaggi di userHanging
            if(strcmp(pkShowRecive.userHanging, hm.userSource) == 0){
                
                inviato = true;

                // invio il messaggio al device
                printf("Invio al device -> %s\n", hm.text);
                // preparo il pacchetto di risposta
                pkShowResponse.mt =  MSG_SHOW_RESPONSE;
                strcpy(pkShowResponse.text, hm.text);
                // conversione a stringa del messaggio
                sprintf(bufferSend, "%d %s", pkShowResponse.mt, pkShowResponse.text);
                // invio risposta al client
                check( send(i, (void*) bufferSend, sizeof(bufferSend), 0) , "Errore invio risposta Show" );

            }else{
                // salvo il messaggio in temp.txt cosi da eliminare i messaggi inviati
                fwrite(&hm, sizeof(hm), 1, fTemp);
            }
        }

        // chiudo i file
        fclose(fHanging);
        fclose(fTemp);
        // rimuovo il vecchio file di hanging
        remove(path);
        // rinomino il file temporaneo
        rename(tempPath, path);
    }

    // invio al device il messaggio di fine comando show
    // preparo il pacchetto di risposta
    pkShowResponse.mt =  MSG_END;
    strcpy(pkShowResponse.text, "0");
    // conversione a stringa del messaggio
    sprintf(bufferSend, "%d %s", pkShowResponse.mt, pkShowResponse.text);
    // invio risposta al client
    check( send(i, (void*) bufferSend, sizeof(bufferSend), 0) , "Errore invio risposta Show" );




    // se ho inviato i messaggi allora devo inviare la notifica al device sorgente dei messaggi
    if(inviato){

        // pacchetto di notifica da inviare
        pk_ack_read pkRead;
        // buffer per inviare in modalita text
        char bufferSendACK[sizeof(pkRead)];

        // inizializzo il messaggio
        pkRead.mt = MSG_ACK_READ;
        strcpy(pkRead.username, pkShowRecive.userRequest);

        if(isOnline(pkShowRecive.userHanging) == 1){
            // se online invio direttamente il messaggio
            int fdDest = getDeviceFd(pkShowRecive.userHanging);

            // conversione a stringa del messaggio
            sprintf(bufferSendACK, "%d %s", pkRead.mt, pkRead.username);
            // invio risposta al client
            check( send(fdDest, (void*) bufferSendACK, sizeof(bufferSendACK), 0) , "Errore invio Notifica" );
            printf("Ho inviato a %s-> %d %s\n", pkShowRecive.userHanging, pkRead.mt, pkRead.username);

        }else{
            // username offline salvo il messaggio di notifica
            // file dove salvare tutte le notifiche di lettura
            FILE* fAck;
            char pathAck[USERNAME_LENGTH*2];

            // creo la cartella se non c'e' gia
            strcpy(pathAck, "Server/");
            strcat(pathAck, pkShowRecive.userHanging);
            mkdir(pathAck, 0777);
            // percorso del file di hanging
            strcat(pathAck, "/ackRead.txt");

            // apro il file in modalita append
            if((fAck = fopen(pathAck, "a")) == NULL){
                printf("Error opening file!");
                exit(1);
            }

            fwrite(&pkRead, sizeof(pkRead), 1, fAck);
            fclose(fAck);

            puts("Salvo la notifica di lettura");
        }

    }

} 

// funzione per inviare la porta di un device
void requestPort(int i){

    // pacchetto da ricevere
    pk_port_req pkPort;
    // buffer per memorizzare il messaggio
    char bufferRecv[sizeof(pkPort)];
    // pacchetto da inviare
    pk_server_ack pkAckResponse;
    // buffer per inviare in modalita text
    char bufferSend[sizeof(pkAckResponse)];
    int ret;

    puts("Pacchetto di richiesta porta ricevuto");

    check( recv(i, (void*) bufferRecv, sizeof(bufferRecv), 0), "Recv Error MSG_PORT_REQ" );
    // parsing e memorizzazione del messaggio
    sscanf(bufferRecv, "%d %s", &pkPort.mt, pkPort.username);
    printf("ho ricevuto ---> %d %s \n", pkPort.mt, pkPort.username);

    ret = isOnline(pkPort.username);

    if(ret == 1){   // il destinatario e' online
        pkAckResponse.mt = MSG_OK;
        pkAckResponse.portDest = getDevicePort(pkPort.username);
    }else{ // destinatario offline
        pkAckResponse.mt = MSG_ERR;
        pkAckResponse.portDest = 0;
    }

    printf("Response to source -> %d %d\n", pkAckResponse.mt, pkAckResponse.portDest);
    // conversione a stringa del messaggio
    sprintf(bufferSend, "%d %d", pkAckResponse.mt, pkAckResponse.portDest);
    // invio risposta ack al source
    check( send(i, (void*) bufferSend, sizeof(bufferSend), 0) , "Errore invio risposta Ack" );

}


