# Progetto

Questa repository contiene l'implementazione in linguaggio C per la gestione concorrente e il mantenimento di una Minimum Spanning Forest, su un grafo soggetto a rimozioni dinamiche degli archi per `msf.c` e anche per le addizioni nel caso di `msf.py`.

## 1. Dettaglio costruzione liste `Lu` e `Lv`

La lista `Lu` rappresenta i nodi raggiungibili partendo da `Lu`,
analogamente la lista `Lv` rappresenta i nodi raggiungibili da `Lv`.
La costruzione delle liste `Lu` e `Lv` a seguito della cancellazione di un arco viene gestita all'interno della funzione `cancella_arco` eseguita dai thread worker, appoggiandosi alla funzione di visita `bfs()`. 

L'obiettivo di questa fase è esplorare la componente connessa, per capire se la rimozione dell'arco `(u, v)` ha diviso la componente in due componenti connesse separate.

Le strutture dati impiegate durante l'uso di `bfs` sono le seguenti:

* **deque**: Viene utilizzato un array come deque, grazie a 2 puntatori `sx` e `dx`, che puntano rispettivamente, alla posizione da cui estrarre un nodo, e la posizione dove inserirlo. 
Quando estraiamo si fa `q[sx ++]` in quando leggiamo il dato alla posizione `sx` e incrementiamo, in modo analogo lo si fa per l'inserimento `q[dx ++]`.
Per assicurarci che i pointer non escano dalla queue, essa viene inizalizzata con una lunghezza `g->nNodi`.
Alla fine si fa una `realloc` di lunghezza `dx`, in quanto la `q` verrà usata come `Lu` o `Lv`, evitando ulteriori riallocamenti.  
* **Array dei nodi visitati (`visti`)**: Un array di interi di dimensione pari al numero totale dei nodi `g->nNodi`, inizializzato a 0. Quando un nodo viene scoperto, il suo valore viene settato a 1 per evitare cicli e visite ridondanti.

## 2. Gestione Concorrente: Mutex e Condition Variables

Essendo che si fa uso del paradigma produttori-consumatori è necessario usare strutture dati per la sincronizzazione.

### Strutture di Sincronizzazione Utilizzate
* **Semafori (`free_sem`, `full_sem`) e Mutex del Buffer (`m_buff`)**: Regolano l'accesso al buffer circolare delle operazioni in modo standard, garantendo che i thread consumatori si sospendano se il buffer è vuoto e il thread principale (produttore) si sospenda se è pieno.
`free_sem` è inizializzato a `Buf_size` e `full_sem` a 0, rispettivamente indicano quando è possibile inserire istruzioni, e quando è possibile prenderne.
Per i thread consumatori, è necessario l'uso di un `m_buff` per assicurarsi un corretto accesso al buffer.
* **Mutex Globale (`global`) e Condition Variable (`cond_cCon`)**: Servono per acquisire il diritto esclusivo di operare su una specifica Componente Connessa.
* **Array Booleano (`bool_cCon`)**: Un array (condiviso) che mappa l'ID della radice di una componente connessa a un booleano, indicando se un thread sta attualmente modificando quella componente (`true` se sta venendo operata `false` altrimenti).
Lavora in tandem con `global` e `cond_cCon`, in quanto viene modificato solo quando viene acquisito il mutex globale e quando nessuno sta lavorando sulle componenti connesse puntante da, `data->bool_cCon[grafo->cCon[u]]` e `data->bool_cCon[grafo->cCon[v]]`.
* **Array di Mutex per l'Hash Table (`mut_gHash`)**: Per consentire l'accesso parallelo alla tabella hash degli archi senza bloccare l'intera struttura, viene usato un array di mutex. L'accesso al bucket avviene bloccando solo il mutex all'indice 
`(hash_idx % num_threads)`.

### Sequenza Temporale e Prevenzione delle Race Condition
Quando un thread estrae un'operazione di cancellazione dell'arco `(u, v)`, segue questa sequenza:

1.  **Attesa di accesso al buffer**
    Il thread aspetta con `sem_wait(data->full)` che siano presenti slot liberi, quando è cosi ottiene il mutex del buffer, estrare le informazioni, aumenta l'indice, restituisce il mutex e poi fa una post su `data->free`, per segnalare che uno slot è libero.
