                                    TEMA 2 - PCOM
                                    NUME: 
                                    GRUPA: 

    Acest readme va atinge punctele principale ale implementarii. Voi avea in vedere 
indicatiile luate in enunt si voi explica, pe rand, particularitatile implementarii.

    1. Alegere EPOLL

    In enunt este specificat elementul de performanta. In general, o imlementare cu select
are dezavantajul iterarii prin socketi de fiecare data cand un eveniment are loc, lucru 
ce nu se doreste pentru un server. EPOLL este folosit in edge-triggered mode.

    2. Structura care retine informatii despre clienti

    In client_data.h/.c se gasesc declaratiile si implementarile pentru aceasta structura.
Este un array dinamic normal in C. Se modifica on demand, atat dimensiunea lui ClientArray,
cat si dimensiunea numarului de subscriptii dintr-un client din array. Nu cred ca este nevoie
de mai multe explicatii, functiile din client_data sunt destul de clare si utilizarea lor in
cod practic substituie structurile deja existente intr-un limbaj orientat pe obiecte(exemplu 
C++). Las aici postarea de referinta:
    https://stackoverflow.com/questions/3536153/c-dynamically-growing-array

    3. TCP Handling si protocol peste nivel 4

    Am incercat sa implementez un protocol peste tcp cat mai robust, fara sa complic 
implementarea. Se pot gasi structurile in packets.h, iar semnificatia lor este evidenta.
Protocolul are doua parti intre client si server:
    a. handshake de inceput, in momentul in care un client nou se conecteaza la server;
o data ce cineva s-a conectat, trimite hello si serverul decide daca inchide conexiunea
(in cazul in care cel conectat trebuie sa fie deconectat, de exemplu cand altcineva are
acelasi nume si este online), daca updateaza is_online in structura de clienti, sau 
daca adauga un client nou
    b. comunicatia de subscribe/unsubscribe; am facut o structura si un parser pentru
acest tip de comunicatie; nu are nimic special, clientul parseaza de la tastatura input-ul
si trimite pachetul de subscribe/unsubscribe catre server. Serverul apoi trimite ack, pentru
ca nu putem printa in consola clientului faptul ca s-a abonat cu succes fara un ack.
    c. comunicatia se termina atunci cand clientul sau serverul inchide conexiunea; asta
se va intampla de obicei cu "exit" citit de la stdin

    4. Parsare wildcard

    Exista doua functii in server.c care se ocupa de aceasta problema. Un path va fi spart
intr-un array de segmente. Exemplu:
    "ana/are/mere" -> [('ana', 'are', 'mere'), 3], unde 3 este lungimea array-ului de segmente.
    Procedeul de decizie este facut prin parcurgerea a doua astfel de array-uri extrase din
pathuri; primul path este path-ul primit de la udp, adica topicul pe care s-a facut post;
al doilea path este path-ul la care clientul este abonat; se vor parcurge simultan; de mentionat
ca functia mea suporta wildcard-uri doar pe path-ul pe care clientul este abonat, insa 
au fost probleme in checker. 
    
    gasire + in path abonament -> avansam in ambele stringuri
    gasire * in path abonament -> avansam doar daca actualul string din celalalt path da match
cu urmatorul string din path_abonament; altfel, avansez doar in primul array de string-uri si
am grija sa tratez cazul in care path_abonament se termina cu *. 

    5. Handle posibile erori pentru functii

    Am incercat, pe cat posibil, sa folosesc codurile de eroare pentru a semnala probleme.
Sper ca este ok, nu sunt sigur pentru ca functiile nu au dat fail pana acum.
    