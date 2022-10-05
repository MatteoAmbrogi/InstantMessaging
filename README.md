# InstantMessaging
Applicazione a linea di comando basata su paradigma ibrido peer-to-peer e client-server, realizzata con Socket TCP in C che implementa una moderna applicazione di instant messaging con notifiche, chat di gruppo, file sharing e messaggistica offline.

[Qui](https://github.com/MatteoAmbrogi/InstantMessaging/blob/main/Specifiche.pdf) sono presenti le specifiche del progetto insieme ai dettagli implementativi del server e dei device con le loro funzioni e comandi.

Le tecnologie utilizzate e i protocolli implementati per la realizzazione dell’applicazione sono documentati [qui](https://github.com/MatteoAmbrogi/InstantMessaging/blob/main/Documentazione.pdf).

## Struttura del progetto
* `serv.c`: server che gestisce i device online;
* `dev.c`: device con cui è connesso l’utente;
* `makefile`: script per compilare i file del progetto;
* `exec.sh`: script bash che esegue il makefile e avvia il server sulla porta 4242 e 3 device sulle porte 5001-5003. 
