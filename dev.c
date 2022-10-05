/*
Client
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
bool checkServer(int exp, char* msgErr);
bool connectServerRequest(int *serverFd, struct sockaddr_in *svAddr);
void listeningSocket(int *listener, struct sockaddr_in *myAddr);
void connectionAccept(fd_set *masterSet, int *fdMax, int listener);
int connectRequest(fd_set *masterSet, int *fdMax, int port);

void insertInList(char* username, int port, int fd);
void delateDevice(int fd);
int insertFd(char* username, int fd);
int insertPort(char* username, int port);
int findUser(char *username);
void getUsername(char* username, int fd);

void attivaChat(char* username);
void disattivaChat();
bool isActive(int fd);
bool isUsernameActive(char* username);

void signup(char* username, char* password, int portaServer, int *serverFd, struct sockaddr_in *svAddr, fd_set *masterSet, int *fdMax);
void login(int *serverFd, struct sockaddr_in *svAddr, fd_set *masterSet, int *fdMax);
bool in(char* username, char* password, int portaServer, int *serverFd, struct sockaddr_in *svAddr, fd_set *masterSet, int *fdMax);
void menuMessage();
void rubrica();

void reciveTextFromServer(int i);
void reciveAckRead(int i);
void reciveText(int i);
void reciveFile(int i);

void startChat(char* username);
void sendMessage(char* message, int serverFd, fd_set *masterSet, int *fdMax);
void sendMessageToServer(int serverFd, char* message, char* username);
void sendMessageToDevice(int Fd, int port, char* username ,char* message);

void getOnlineList(int serverFd);

void saveMessage(char* username, bool inviato, bool recapitato, char* message);

void hanging(int serverFd);
void showMessage(int serverFd, char* username);

void shareCommand(char* fileName, int serverFd, fd_set *masterSet, int *fdMax);
int requestPort(int serverFd, char* username);
void sendFile(int fd, char* fileName);

// global
bool logged = false;
int myPort;
int serverPort;
char myUsername[USERNAME_LENGTH];
deviceConnected *deviceConnectedList = NULL;    //puntatore alla lista dei device connessi
int isChatting = 0;
fd_set masterSet;
int server_Fd = -1;


// funzione per il controllo di errore
int check(int exp, char *msgErr){
    if(exp == -1){
        perror(msgErr);
        exit(1);
    }

    return exp;
}

// funzione per il controllo di disconnessione server
bool checkServer(int exp, char* msgErr){
    if(exp <= 0){
        // se ricevo 0 e' la disconnessione del server
        if(exp == 0){
            printf("Server offline!\n");
        
        }else{  // -1 e' un errore
            perror(msgErr);
        }
        // chiudo il socket e rimuovo dal set
        close(server_Fd);
        FD_CLR(server_Fd, &masterSet);
        server_Fd = -1;
        return true; 
    }
    return false;
}

// funzione per la creazione del socket connesso al server
bool connectServerRequest(int *serverFd, struct sockaddr_in *svAddr){

    // creazione socket
    check((*serverFd = socket(AF_INET, SOCK_STREAM,0)), "Socket");

    // creazione indirizzo server
    memset(svAddr, 0, sizeof(*svAddr));
    svAddr->sin_family = AF_INET;
    svAddr->sin_port = htons(serverPort);
    inet_pton(AF_INET , IND, &svAddr->sin_addr);

    // connessione al server
    if(connect(*serverFd, (struct sockaddr *) svAddr, sizeof(*svAddr)) == -1){
        perror("Server offline");
        return false;
    }

    return true;

}

// funzione per la creazione del socket di ascolto
void listeningSocket(int *listener, struct sockaddr_in *myAddr){
    
    // creazione socket ascolto
    check((*listener = socket(AF_INET,SOCK_STREAM,0)), "Socket");

    // creazione indirizzo device
    memset(myAddr,0,sizeof(*myAddr));
    myAddr->sin_family = AF_INET;
    myAddr->sin_port = htons(myPort);
    // inet_pton(AF_INET, IND, &myAddr->sin_addr);
    myAddr->sin_addr.s_addr = INADDR_ANY;

    // bind
    check(bind(*listener,(struct sockaddr *) myAddr,sizeof(struct sockaddr)),"Bind");
    // listen
    check(listen(*listener, 10), "Listen");

    printf("\nListening on port %d\n",myPort);
	fflush(stdout);
}

// funzione per la creazione dei socket di comunicazione
void connectionAccept(fd_set *masterSet, int *fdMax, int listener){
    
    int addrlen, newFd;
    struct sockaddr_in clAddr;

    printf("Nuovo device rilevato.\n");
    fflush(stdout);
    // creo il socket di comunicazione
    addrlen = sizeof(struct sockaddr_in);
    check((newFd = accept(listener,(struct sockaddr*) &clAddr,(socklen_t*) &addrlen)),"accept");
    // aggiungo il descrittore al set dei socket monitorati
    FD_SET(newFd, masterSet);
    // Aggiorno il massimo descrittore
    if(newFd > *fdMax){*fdMax = newFd;}

    printf("new connection from %s on port %d , fd is %d\n",inet_ntoa(clAddr.sin_addr), ntohs(clAddr.sin_port), newFd);
}

// funzione per connettersi ad altri device
int connectRequest(fd_set *masterSet, int *fdMax, int port){
    
    int newFd;
    struct sockaddr_in destAddr;
    // creazione socket
    check((newFd = socket(AF_INET, SOCK_STREAM,0)), "Socket");

    // indirizzo e porta device
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    inet_pton(AF_INET , IND, &destAddr.sin_addr);

    // connessione al device
    if(connect(newFd, (struct sockaddr *) &destAddr, sizeof(destAddr)) == -1){
        perror("Connect to device");
        return 0;
    }

    // aggiungo il descrittore al set dei socket monitorati
    FD_SET(newFd, masterSet);
    // Aggiorno il massimo descrittore
    if(newFd > *fdMax){*fdMax = newFd;}

    return newFd;

}

// funzione per inserire in coda alla lista
void insertInList(char* username, int port, int fd){

    deviceConnected *newDevice = (deviceConnected*)malloc(sizeof(deviceConnected));
    deviceConnected *last = deviceConnectedList;

    // inserisco i dati del nuovo device
    strcpy(newDevice->username, username);
    newDevice->port = port;
    newDevice->fd = fd;
    newDevice->activeChat = false;
    newDevice->next = NULL;

    // se lista vuota inserisco in testa
    if(deviceConnectedList == NULL){
        deviceConnectedList = newDevice;
        return;
    }
    // inserisco in in coda
    while (last->next != NULL) last = last->next;
    
    last->next = newDevice;
    return;
}

// funzione per eliminare un device disconnesso
void delateDevice(int fd){
    
    deviceConnected *temp = deviceConnectedList;
    deviceConnected *prev;

    // e' il primo elemento
    if(temp != NULL && temp->fd == fd){
        deviceConnectedList = temp->next;
        free(temp);
        return;
    }
    // cerco il device
    while( temp != NULL && temp->fd != fd ){
        prev = temp;
        temp = temp->next;
    }
    // se non e' presente
    if( temp == NULL ){
        return;
    }
    // elimino
    prev->next = temp->next;
    free(temp);
}

// funzione per aggiornare il fd dell user nella lista
int insertFd(char* username, int fd){
    deviceConnected *curr = deviceConnectedList;

    while(curr != NULL){
        // cerco l'username
        if(strcmp(curr->username, username) == 0){
            curr->fd = fd;
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

// funzione per aggiornare la porta dell user nella lista
int insertPort(char* username, int port){
    deviceConnected *curr = deviceConnectedList;

    while(curr != NULL){
        // cerco l'username
        if(strcmp(curr->username, username) == 0){
            curr->port = port;
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

// funzione per cercare username nella lista
int findUser(char *username){
    deviceConnected *curr = deviceConnectedList;

    while(curr != NULL){
        // cerco l'username
        if(strcmp(curr->username, username) == 0){
            if(curr->port != 0 && curr->fd != 0){
                return 2;
            }else if(curr->port != 0){
                return 1;
            }
            return 0;
        }
        curr = curr->next;
    }
    return -1;
}

// funzione per avere l'username dato il fd associato
void getUsername(char* username, int fd){
    deviceConnected *curr = deviceConnectedList;

    while(curr != NULL){
        // cerco il fd
        if(curr->fd == fd){
            strcpy(username, curr->username);
            return;
        }
        curr = curr->next;
    }
    return;
}

// funzione per attivare una chat
void attivaChat(char* username){
    deviceConnected *curr = deviceConnectedList;

    while(curr != NULL){
        // cerco l'username
        if(strcmp(curr->username, username) == 0){
            curr->activeChat = true;
            return;
        }
        curr = curr->next;
    }
    return;
}

// funzione per disattivare tutte le chat
void disattivaChat(){
    deviceConnected *curr = deviceConnectedList;

    isChatting = 0;
    
    while(curr != NULL){
        // cerco tutte le chat attive e disattivo
        if(curr->activeChat){
            curr->activeChat = false;
        }
        curr = curr->next;
    }
    return;
}

// funzione che cerca se il fd appartiene a una chat attiva
bool isActive(int fd){
    deviceConnected *curr = deviceConnectedList;

    while(curr != NULL){
        // cerco le chat attive
        if(curr->activeChat){
            // cerco fd
            if(curr->fd == fd){
                return true;
            }
        }
        curr = curr->next;
    }
    return false;
}

// funzione che cerca se l'username ha chat attiva
bool isUsernameActive(char* username){
    deviceConnected *curr = deviceConnectedList;

    while(curr != NULL){
        // cerco le chat attive
        if(curr->activeChat){
            // cerco username
            if(strcmp(curr->username, username) == 0){
                return true;
            }
        }
        curr = curr->next;
    }
    return false;
}






int main(int argc, char const *argv[])
{
    int fdMax, i, listener;
    struct sockaddr_in svAddr, myAddr;
    fd_set readSet;

    // azzeramento set
    FD_ZERO(&masterSet);
    FD_ZERO(&readSet);

    // controllo presenza porta
    if(argc != 2){
        puts("Usage: ./dev port");
        exit(1);
    }
    // conversione porta
    myPort = atoi(argv[1]);
    // creazione socket di ascolto
    listeningSocket(&listener, &myAddr);
    
    // aggiunta listener al set
    FD_SET(listener, &masterSet);
    // aggiunta stdin al set
    FD_SET(STDIN_FILENO,&masterSet);
    // aggiorno il massimo
    fdMax = listener;
    
    // main loop
    while(1){

        if(!logged){

            login(&server_Fd, &svAddr, &masterSet, &fdMax);

        }else{

            // inizializzo il set manipolato dalla select
            readSet = masterSet;
            // select in lettura senza timeout
            check((select(fdMax+1, &readSet, NULL,NULL,NULL)), "Select");

            // scorro i descrittori pronti in lettura
            for( i = 0; i<=fdMax ; i++){
                // se "i" pronto
                if(FD_ISSET(i, &readSet)){
                    
                    
                    if( i == listener){ // se "i" listener
                        //richiesta di connessione, creare un nuovo socket di comunicazione
                        connectionAccept(&masterSet, &fdMax, listener);
                    
                    }else if( i == STDIN_FILENO){   // se "i" stdin
                        char comando[10];
                        char buffStdin[BUFSIZE];

                        fgets(buffStdin, sizeof(buffStdin), stdin);

                        sscanf(buffStdin, "%9s" , comando);

                        if(isChatting > 0){
                            // se e' presente una chat attiva devo leggere solo i comandi della chat
                            if(strcmp(comando, "\\q") == 0){
                                // comando per uscire dalla chat
                                puts("\nchat terminata\n");
                                disattivaChat();

                            }else if(strcmp(comando, "\\u") == 0){
                                // comando per vedere i client online
                                getOnlineList(server_Fd);

                            }else if(strcmp(comando, "\\a") == 0){
                                // comando per aggiungere client alla chat
                                char username[USERNAME_LENGTH];
                                sscanf(buffStdin, "%9s %31s", comando, username);

                                printf("\nAggiunto al gruppo %s \n", username);

                                isChatting++;
                                // se non e' nella lista lo aggiungo
                                if(findUser(username) == -1){
                                    insertInList(username, 0, 0);
                                }
                                // metto chat attiva dell'username a true
                                attivaChat(username);

                            }else if(strcmp(comando, "share") == 0){
                                // comando per inviare file
                                char fileName[USERNAME_LENGTH];
                                sscanf(buffStdin, "%9s %31s", comando, fileName);

                                shareCommand(fileName, server_Fd, &masterSet, &fdMax);

                            }else{
                                // messaggio da inviare
                                sendMessage(buffStdin, server_Fd, &masterSet, &fdMax);
                            }
                        }else{
                            // se non e' presente una chat attiva devo leggere solo i comandi del menu
                            if(strcmp(comando, "hanging") == 0){
                                // comando per lista utenti che hanno inviato mentre era offline
                                hanging(server_Fd);

                            }else if(strcmp(comando, "show") == 0){
                                // comando per mostrare i messaggi pendenti da username
                                char username[USERNAME_LENGTH];
                                sscanf(buffStdin, "%9s %31s", comando, username);
                                showMessage(server_Fd, username);

                            }else if(strcmp(comando, "chat") == 0){
                                // comando per aprire una chat con username
                                char username[USERNAME_LENGTH];
                                sscanf(buffStdin, "%9s %31s", comando, username);

                                startChat(username);

                            }else if(strcmp(comando, "rubrica") == 0){
                                // comando per mostrare la rubrica
                                rubrica();

                            }else if(strcmp(comando, "out") == 0){
                                // comando per disconnettersi
                                puts("Disconnessione!");
                                exit(1);                          

                            }else if(strcmp(comando, "help") == 0){
                                // comando per mostrare il menu dei comandi
                                menuMessage();

                            }
                        }



                    }else if(i == server_Fd){    // se "i" server
                    
                        pk_type pkType;
                        // buffer per memorizzare il tipo di messaggio ricevuto
                        char buff[4];
                        int nBytesRecvd;
                        
                        // recv con MSG_PEEK non rimuove il dato dalla coda, prossima recv legge gli stessi dati
                        if((nBytesRecvd = recv(i, (void*) buff, sizeof(pkType), MSG_PEEK)) <= 0){
                            // se ricevo 0 e' la disconnessione del server
                            if(nBytesRecvd == 0){
                                printf("Server offline!\n");
                                server_Fd = -1;
                            
                            }else{  // -1 e' un errore
                                perror("Recv Error");
                                server_Fd = -1;
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
                        
                        case MSG_TEXT:{
                            puts("Ho ricevuto un messaggio text dal server");
                            reciveTextFromServer(i);
                        }break;
                        
                        case MSG_ACK_READ:{
                            puts("Ho ricevuto una notifica di lettura dal server");
                            reciveAckRead(i);
                        }break;

                        default:{
                            puts("Errore di invio pacchetto!");

                        }break;
                        }

                    }else{  // ricezione messaggio da altri device

                        pk_type pkType;
                        // buffer per memorizzare il tipo di messaggio ricevuto
                        char buff[4];
                        int nBytesRecvd;
                        
                        // recv con MSG_PEEK non rimuove il dato dalla coda, prossima recv legge gli stessi dati
                        if((nBytesRecvd = recv(i, (void*) buff, sizeof(pkType), MSG_PEEK)) <= 0){
                            // se ricevo 0 e' una disconnessione
                            if(nBytesRecvd == 0){
                                printf("socket %d hung up\n", i);
                                char username[USERNAME_LENGTH];
                                
                                if(!isActive(i)){   // se la chat non e' attiva elimino il device dalla lista

                                    delateDevice(i);

                                }else{  // se chat attiva inviero' il prossimo messaggio al server
                                    
                                    getUsername(username, i);
                                    printf("%s offline\n", username);
                                    // elimino il fd e la porta dalla lista
                                    insertFd(username, 0);
                                    insertPort(username, 0);

                                }

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
                        case MSG_TEXT:{
                            reciveText(i);
                        }break;

                        case MSG_FILE:{
                            reciveFile(i);
                        }break;

                        default:{
                            puts("Errore di invio pacchetto!");

                        }break;
                        }

                    }
                }
            }
        } 
    }

    return 0;
}


// funzione di gestione comando signup
void signup(char* username, char* password, int portaServer, int *serverFd, struct sockaddr_in *svAddr, fd_set *masterSet, int *fdMax){

    // pacchetto da inviare
    pk_signup pkSignup;
    // buffer per inviare in modalita text
    char buffSend[sizeof(pkSignup)];
    // pacchetto da ricevere
    pk_info pkInfoResponse;
    // buffer per memorizzare il messaggio di risposta
    char buffRecv[sizeof(pkInfoResponse)];


    // mi connetto al server se non e' ancora stato fatto 
    if (*serverFd == -1)
    {
        serverPort = portaServer;
        // creazione socket connesso al server
        if(connectServerRequest(serverFd, svAddr)){
            // aggiunta del descrittore al set
            FD_SET(*serverFd, masterSet);
            // aggiorno il massimo descrittore
            if(*serverFd > *fdMax){*fdMax = *serverFd;}
        }else{
            close(*serverFd);
            *serverFd = -1;
            return;
        }
    }

    // inserimento dati nella struttura
    pkSignup.mt = MSG_SIGNUP;
    strcpy(pkSignup.username, username);
    strcpy(pkSignup.password, password);
    // conversione a stringa del messaggio
    sprintf(buffSend, "%d %s %s", pkSignup.mt, pkSignup.username, pkSignup.password);
    // invio al server
    check( send(*serverFd, (void*) buffSend, sizeof(buffSend), 0), "Errore di comunicazione con il server" );

    puts("Ho inviato il messaggio di Signup al server, attendo risposta....");
    // ricezione dal server
    if(checkServer( recv(*serverFd, (void*) buffRecv, sizeof(buffRecv), 0), "Errore ricezione risposta SignUp" )){
        return;
    }
    // parsing e memorizzazione del messaggio
    sscanf(buffRecv, "%d %[^\n]", &pkInfoResponse.mt, pkInfoResponse.info);
    // mostro risposta
    printf("%s\n", pkInfoResponse.info);

}

// funzione di gestione comando in
bool in(char* username, char* password, int portaServer, int *serverFd, struct sockaddr_in *svAddr, fd_set *masterSet, int *fdMax){
    // pacchetto da inviare
    pk_in pkIn;
    // buffer per inviare in modalita text
    char buffSend[sizeof(pkIn)];
    // pacchetto da ricevere
    pk_info pkInfoResponse;
    // buffer per memorizzare il messaggio di risposta
    char buffRecv[sizeof(pkInfoResponse)];


    // mi connetto al server se non e' ancora stato fatto 
    if (*serverFd == -1)
    {
        serverPort = portaServer;
        // creazione socket connesso al server
        if(connectServerRequest(serverFd, svAddr)){
            // aggiunta del descrittore al set
            FD_SET(*serverFd, masterSet);
            // aggiorno il massimo descrittore
            if(*serverFd > *fdMax){*fdMax = *serverFd;}
        }else{
            close(*serverFd);
            *serverFd = -1;
            return false;
        }
    }

    // inserimento dati nella struttura
    pkIn.mt = MSG_IN;
    strcpy(pkIn.username, username);
    strcpy(pkIn.password, password);
    pkIn.port = myPort;
    // conversione a stringa del messaggio
    sprintf(buffSend, "%d %s %s %d", pkIn.mt, pkIn.username, pkIn.password, pkIn.port);
    // invio al server
    check( send(*serverFd, (void*) buffSend, sizeof(buffSend), 0), "Errore di comunicazione con il server" );

    puts("Ho inviato il messaggio di In al server, attendo risposta....");
    // ricezione dal server
    if(checkServer( recv(*serverFd, (void*) buffRecv, sizeof(buffRecv), 0), "Errore ricezione risposta In" )){
        return false;
    }
    // parsing e memorizzazione del messaggio
    sscanf(buffRecv, "%d %[^\n]", &pkInfoResponse.mt, pkInfoResponse.info);
    // mostro risposta
    printf("%s\n", pkInfoResponse.info);

    if(pkInfoResponse.mt == MSG_OK){
        strcpy(myUsername, username);
        // creazione cartella "username" se non esiste gia'
        if(mkdir(username, 0777) == -1){
            perror("Errore Creazione cartella");
        }else{
            puts("Cartella creata correttamente");
        }
        return true;
    }

    return false;
}

// funzione per gestire la sezione di login
void login(int *serverFd, struct sockaddr_in *svAddr, fd_set *masterSet, int *fdMax){
    
    char comando[10], username[USERNAME_LENGTH], password[USERNAME_LENGTH];
    int portaServer;

    puts("Login");
    // inserimento comando e parametri
    scanf("%9s %d %31s %31s", comando, &portaServer, username, password);

    if(strcmp(comando, "signup") == 0){
        // comando per la registrazione sul server
        signup(username, password, portaServer, serverFd, svAddr, masterSet, fdMax);
    }else if (strcmp(comando, "in") == 0){
        // comando per il login
        if( in(username, password, portaServer, serverFd, svAddr, masterSet, fdMax) ){
            logged = true;
            menuMessage();
        }
    }else{

        puts("Comando inesistente");
        
    }
     
}

// funzione per mostrare il menu dei comandi
void menuMessage(){

    puts("\nDigita un comando:");
    puts("1) help --> mostra i dettagli dei comandi");
    puts("2) hanging --> lista degli utenti che hanno inviato messaggi");
    puts("3) show username --> per ricevere i messaggi pendenti");
    puts("4) rubrica --> mostra la rubrica");
    puts("5) chat username --> avvia una chat con l'utente username");
    puts("  a) \\q --> termina la chat");
    puts("  b) \\u --> mostra gli utenti online");
    puts("  c) \\a username --> aggiunge l'username alla chat");
    puts("  d) share file --> invia il file");
    puts("6) out --> disconnette l'utente");
    printf("\n");
    fflush(stdout);
}

// funzione ler leggere e mostrare a video la rubrica
void rubrica(){

    // puntatore al file rubrica
    FILE* fRubrica;
    char username[USERNAME_LENGTH];
    bool vuoto = true;
    char path[USERNAME_LENGTH + 15];

    // inizializzo il percorso del file
    strcpy(path, myUsername);
    strcat(path, "/Rubrica.txt");

    // apro il file rubrica in modalita lettura e append
    if((fRubrica = fopen(path, "a+")) == NULL){
        printf("Error opening Rubrica file!");
        exit(1);
    }
    // leggo il conteuto 
    while( fscanf(fRubrica, "%s", username) != EOF ){
        vuoto = false;
        printf(">%s\n", username);
    }
    // se il file e' vuoto
    if(vuoto){
        printf("Rubrica vuota!\n");
    }
    // chiusura file
    fclose(fRubrica);

}



//  funzione per ricevere messaggi text dal server 
void reciveTextFromServer(int i){

    // struttura per text
    pk_text pkText;
    // buffer per memorizzare il messaggio di text
    char bufferRecv[sizeof(pkText)];


    check( recv(i, (void*) bufferRecv, sizeof(bufferRecv), 0), "Recv Error MSG_TEXT" );
    // parsing e memorizzazione del messaggio text
    sscanf(bufferRecv, "%d %s %d %s %d %[^\n]", &pkText.mt, pkText.userSource, &pkText.portSource, pkText.userDest, &pkText.portDest, pkText.text);
    printf("ho ricevuto ---> %d %s %d %s %d %s \n", pkText.mt, pkText.userSource, pkText.portSource, pkText.userDest, pkText.portDest, pkText.text);

    if(findUser(pkText.userSource) == -1){  // se non e' nella lista inserisco
        insertInList(pkText.userSource, pkText.portSource, 0);
    }else if(findUser(pkText.userSource) == 0){ // se e' gia presente nella lista inserisco la porta
        insertPort(pkText.userSource, pkText.portSource);
    }

    // se e' stata fatta "chat usernameSource" mostro a video il messaggio
    if(isUsernameActive(pkText.userSource)){
        printf("%s-> %s\n", pkText.userSource, pkText.text);
    }

    strcat(pkText.text, "\n");

    // salvo il messaggio
    saveMessage(pkText.userSource, false, false, pkText.text);
}

// funzione per ricevere notifiche di lettura messaggi
void reciveAckRead(int i){
    
    // pacchetto da ricevere
    pk_ack_read pkRead;
    // buffer per memorizzare il messaggio
    char bufferRecv[sizeof(pkRead)];
    // file di cronologia
    FILE* fp;
    char path[USERNAME_LENGTH*2];
    chatMessage newMessage;
    // file temporaneo
    FILE* fTemp;
    char tempPath[USERNAME_LENGTH*2];
    

    check( recv(i, (void*) bufferRecv, sizeof(bufferRecv), 0), "Recv Error MSG_ACK_READ" );
    // parsing e memorizzazione del messaggio
    sscanf(bufferRecv, "%d %s", &pkRead.mt, pkRead.username);
    printf("ho ricevuto ---> %d %s \n", pkRead.mt, pkRead.username);

    // inizializzo il percorso del file di cronologia
    strcpy(path, myUsername);
    strcat(path, "/");
    strcat(path, pkRead.username);
    strcat(path, ".txt");

    // inizializzo il percorso del file
    strcpy(tempPath, myUsername);
    strcat(tempPath, "/temp.txt");

    // apro il file di cronologia in lettura
    if( (fp = fopen(path, "r")) != NULL){

        // apro il file temporaneo in scrittura
        if((fTemp = fopen(tempPath, "w")) == NULL){
            printf("Error opening file temp!");
            exit(1);
        }

        // leggo tutto il contenuto del file di cronologia
        while(fread(&newMessage, sizeof(newMessage), 1, fp)){
            
            // se ho inviato il messaggio ma non lo aveva ricevuto ora lo contrassegno come ricevuto
            if((newMessage.inviato == true) && (newMessage.recapitato == false)){
                newMessage.recapitato = true;
            }

            // scrivo sul file temporaneo i dati
            fwrite(&newMessage, sizeof(newMessage), 1, fTemp);
        }

        // chiudo i file
        fclose(fp);
        fclose(fTemp);
        // rimuovo il vecchio file di cronologia
        remove(path);
        // rinomino il file temporaneo
        rename(tempPath, path);
    }

}

// funzione per ricevere messaggi text da altri device
void reciveText(int i){

    // struttura per text
    pk_text pkText;
    // buffer per memorizzare il messaggio di text
    char bufferRecv[sizeof(pkText)];


    check( recv(i, (void*) bufferRecv, sizeof(bufferRecv), 0), "Recv Error MSG_TEXT" );
    // parsing e memorizzazione del messaggio text
    sscanf(bufferRecv, "%d %s %d %s %d %[^\n]", &pkText.mt, pkText.userSource, &pkText.portSource, pkText.userDest, &pkText.portDest, pkText.text);
    printf("ho ricevuto ---> %d %s %d %s %d %s \n", pkText.mt, pkText.userSource, pkText.portSource, pkText.userDest, pkText.portDest, pkText.text);

    if(findUser(pkText.userSource) == 1){ // se non c'e' gia' salvo il fd nella lista
        insertFd(pkText.userSource, i);
    }

    // se e' stata fatta "chat usernameSource" mostro a video il messaggio
    if(isActive(i)){
        printf("%s-> %s\n", pkText.userSource, pkText.text);
    }

    strcat(pkText.text, "\n");

    // salvo il messaggio
    saveMessage(pkText.userSource, false, false, pkText.text);
}

// funzione per ricevere i file
void reciveFile(int i){
    
    // file da ricevere
    FILE* fp;
    // pacchetto da ricevere
    pk_file pkFile;
    // buffer per memorizzare il messaggio di risposta
    char bufferRecv[sizeof(pkFile)];
    int find;
    char newName[USERNAME_LENGTH*2];
    // buffer per ricevere le porzioni del file
    char buffChunk[CHUNKSIZE];
    int nBytesRecvd;
    unsigned long tmp;


    check( recv(i, (void*) bufferRecv, sizeof(bufferRecv), 0), "Recv Error MSG_FILE" );
    // parsing e memorizzazione del messaggio
    sscanf(bufferRecv, "%d %s %d %s %lu", &pkFile.mt, pkFile.userSource, &pkFile.portSource, pkFile.fileName, &pkFile.nChunk);
    printf("ho ricevuto ---> %d %s %d %s %lu \n", pkFile.mt, pkFile.userSource, pkFile.portSource, pkFile.fileName, pkFile.nChunk);

    // cerco l'username che mi ha inviato il file
    find = findUser(pkFile.userSource);
    if(find == -1){
        // se non c'e' lo inserisco
        insertInList(pkFile.userSource, pkFile.portSource, i);
    }else if(find == 0){
        // se c'e' ma non ho il fd e la porta
        insertPort(pkFile.userSource, pkFile.portSource);
        insertFd(pkFile.userSource, i);
    }else if(find == 1){
        // se c'e' ma non ho il fd
        insertFd(pkFile.userSource, i);
    }

    //inizializzo il nuovo nome del file
    strcpy(newName, myUsername);
    strcat(newName, "_");
    strcat(newName, pkFile.fileName);

    // apro il file in scrittura
    if((fp = fopen(newName, "wb")) == NULL){
        printf("Error opening file!");
        exit(1);
    }

    // ricezione pacchetti
    for( tmp = 0; tmp < pkFile.nChunk; tmp++){

        // ricevo dal device
        nBytesRecvd = recv(i, (void*) buffChunk, CHUNKSIZE, 0 );
        
        if(nBytesRecvd > 0){
            // scrivo su file
            fwrite(buffChunk, sizeof(char), nBytesRecvd, fp);
        }else{
            break;
        }
        
    }

    fclose(fp);
    
    if(nBytesRecvd <= 0){
        // se ricevo 0 e' una disconnessione del device
        if(nBytesRecvd == 0){
            printf("socket %d hung up\n", i);
            char username[USERNAME_LENGTH];
            
            if(!isActive(i)){   // se la chat non e' attiva elimino il device dalla lista

                delateDevice(i);

            }else{  // se chat attiva inviero' il prossimo messaggio al server
                
                getUsername(username, i);
                printf("%s offline\n", username);
                // elimino il fd e la porta dalla lista
                insertFd(username, 0);
                insertPort(username, 0);

            }

        }else{  // -1 e' un errore
            perror("Recv Error");
        }
        // chiudo il socket e rimuovo dal set
        close(i);
        FD_CLR(i, &masterSet);
        // elimino il file perche ci sono stati degli errori
        remove(newName);
    
    }else{
        // se la chat e' attiva mostro il messaggio di ricezione file
        if(isActive(i)){
            printf("%s-> ha inviato il file %s \n", pkFile.userSource, pkFile.fileName);
        }
    }
   
}


// funzione per aprire una chat
void startChat(char* username){

    // puntatore al file di cronologia
    FILE* fp;
    char path[USERNAME_LENGTH*2];
    chatMessage message;
    // puntatore al file di rubrica
    FILE* fRubrica;
    char usernameRubrica[USERNAME_LENGTH];
    char pathRubrica[USERNAME_LENGTH + 15];
    bool trovato = false;


    // inizializzo il percorso della rubrica
    strcpy(pathRubrica, myUsername);
    strcat(pathRubrica, "/Rubrica.txt");
    // apro la rubrica in lettura
    if((fRubrica = fopen(pathRubrica, "r")) != NULL){
        // leggo il contenuto
        while( fscanf(fRubrica, "%s", usernameRubrica) != EOF ){
            if(strcmp(usernameRubrica, username) == 0){
                trovato = true;
                break;
            }
        }
        // chiusura file
        fclose(fRubrica);
    }

    if(!trovato){
        printf("Username %s non in rubrica!\n", username);
        return;
    }


    // inizializzo il percorso del file
    strcpy(path, myUsername);
    strcat(path, "/");
    strcat(path, username);
    strcat(path, ".txt");


    printf("\nChat attiva con %s \n\n", username);

    isChatting = 1;
    // se non e' nella lista lo aggiungo
    if(findUser(username) == -1){
        insertInList(username, 0, 0);
    }
    // metto chat attiva dell'username a true
    attivaChat(username);

    // apro il file di cronologia
    if((fp = fopen(path, "r")) != NULL){
        // leggo e stampo tutto
        while(fread(&message, sizeof(message), 1, fp)){
            if(message.inviato){
                if(message.recapitato){
                    printf("**%s", message.text);
                }else{
                    printf("*%s", message.text);
                }
            }else{
                printf("%s-> %s", username, message.text);
            }
        }
        // chiusura file
        fclose(fp);
    }


}

// funzione per inviare i messaggi di testo
void sendMessage(char* message, int serverFd, fd_set *masterSet, int *fdMax){
    
    deviceConnected *curr = deviceConnectedList;

    while(curr != NULL){
        // cerco tutte le chat attive
        if(curr->activeChat){
            if(curr->port == 0){    // se non ho la porta invio al server

                sendMessageToServer(serverFd, message, curr->username);

            }else if( (curr->port != 0) && (curr->fd == 0) ){    // se non ho il fd devo connettermi al destinatario

                int newFd = connectRequest(masterSet, fdMax, curr->port);
                
                if(newFd == 0){     // caso in cui e' cambiata la porta del destinatario perche si e' disconnesso
                    
                    // rimuovo la porta 
                    insertPort(curr->username, 0);
                    // invio al server
                    sendMessageToServer(serverFd, message, curr->username);

                }else{

                    printf("connesso con fd: %d\n", newFd);
                    // inserisco il fd alla lista
                    insertFd(curr->username, newFd);
                    // invio al device
                    sendMessageToDevice(newFd, curr->port, curr->username ,message);

                }

            }else if( (curr->port != 0) && (curr->fd != 0) ){   // se ho anche il fd invio direttamente

                sendMessageToDevice(curr->fd, curr->port, curr->username ,message);
                
            }
        }
        curr = curr->next;
    }

}

// funzione per inviare un messaggio di testo al server
void sendMessageToServer(int serverFd, char* message, char* username){

    // pacchetto da inviare
    pk_text pkText;
    // buffer per inviare in modalita text
    char buffSend[sizeof(pkText)];
    // pacchetto da ricevere
    pk_server_ack pkAckResponse;
    // buffer per memorizzare il messaggio di risposta
    char buffRecv[sizeof(pkAckResponse)];

    puts("Send message to server");


    // inserimento dati nella struttura
    pkText.mt = MSG_TEXT;
    strcpy(pkText.userSource, myUsername);
    pkText.portSource = myPort;
    strcpy(pkText.userDest, username);
    pkText.portDest = 0;
    strcpy(pkText.text, message);
    
    // conversione a stringa del messaggio
    sprintf(buffSend, "%d %s %d %s %d %s", pkText.mt, pkText.userSource, pkText.portSource, pkText.userDest, pkText.portDest, pkText.text);
    
    // invio al server
    if(serverFd == -1){
        printf("Server offline!\n");
        return;
    }else{
        check( send(serverFd, (void*) buffSend, sizeof(buffSend), 0), "Errore di comunicazione con il server" );
    }

    puts("Ho inviato il messaggio di Text al server, attendo risposta....");
    printf("ho inviato ---> %d %s %d %s %d %s", pkText.mt, pkText.userSource, pkText.portSource, pkText.userDest, pkText.portDest, pkText.text);

    // ricezione dal server
    if(checkServer( recv(serverFd, (void*) buffRecv, sizeof(buffRecv), 0), "Errore ricezione risposta ack")){
        return;
    }

    // parsing e memorizzazione del messaggio
    sscanf(buffRecv, "%d %d", &pkAckResponse.mt, &pkAckResponse.portDest);
    // mostro risposta
    printf("Ricevuto-> %d %d\n", pkAckResponse.mt, pkAckResponse.portDest);
    
    // controllo il tipo di ack ricevuto
    if(pkAckResponse.mt == MSG_ACK_SEND){
        puts("ack send");
        // inserisco la porta del dest ricevuta dal server
        insertPort(username, pkAckResponse.portDest);
    }else if(pkAckResponse.mt == MSG_ACK_STORE){
        puts("ack store");
    }else if(pkAckResponse.mt == MSG_ERR){
        puts("Error: username does not exist!");
        return;
    }
    
    // salvo il messaggio se non ci sono errori
    if(pkAckResponse.mt != MSG_ERR){
        if(pkAckResponse.mt == MSG_ACK_SEND){
            // se il server mi risponde con un ack send allora e' stato recapitato
            saveMessage(username, true, true, message);
        }else if(pkAckResponse.mt == MSG_ACK_STORE){
            // se il server mi risponde con un ack store allora non e' stato recapitato
            saveMessage(username, true, false, message);
        }
    }

}

// funzione per inviare un messaggio di testo al device
void sendMessageToDevice(int fd, int port, char* username ,char* message){

    // pacchetto da inviare
    pk_text pkText;
    // buffer per inviare in modalita text
    char buffSend[sizeof(pkText)];

    // inserimento dati nella struttura
    pkText.mt = MSG_TEXT;
    strcpy(pkText.userSource, myUsername);
    pkText.portSource = myPort;
    strcpy(pkText.userDest, username);
    pkText.portDest = port;
    strcpy(pkText.text, message);

    // conversione a stringa del messaggio
    sprintf(buffSend, "%d %s %d %s %d %s", pkText.mt, pkText.userSource, pkText.portSource, pkText.userDest, pkText.portDest, pkText.text);
    // invio al device
    check( send(fd, (void*) buffSend, sizeof(buffSend), 0), "Errore di comunicazione con il device" );

    puts("Ho inviato il messaggio di Text al device");
    printf("ho inviato ---> %d %s %d %s %d %s", pkText.mt, pkText.userSource, pkText.portSource, pkText.userDest, pkText.portDest, pkText.text);

    // salvo il messaggio
    saveMessage(username, true, true, message);

}



// funzione per ricevere la lista degli utenti online dal server
void getOnlineList(int serverFd){

    int n, i;
    // pacchetto da inviare
    pk_onlinelist pkOnlineSend;
    // buffer per inviare in modalita text
    char buffSend[sizeof(pkOnlineSend)];
    // pacchetto da ricevere
    pk_onlinelist_number pkNumberResponse;
    // buffer per memorizzare il messaggio di risposta
    char buff[sizeof(pkNumberResponse)];
    // pacchetto da ricevere
    pk_onlinelist pkOnlineResponse;
    // buffer per memorizzare il messaggio di risposta
    char buffRecv[sizeof(pkOnlineResponse)];

    // inserimento dati nella struttura
    pkOnlineSend.mt = MSG_ONLINELIST;
    strcpy(pkOnlineSend.username, myUsername);

    // conversione a stringa del messaggio
    sprintf(buffSend, "%d %s", pkOnlineSend.mt, pkOnlineSend.username);
    
    // invio al server
    if(serverFd == -1){
        printf("Server offline!\n");
        return;
    }else{
        check( send(serverFd, (void*) buffSend, sizeof(buffSend), 0), "Errore di comunicazione con il server" );
    }

    // ricezione dal server del numero di device online
    if(checkServer( recv(serverFd, (void*) buff, sizeof(buff), 0), "Errore ricezione risposta" )){
        return;
    }
    // parsing e memorizzazione del messaggio
    sscanf(buff, "%d %d", &pkNumberResponse.mt, &pkNumberResponse.number);
    // mostro risposta
    printf("Ricevuto-> %d %d\n", pkNumberResponse.mt, pkNumberResponse.number);

    n = pkNumberResponse.number;

    // ricevo e stampo gli n utenti online
    printf("Ci sono %d utenti online:\n", n);
    for ( i = 0; i < n; i++){
        // ricezione dal server
        if(checkServer( recv(serverFd, (void*) buffRecv, sizeof(buffRecv), 0), "Errore ricezione risposta server" )){
            return;
        }
        // parsing e memorizzazione del messaggio
        sscanf(buffRecv, "%d %s", &pkOnlineResponse.mt, pkOnlineResponse.username);
        // mostro risposta
        printf("%s\n", pkOnlineResponse.username);

    }
    
}



// funzione per salvare la cronologia della chat
void saveMessage(char* username, bool inviato, bool recapitato, char* message){

    FILE* fp;
    char path[USERNAME_LENGTH*2];
    chatMessage newMessage;

    // inizializzo il percorso del file
    strcpy(path, myUsername);
    strcat(path, "/");
    strcat(path, username);
    strcat(path, ".txt");

    // inizializzo la struttura da salvare
    newMessage.inviato = inviato;
    newMessage.recapitato = recapitato;
    strcpy(newMessage.text, message);

    // apro il file in modalita append
    if((fp = fopen(path, "a")) == NULL){
        printf("Error opening file!");
        exit(1);
    }

    fwrite(&newMessage, sizeof(newMessage), 1, fp);
    fclose(fp);

}



// funzione per gestire il comando hanging
void hanging(int serverFd){
    
    // pacchetto da inviare
    pk_info pkInfoSend;
    // buffer per inviare in modalita text
    char buffSend[sizeof(pkInfoSend)];
    // pacchetto da ricevere
    pk_hanging pkHangingResponse;
    // buffer per memorizzare il messaggio di risposta
    char buffRecv[sizeof(pkHangingResponse)];
    int nBytesRecvd;
    bool ricevuto = false;

    // inserimento dati nella struttura
    pkInfoSend.mt = MSG_HANGING;
    strcpy(pkInfoSend.info, myUsername);

    // conversione a stringa del messaggio
    sprintf(buffSend, "%d %s", pkInfoSend.mt, pkInfoSend.info);
    // invio al server
    if(serverFd == -1){
        printf("Server offline!\n");
        return;
    }else{
        check( send(serverFd, (void*) buffSend, sizeof(buffSend), 0), "Errore di comunicazione con il server" );
    }
    
    puts("Ho inviato al server richiesta di hanging");

    // ricezione dal server
    while((nBytesRecvd = recv(serverFd, (void*) buffRecv, sizeof(buffRecv), 0)) > 0 ){
        // parsing e memorizzazione del messaggio
        sscanf(buffRecv, "%d %s %d %[^\n]", &pkHangingResponse.mt, pkHangingResponse.username, &pkHangingResponse.number, pkHangingResponse.timestamp);
        if(pkHangingResponse.mt == MSG_END){
            // ricevo END allora e' l'ultimo messaggio
            break;
        }else{
            ricevuto = true;
            printf("%s %d %s \n", pkHangingResponse.username, pkHangingResponse.number, pkHangingResponse.timestamp);
        }
    }

    // controllo disconnessione server
    if(checkServer(nBytesRecvd, "Errore ricezione hanging dal server")){
        return;
    }

    // caso in cui non ho ricevuto nulla
    if(!ricevuto){
        printf("Non ci sono messaggi sul server!\n");
    }
}



// funzione per gestire il comando show
void showMessage(int serverFd, char* username){
    // pacchetto da inviare
    pk_show pkShowSend;
    // buffer per inviare in modalita text
    char buffSend[sizeof(pkShowSend)];
    // pacchetto da ricevere
    pk_show_response pkShowResponse;
    // buffer per memorizzare il messaggio di risposta
    char buffRecv[sizeof(pkShowResponse)];
    int nBytesRecvd;
    bool ricevuto = false;


    // inserimento dati nella struttura
    pkShowSend.mt = MSG_SHOW;
    strcpy(pkShowSend.userRequest, myUsername);
    strcpy(pkShowSend.userHanging, username);

    // conversione a stringa del messaggio
    sprintf(buffSend, "%d %s %s", pkShowSend.mt, pkShowSend.userRequest, pkShowSend.userHanging);
    // invio al server
    if(serverFd == -1){
        printf("Server offline!\n");
        return;
    }else{
        check( send(serverFd, (void*) buffSend, sizeof(buffSend), 0), "Errore di comunicazione con il server" );
    }
    printf("Ho inviato al server richiesta di show %s\n", pkShowSend.userHanging);

    // ricezione dal server
    while((nBytesRecvd = recv(serverFd, (void*) buffRecv, sizeof(buffRecv), 0)) > 0 ){
        // parsing e memorizzazione del messaggio
        sscanf(buffRecv, "%d %[^\n]", &pkShowResponse.mt, pkShowResponse.text);
        if(pkShowResponse.mt == MSG_END){
            // ricevo END allora e' l'ultimo messaggio
            break;
        }else{
            ricevuto = true;
            printf("Ricevuto -> %s\n", pkShowResponse.text);

            strcat(pkShowResponse.text, "\n");
            // salvo il messaggio
            saveMessage(username, false, false, pkShowResponse.text);
        }
    }

    // controllo disconnessione server
    if(checkServer(nBytesRecvd, "Errore ricezione show dal server")){
        return;
    }

    // caso in cui non ho ricevuto nulla
    if(!ricevuto){
        printf("Non ci sono messaggi sul server!\n");
    }

}


// funzione per gestire il comando share
void shareCommand(char* fileName, int serverFd, fd_set *masterSet, int *fdMax){
    deviceConnected *curr = deviceConnectedList;

    while(curr != NULL){
        // cerco tutte le chat attive
        if(curr->activeChat){

            if( (curr->fd == 0) ){  //se non ho il fd chiedo al server la porta poi invio se online

                int port = requestPort(serverFd, curr->username);
                if(port == 0){
                    printf("%s offline impossibile inviare file \n", curr->username);
                }else{
                    // mi connetto al device
                    int newFd = connectRequest(masterSet, fdMax, port);

                    if(newFd == 0){     // caso in cui e' cambiata la porta del destinatario perche si e' disconnesso
                    
                        // rimuovo la porta 
                        insertPort(curr->username, 0);

                        printf("%s offline impossibile inviare file \n", curr->username);

                    }else{

                        printf("connesso con fd: %d\n", newFd);
                        // inserisco il fd nella lista
                        insertFd(curr->username, newFd);
                        // invio file al device
                        sendFile(newFd, fileName);

                    }
                }

            }else if( (curr->fd != 0) ){   // se ho il fd invio direttamente

                sendFile(curr->fd, fileName);
                
            }
        }
        curr = curr->next;
    }
}

// funzione per richiedere la porta di un client al server
int requestPort(int serverFd, char* username){

    // pacchetto da inviare
    pk_port_req pkPort;
    // buffer per inviare in modalita text
    char buffSend[sizeof(pkPort)];
    // pacchetto da ricevere
    pk_server_ack pkAckResponse;
    // buffer per memorizzare il messaggio di risposta
    char buffRecv[sizeof(pkAckResponse)];

    // inserimento dati nella struttura
    pkPort.mt = MSG_PORT_REQ;
    strcpy(pkPort.username, username);

    // conversione a stringa del messaggio
    sprintf(buffSend, "%d %s", pkPort.mt, pkPort.username);
    
    // invio al server
    if(serverFd == -1){
        printf("Server offline!\n");
        return 0;
    }else{
        check( send(serverFd, (void*) buffSend, sizeof(buffSend), 0), "Errore di comunicazione con il server" );
    }

    puts("Ho inviato il messaggio di Richiesta porta al server, attendo risposta....");
    printf("ho inviato ---> %d %s \n", pkPort.mt, pkPort.username);

    // ricezione dal server
    if(checkServer( recv(serverFd, (void*) buffRecv, sizeof(buffRecv), 0), "Errore ricezione risposta ack")){
        return 0;
    }

    // parsing e memorizzazione del messaggio
    sscanf(buffRecv, "%d %d", &pkAckResponse.mt, &pkAckResponse.portDest);
    // mostro risposta
    printf("Ricevuto-> %d %d\n", pkAckResponse.mt, pkAckResponse.portDest);

    // controllo il tipo di ack ricevuto
    if(pkAckResponse.mt == MSG_OK){
        // inserisco la porta del dest ricevuta dal server
        insertPort(username, pkAckResponse.portDest);
        return pkAckResponse.portDest;

    }else if(pkAckResponse.mt == MSG_ERR){
        return 0;
    }

    return 0;
}

// funzione per inviare i file
void sendFile(int fd, char* fileName){
    
    // file da inviare
    FILE* fp;
    // primo pacchetto da inviare
    pk_file pkFile;
    // buffer per inviare in modalita text
    char buffSend[sizeof(pkFile)];
    // variabile per il calcolo della dimensione del file
    unsigned long nChunk;
    unsigned long tmp;

    // apro in modalita lettura
    if((fp = fopen(fileName, "rb")) == NULL ){
        // se il file non c'e' termino la funzione
        puts("Error file not found");
        return;
    }

    // calcolo della dimensione del file
    // porto il puntatore del file alla fine
    fseek(fp, 0, SEEK_END);
    // numero byte
    tmp = ftell(fp);
    // calcolo del numero dei pacchetti approssimanto per eccesso
    nChunk = (tmp + CHUNKSIZE - 1)/CHUNKSIZE;
    // porto il puntatore del file all inizio
    fseek(fp, 0, SEEK_SET);

    // inserimento dati nella struttura
    pkFile.mt = MSG_FILE;
    strcpy(pkFile.userSource, myUsername);
    pkFile.portSource = myPort;
    strcpy(pkFile.fileName, fileName);
    pkFile.nChunk = nChunk;

    // conversione a stringa del messaggio
    sprintf(buffSend, "%d %s %d %s %lu", pkFile.mt, pkFile.userSource, pkFile.portSource, pkFile.fileName, pkFile.nChunk);
    // invio al device
    check( send(fd, (void*) buffSend, sizeof(buffSend), 0), "Errore di comunicazione con il device" );

    puts("Ho inviato il messaggio di File al device");
    printf("ho inviato ---> %d %s %d %s %lu \n", pkFile.mt, pkFile.userSource, pkFile.portSource, pkFile.fileName, pkFile.nChunk);

    // invio pacchetti
    for( tmp = 0; tmp < nChunk; tmp++){
        // buffer per inviare le porzioni del file
        char buffChunk[CHUNKSIZE];

        // leggo il file
        int nRead = fread(buffChunk, sizeof(char), CHUNKSIZE, fp);
        // se ho letto qualcosa invio
        if(nRead > 0){
            check( send(fd, (void*) buffChunk, nRead, 0), "Errore di comunicazione con il device" );
        }
    }
    
    fclose(fp);

}