2.  **Richiesta di Esclusività (Locking Logico della Componente Connessa)**:
    Il thread acquisisce il mutex `global`. Controlla tramite l'array `bool_cCon` se le componenti connesse a cui appartengono `u` o `v` sono già bloccate da un altro thread. 
    Se lo sono, si mette in attesa passiva sulla condition variable `cond_cCon`.
    Se non lo sono, imposta a `true` il flag delle rispettive componenti così che nessuno ci può accedere, segnala eventuali thread in attesa e rilascia il mutex `global`, così che altri thread possono vedere se possono accedere all loro cCon.
3.  **Modifica Strutturale (Rimozione e Ricerca MSF)**:
    Se l'arco che mi è stato chiesto di rimuovere non esiste, il thread acquisice il mutex `global` aggiorna l'array `grafo->bool_cCon[oldcConu]` e `grafo->bool_cCon[oldcConv]` a `false` in quanto non lavora più su quelle componenti connesse, e poi fà signal su `cond_cCon` e rilasca il mutex `global`.
    Altrimenti essendo l'unico thread ad operare su quella specifica componente connessa, può liberamente aggiornare la lista di adiacenza e rimuovere l'arco in questione.
    Poi usa la `bfs` a partire da `u` e da `v` per creare le liste dei nodi raggiunti da questi 2.
4.  **Aggiornamento Hash Table**:
    La cancellazione fisica dall'hash table (`removeHash`) e della ricerca (`findHash`) viene protetta dal mutex specifico di quel bucket (`mut_gHash[idx%(numthread)]`).
5. **Aggiornamento Grafo**:
    Quando si aggiornano dati del grafo come `grafo->numArchi`, `grafo->costoMSF` e `grafo->numCoCo`, si usa il mutex condiviso da tutti i thread: `data->g_info`, che blocca l'accesso a queste strutture, in modo da poter rendere il risultato sempre accurato, anche durante l'esecuzione di operazioni su componenti connesse diverse. 
    Quando si ricalcola l'indice di `grafo->cCon`, lo si fa dentro una zona protetta dal mutex `data->global`,
    che è lo stesso che i thread usano per accedere all'array booleano per vedere quali cCon sono libere.
    Così facendo non si permette a nuovi consumatori di vedere gli archi che stanno venendo aggiornati come appartenenti ad una cCon diversa, e quindi, liberi su cui operare; azione che permetterebbe a 2 o più thread di operare sulla stessa cCon, andando a creare problemi quali errori di indexing nell'array `grafo->cCon`, frammentandolo in zone con vecchi indici e nuovi (in quanto le scritture non avverrebbero sequenzialmente, ma in contemporanea).
6.  **Rilascio Risorse**:
    A operazione conclusa, il thread riacquisisce il mutex `global`, rimette a `false` i flag nell'array `bool_cCon` per `u` e `v`, risveglia i thread dormienti con una `pthread_cond_signal` e rilascia il mutex `global`.

### Esempi di Esecuzione in Parallelo
* **Esecuzione Parallela Ottimale**: Se il Thread 1 deve rimuovere l'arco tra `A` e `B` (appartenenti alla Componente Connessa 1) e il Thread 2 deve rimuovere l'arco tra `X` e `Y` (appartenenti alla Componente Connessa 2), entrambi i thread troveranno i rispettivi `bool_cCon` a `false`. Imposteranno i loro flag e procederanno in parallelo con la BFS e i calcoli MSF simultaneamente, sfruttanto il parallelismo.
* **Esecuzione Sequenziale**: Se il Thread 1 agisce sull'arco `(A, B)` della Componente 1, e il Thread 2 tenta di rimuovere l'arco `(B, C)` che è sempre nella stessa componente, il Thread 2 troverà `bool_cCon[1] == true`. Verrà sospeso dalla condition variable finché il Thread 1 non avrà finito e ripristinato l'albero di copertura, assicurando così un comportamento sicuro.
* **Tabella Hash**: Se il Thread 1 accede sulla tabella hash alla posizione I, acquisice il lock `mut_gHash[I % nmutex]` e procede.
Se nel frattempo anche il Thread 2 vuole accedere sulla tabella hash alla posizione I,perchè lavora su un'altra componente connessa che però ha degli archi nella posizione I, prova ad acquistare lo stesso lock, che però è già preso, e quindi si bloccherà finchè non verrà rilasciato.

### Correttezza
Essendo che il codice quando opera in una fase (che sia lettura dal buffer, eliminazione arco e segnalazione agli altri thread) si assicura prima di avere un solo mutex (che sia `global`,`mut_gHash[I % nmutex]` o `m_buffer`), e ad operazione finita lo rilascia; e anche grazie al fatto che lock e unlock di `m_buffer` avvenga dentro i semafori, permette al codice non non andare mai in deadlock.
