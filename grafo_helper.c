#include "grafo_helper.h"
// Funzioni utilizzate
unsigned int hash(int u, int v, int size){
    // Moltiplichiamo per dei numeri primi onde ridurre la chance di collisione
    unsigned int res = u*P4;
    res = res + (v*P4);
    return (res*P4)%size;
}

void insert_hash(int i,int u,int v,int w,arco **gHash){ // O(1)
    arco *curr = malloc(sizeof(arco));
    if (!curr){
        fprintf(stderr,"Errore malloc in insert_hash\n");
        exit(1);
    }
    // Sono arrivato ad una posizione libera
    curr->u = u;
    curr->v = v;
    curr->weight = w;
    curr->msf = false;
    curr->next = gHash[i]; // Inserimento in testa
    gHash[i] = curr;
}

void insert_vicini(int u,int v,int w,elemento **vicini){ //O(1)

    // Alloca nuovo nodo per la lista di u
    elemento *curru = malloc(sizeof(elemento));
    if (!curru){
        // Errore malloc
        fprintf(stderr,"Errore malloc in insert_vicini u\n");
        exit(1);
    }
    curru->id = v;
    curru->w = w;
    curru->msf = false;
    curru->next = vicini[u]; // Inserimento in testa
    vicini[u] = curru;

    // Alloca nuovo nodo per la lista di v
    elemento *currv = malloc(sizeof(elemento));
    if (!currv){
        // Errore malloc
        fprintf(stderr,"Errore malloc in insert_vicini v\n");
        exit(1);
    }
    currv->id = u;
    currv->w = w;
    currv->msf = false;
    currv->next = vicini[v]; // Inserimento in testa
    vicini[v] = currv;
}

// Funzione di comparazione per qsort basata su id
int compare_temp(const void* a, const void* b) { //O(1)
    temp_elemento *elemA = (temp_elemento *)a;
    temp_elemento *elemB = (temp_elemento *)b;
    if (elemA->id > elemB->id) return 1;
    if (elemA->id < elemB->id) return -1;
    return 0;
}

void ordina_lista(elemento *vicini) { //O(nlog(n) + 2n) meno di un secondo però
    if (vicini == NULL) return;

    // Contiamo gli elementi
    int n = 0;
    elemento *curr = vicini;
    while (curr != NULL) {
        n++;
        curr = curr->next;
    }

    // Allochiamo un array di strutture temporanee
    temp_elemento *dummy = malloc(sizeof(temp_elemento) * n);
    if (!dummy) {
        fprintf(stderr, "Errore malloc in ordina_lista\n");
        exit(1);
    }

    // Popoliamo l'array dummy e lo ordiniamo
    curr = vicini;
    for (int i = 0; i < n; i++) {
        dummy[i].id = curr->id;
        dummy[i].w = curr->w;
        dummy[i].msf = curr->msf;
        curr = curr->next;
    }

    qsort(dummy, n, sizeof(temp_elemento), compare_temp);

    //Riscriviamo i valori nella lista originale
    curr = vicini;
    for (int i = 0; i < n; i++) {
        curr->id = dummy[i].id;
        curr->w = dummy[i].w;
        curr->msf = dummy[i].msf;
        curr = curr->next;
    }

    // Deallocare
    free(dummy);
}

int find(int x,int* cCon){
    // Come da pseudocodice
    if (cCon[x] != x){
        cCon[x] = find(cCon[x],cCon);
        return cCon[x];
    }
    else
        return x;
}

void union_(int u,int v,int *cCon,int* rank){
    // Troviamo radice di u e di v
    int x = find(u,cCon);
    int y = find(v,cCon);

    if (x == y)
        return;  
    
    int z;
    if (rank[x] < rank[y]){ // Ci assicuriamo che x abbia sempre rank > di y
        z = x;
        x = y;
        y = z;
    }

    // X è la nuova radice di y
    cCon[y] = x;

    // Incrementiamo il rank
    if (rank[x] == rank[y])
        rank[x] = rank[x] + 1;
}

void unionFind(arco **hash,int *cCon,int *rank,int hash_size){ // O(a(|E|)), quasi costante
    // Itero la tabella hash
    for (int i=0;i<hash_size;i++)
    {
        arco *curr = hash[i];
        while (curr != NULL) {
            union_(curr->u,curr->v,cCon,rank);
            curr = curr->next;
        }
    }
}

int compare_int(const void* a, const void* b) {
    if ( (*(int *)a) > (*(int *)b) ) return 1;
    if ( (*(int *)a) < (*(int *)b) ) return -1;
    return 0;
}

int compare_archi(const void* a, const void* b) {
    arco* arcoA = *(arco**)a;
    arco* arcoB = *(arco**)b;
    if (arcoA->weight > arcoB->weight) return 1;
    if (arcoA->weight < arcoB->weight) return -1;
    return 0;
}

void makeval(grafo *g, arco *a, bool val) // O(DEGREE) 
{
    //clock_t tt; 
    //tt = clock();

    elemento *curru = g->vicini[a->u];
    while (curru != NULL) {
        if (curru->id == a->v){
            // trovato l'arco
            curru->msf = val;
            break;
        }
        curru = curru->next;
    }

    elemento *currv = g->vicini[a->v];
    while (currv != NULL) {
        if (currv->id == a->u){
            // trovato l'arco
            currv->msf = val;
            break;
        }
        currv = currv->next;
    }
    //float tem = ((double)clock() - tt)/CLOCKS_PER_SEC;
    //printf("parte makeval : %f\n", tem);
    //atomic_fetch_add(&nummakeval,1);
    //atomic_fetch_add(&sumtime,tem);
    //printf("Num: %ld\n",nummakeval);
}

void kruskal(grafo *g, int hash_size) { //O(|E| + |E|log(|E|) + Ea(|V|))
    // Estrazione di tutti gli archi dalla Hash Table
    arco **tutti_gli_archi = malloc(sizeof(arco*) * g->numArchi);
    if(!tutti_gli_archi){fprintf(stderr,"Errore malloc tutti_gli_archi in kruskal\n");exit(1);}
    int contatore = 0;
    for (int i = 0; i < hash_size; i++) {
        arco *curr = g->gHash[i];
        while (curr != NULL) {
            tutti_gli_archi[contatore] = curr;
            contatore++;
            curr = curr->next;
        }
    }

    // Ordinamento degli archi per peso
    qsort(tutti_gli_archi, g->numArchi, sizeof(arco*), compare_archi);

    int *parent = malloc(sizeof(int) * g->nNodi);
    if(!parent){fprintf(stderr,"Errore malloc parent in kruskal\n");exit(1);}

    int *rank = malloc(sizeof(int) * g->nNodi);
    if(!rank){fprintf(stderr,"Errore malloc rank in kruskal\n");exit(1);}

    for (int i = 0; i < g->nNodi; i++) {
        parent[i] = i;
        rank[i] = 1;
    }

    for (int i = 0; i < g->numArchi; i++) {
        arco *a = tutti_gli_archi[i];
        int x = find(a->u, parent);
        int y = find(a->v, parent);

        if (x != y) {
            // L'arco appartiene alla MSF
            a->msf = true;

            // Renderlo true anche nella lista di adiacenza
            makeval(g,a,a->msf);

            g->costoMSF += a->weight;
            union_(a->u, a->v, parent, rank);
        }
    }

    free(tutti_gli_archi);
    free(parent);
    free(rank);
}

void liberahash(grafo* g, int size){ //O(|E|), per forza, non posso usare proprietà ordinate
    for (int i=0;i<size;i++){
        arco *curr = g->gHash[i];
        arco *temp;
        while (curr != NULL) {
            temp = curr;
            curr = curr->next;
            free(temp);
        }
    }
    free(g->gHash);
}

void liberavicini(grafo *g){ //O(|V| + |E|), per forza
    for (int i=0;i<g->nNodi;i++){
        elemento *curr = g->vicini[i];
        elemento *temp;
        while (curr != NULL) {
            temp = curr;
            curr = curr->next;
            free(temp);
        }
    }
    free(g->vicini);
}

arco* findHash(arco **gHash,pthread_mutex_t *mut_gHash,int u,int v,int hash_size,int nummutex){ //O(CHAIN)
    // Per accedere a gHash[i] devi fare il lock del mutex mut_gHash[i%nthread]
    unsigned int idx = hash(u,v,hash_size);
    int pmle = pthread_mutex_lock(&mut_gHash[idx%nummutex]);
    if(pmle != 0){fprintf(stderr,"Errore mutex lock in findHash\n");exit(1);}

    arco *curr = gHash[idx];
    while (curr != NULL) {
        if (curr->u == u && curr->v == v){
            int pmue = pthread_mutex_unlock(&mut_gHash[idx%nummutex]);
            if(pmue != 0){fprintf(stderr,"Errore mutex unlock in findHash\n");exit(1);}
            return curr;
        }
        curr = curr->next;
    }
    int pmue = pthread_mutex_unlock(&mut_gHash[idx%nummutex]);
    if(pmue != 0){fprintf(stderr,"Errore mutex unlock in findHash\n");exit(1);}

    return NULL;
}

void removeHash(grafo *g, arco *a, int hash_size){//O(CHAIN + 2*DEGREE) 
    // Eliminare dalla hash
    // Per accedere a gHash[i] devi fare il lock del mutex mut_gHash[i%nthread]
    unsigned int idx = hash(a->u,a->v,hash_size);
    int pmle = pthread_mutex_lock(&g->mut_gHash[idx%(g->nummutex)]);
    if(pmle != 0){fprintf(stderr,"Errore mutex lock in removeHash\n");exit(1);}

    arco *curr = g->gHash[idx];
    arco *prev = NULL;
    int v = a->v;
    int u = a->u;
    while (curr != NULL) {
        if (curr->u == a->u && curr->v == a->v){

            if(prev != NULL) 
                prev->next = curr->next;
            else{
                g->gHash[idx] = curr->next;
            }
            free(curr); 
            break;
        }
        prev = curr;
        curr = curr->next;
    }


    // Eliminare v dalla lista di adiacenza di u
    elemento *curru = g->vicini[u];
    elemento *prevu = NULL;
    while (curru != NULL) {
        if (curru->id == v){
            curru->msf = false;
            if(prevu != NULL) 
                prevu->next = curru->next;
            else{
                g->vicini[u] = curru->next;
            }
            free(curru); 
            break;
        }
        prevu = curru;
        curru = curru->next;
    }
    
    // Eliminare u dalla lista di adiacenza di v
    elemento *currv = g->vicini[v];
    elemento *prevv = NULL;
   
    while (currv != NULL) {
        if (currv->id == u){
            currv->msf = false;
            if(prevv != NULL) 
                prevv->next = currv->next;
            else{
                g->vicini[v] = currv->next;
            }
            free(currv); 
            break;
        }
        prevv = currv;
        currv = currv->next;
    }

    int pmue = pthread_mutex_unlock(&g->mut_gHash[idx%(g->nummutex)]);
    if(pmue != 0){fprintf(stderr,"Errore mutex unlock in findHash\n");exit(1);}
}

int *bfs(grafo* g,int start,int *len){//O(|E| + |V|)

    // Arr nodi raggiunti
    /*
    *len += 1;
    int *arr = malloc(sizeof(int)*32);
    if(!arr){fprintf(stderr,"Errore mallointc arr in bfs\n");exit(1);}

    int rem = 31; // Rimangono 31 inserzioni
    arr[*len - 1] = start;
    */
    bool *visti = (bool *)calloc(g->nNodi, sizeof(bool));
    if(!visti){fprintf(stderr,"Errore calloc visti in bfs");}
    
    int *q = malloc(sizeof(int) * g->nNodi);
    int sx = 0; // Da cui estriamo nodi
    int dx = 0; // Da cui inseriamo nodi

    visti[start] = true;
    q[dx++] = start; //push(q,start);

    int nodivisti = 1;
    // clock_t tt; 
    // tt = clock();
    while(sx<dx){ //q->next != NULL
        int parent = q[sx ++];//popleft(q);
        //elemento* vicino = g->vicini[parent];

        // Con il for ho visto che è un pò più veloce
        for (elemento *e = g->vicini[parent]; e != NULL; e = e->next) {
            
            if (e->msf && !visti[e->id]) {
                nodivisti++;
                visti[e->id] = true;
                q[dx++] = e->id; 
            }
        }
        
    }
    free(visti);

    // printf("NodiVisti: %d\n",nodivisti);
    // Libero la testa di deque
    // free(q);
    // free(visti);
    // Elimiamo lo scarto che è rimasto
    int *temp = realloc(q,dx * sizeof(int));
    if (temp == NULL){
        fprintf(stderr,"Errore realloc arr\n");
        free(q);
        exit(1);
    }
    
    q = temp;
    *len = dx;
    return q;
}

void *cancella_arco(void *arg){//O( 3(|E| + |V|) +  )

    // Regole di accesso:
    // Per accedere a gHash[i] devi fare il lock del mutex mut_gHash[i%nthread]
    // global mutex + cv + boolean per accedere a quella componenete connessa
    // printf("Inzio exec thread n: %d\n",gettid());
    attr* data = (attr*) arg;
    grafo* grafo = data->g;
    /*
    long *visti = calloc(grafo->nNodi,sizeof(long)); // Dichiaro solo 1 volta
    if(visti == NULL){fprintf(stderr,"Errore malloc visti in cancella_arco\n");exit(1);}
    long gen = 0;
    int *q = malloc(grafo->nNodi*sizeof(int)); // Dichiaro solo 1 volta per thread
    if(q == NULL){fprintf(stderr,"Errore malloc q in cancella_arco\n");exit(1);}
    int numop = 0;
    */
    while(true){
        // u e v presi dal buffer
        int u,v;
        //clock_t t = clock();
        int dfwe = sem_wait(data->full); // Aspetto che ci siano slot liberi
        if(dfwe != 0){fprintf(stderr,"Errore wait data->full in cacella_arco");exit(1);}

        int ptmlde = pthread_mutex_lock(data->m_buffer); // Prendo il mutex per accedere al buffer
        if(ptmlde != 0){fprintf(stderr,"Errore mutex lock data->m_buffer in cancella_arco\n");exit(1);}

        //printf("Leggo buffer exec thread n: %d\n",gettid());
        //numop++;
        u = data->buffer[*(data->idx)]->u;
        v = data->buffer[*(data->idx)]->v;
        free(data->buffer[*(data->idx)]); // Libero questo slot
        *(data->idx) = ((*(data->idx)) + 1 ) % Buf_size; // Incremento l'indice
        

        int ptmude = pthread_mutex_unlock(data->m_buffer);
        if(ptmude != 0){fprintf(stderr,"Errore mutex unlock data->m_buffer in cancella_arco\n");exit(1);}

        int dfpe = sem_post(data->free);
        if(dfpe != 0){fprintf(stderr,"Errore post data->free in cancella_arco\n");exit(1);}

        // printf("Libero buffer exec thread n: %d\n",gettid());
        if(u == -1 || v == -1){ // Finire la consumazione
            break;
        }

        // THREAD: Ci assicuriamo solo per noi la componente connessa
        int ptmlge = pthread_mutex_lock(data->global);
        if(ptmlge != 0){fprintf(stderr,"Errore mutex lock data->global in cancella_arco\n");exit(1);}

        //printf("%d aspetta le cCon[%d] = %d e cCon[%d] = %d\n",gettid(),u,grafo->cCon[u],v,grafo->cCon[v]);
        while(data->bool_cCon[grafo->cCon[u]] || data->bool_cCon[grafo->cCon[v]]){
            int pcwe = pthread_cond_wait(data->cond_cCon,data->global);
            if(pcwe !=0){fprintf(stderr,"Errore cond wait in cancella_arco\n");exit(1);}
        }
        // printf("Mi assicuro la cc_oneessa buffer exec thread n: %d\n",gettid());
        // Segnalo che io ci sto lavorando
        // printf("%d blocca le cCon[%d] = %d e cCon[%d] = %d\n",gettid(),u,grafo->cCon[u],v,grafo->cCon[v]);
        data->bool_cCon[grafo->cCon[u]] = true;
        data->bool_cCon[grafo->cCon[v]] = true;

        int oldcConu = grafo->cCon[u]; 
        int oldcConv = grafo->cCon[v];

        int pcsce = pthread_cond_signal(data->cond_cCon);
        if(pcsce != 0){fprintf(stderr,"Errore signal data->cond_cCon in cancella_arco\n");exit(1);}

        int pmuge = pthread_mutex_unlock(data->global);
        if(pmuge != 0){fprintf(stderr,"Errore unlock data->global in cancella_arco\n");exit(1);}

        // printf("Libero la cc_oneessa buffer exec thread n: %d\n",gettid());
        // printf("Tempo Pre FindHash Arco: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);
        // 6.1) Si elimina l'arco aggiornando gHash e vicini 
        arco* res = findHash(grafo->gHash,grafo->mut_gHash,u,v,data->hash_size,grafo->nummutex); //Non presente in Hash
        if(res==NULL){
            // Stampa risultato
            fprintf(stdout,"- %d %d 0\n",u ,v);

            // THREAD: Liberiamo il mutex 
            int pmldge = pthread_mutex_lock(data->global);
            if(pmldge != 0){fprintf(stderr,"Errore lock data->global in cancella_arco\n");exit(1);}

            data->bool_cCon[oldcConu] = false; // Non ci sto più lavorando
            data->bool_cCon[oldcConv] = false;
            int pcsdce = pthread_cond_signal(data->cond_cCon);
            if(pcsdce != 0){fprintf(stderr,"Errore signal data->cond_cCon in cancella_arco\n");exit(1);}

            int pmudge = pthread_mutex_unlock(data->global);
            if(pmudge != 0){fprintf(stderr,"Errore unlock data->global in cancella_arco\n");exit(1);}
            //printf("Tempo Caso Arco non Esistente: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);
            continue;
        }

        bool check = res->msf;
        int pmlgi = pthread_mutex_lock(data->g_info);
        if(pmlgi != 0){fprintf(stderr,"Errore lock data->g_info in cancella_arco\n");exit(1);}
        grafo->numArchi -= 1;
        int pmugi = pthread_mutex_unlock(data->g_info);
        if(pmugi != 0){fprintf(stderr,"Errore unlock data->g_info in cancella_arco\n");exit(1);}
        // 6.2) leva l'arco e cambia il costo
        int w = res->weight;
        //grafo->costoMSF = check ? grafo->costoMSF - w : grafo->costoMSF;
        res->msf = false;
        //makeval(grafo,res,res->msf); // Caso Pessimo O(n)
        removeHash(grafo,res,data->hash_size);
        
        // Se non c'è check non ha senso fare la BFS!!!!
        if(!check){ // Andiamo alla prossima 

            fprintf(stdout,"- %d %d %d %d %ld\n",u,v,grafo->numArchi,grafo->numCoCo,grafo->costoMSF);

            // THREAD: Liberiamo il mutex 
            int pmldge = pthread_mutex_lock(data->global);
            if(pmldge != 0){fprintf(stderr,"Errore lock data->global in cancella_arco\n");exit(1);}

            data->bool_cCon[oldcConu] = false; // Non ci sto più lavorando
            data->bool_cCon[oldcConv] = false;
            int pcsdce = pthread_cond_signal(data->cond_cCon);
            if(pcsdce != 0){fprintf(stderr,"Errore signal data->cond_cCon in cancella_arco\n");exit(1);}

            int pmudge = pthread_mutex_unlock(data->global);
            if(pmudge != 0){fprintf(stderr,"Errore unlock data->global in cancella_arco\n");exit(1);}

            continue;
        }

        // 6.3) Non si passa per u,v e si elencano i nodi trovati in ognuna delle nuove possibili cCon
        // printf("Tempo Rimozione Arco: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);
        int nU = 0;
        //gen++;
        //clock_t t = clock();
        int* Lu = bfs(grafo,u,&nU);//,data->hash_size,visti,gen,q);
        //printf("Tempo BFS 1: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);
        // printf("\n-----------------\n");
        int nV = 0;
        //gen++;
        //clock_t tt = clock();
        int* Lv = bfs(grafo,v,&nV);//,data->hash_size,visti,gen,q);
        
        //printf("Tempo BFS 2: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);
       
        // Mappa che assegna ad ogni nodo la sua area di apparteneza
        // 2 alla cCon raggiungibile da v
        // 1 alla cCon raggiungibile da u
        // 0 altrimenti

        //clock_t tt; 
        //tt = clock();

        int *map = calloc(grafo->nNodi,sizeof(int));
        // Fai che i valori aumentano in POS e NEG
        if(!map){fprintf(stderr,"Errore calloc map in cancella_arco\n");exit(1);}
        for(int i=0;i<nU;i++)
            map[Lu[i]] = 1;
        
        for(int i=0;i<nV;i++)
            map[Lv[i]] = 2;
        
        // 6.4) Si trova l'arco di costo minimo che connette cCon_u a cCon_v, e lo si mette nel MSF
        arco *minU_V = NULL;

        // Vediamo di ridurre anche qui l'uso di findhash
        int minu = -1;
        int minv = -1;
        int minw = -1;
        
        
        // O(|V| + |E|)
        if (nU<nV){ // Un pò di pruning che va bene quando sia assume che il numero di nodi
            // Proporzionale al numero di connessioni, non sempre
            for(int i=0; i<nU; i++){
                // printf("Lu[%d]: %d\n",i,Lu[i]);
                elemento *curr = grafo->vicini[Lu[i]]; //FIX: Vedi quanto lunga è, salvare gli archi possibili durante BFS
                while (curr != NULL) {

                    // Se l'id appartiene a Lv e l'arco è conveniente
                    if(map[curr->id] == 2 && (minu == -1 || minw > curr->w) ){ 
                        minu = Lu[i];
                        minv = curr->id;
                        minw = curr->w;
                    }

                    curr = curr -> next;
                }
            }
        }
        else{
            for(int i=0; i<nV; i++){
                // printf("Lv[%d]: %d\n",i,Lu[i]);
                elemento *curr = grafo->vicini[Lv[i]];
                while (curr != NULL) {

                    // Se l'id appartiene a Lv e l'arco è conveniente
                    if(map[curr->id] == 1 && (minu == -1 || minw > curr->w) ){ 
                        minu = Lv[i];
                        minv = curr->id;
                        minw = curr->w;
                    }

                    curr = curr -> next;
                }
            }
        }
        
        // Sfruttiamo il fatto che in hash ho solo archi con u < v
        // Non è molto necessario, giusto per completezza
        if(minu < minv)
            minU_V = findHash(grafo->gHash,grafo->mut_gHash,minu,minv,data->hash_size,grafo->nummutex);
        else
            minU_V = findHash(grafo->gHash,grafo->mut_gHash,minv,minu,data->hash_size,grafo->nummutex);

        //printf("Tempo Calcolo Trovare Arco: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);

        // 6.5) Se non esiste renderle 2 cCon diverse, e aggiornare il count di cCon 
        if(minU_V == NULL){ //&& minV_U == NULL){
            // printf("here\n");
            // Logica
            int minLu = -1;
            int minLv = -1;
            

            // Ottimizziamo ricerca cCon, così che non deve vedere tutti i nodi ogni volta
            
            for(int i=0;i<nU;i++){
                minLu = minLu == -1 ? Lu[i] : fmin(minLu,Lu[i]);
            }
            for(int i=0;i<nV;i++){
                minLv = minLv == -1 ? Lv[i] : fmin(minLv,Lv[i]);
            }
            
            // Da proteggere con il mutex global, per assicurarci che
            // Nessun nuovo thread possa accedere ai nuovi archi marcati con una nuova cCon idx,
            // E quindi ci possa accedere e modificare, anche i loro cCon, rischiando,
            // Di spezzare l'array di cCon tra index che stavano venendo aggiornati e quelli aggiornati
            int pmldgc = pthread_mutex_lock(data->global);
            if (pmldgc != 0){fprintf(stderr,"Errore lock data->global dopo minLu in cancella_arco");}
            for(int i=0;i<nU;i++){
                grafo->cCon[Lu[i]] = minLu;
            }

            for(int i=0;i<nV;i++){
                grafo->cCon[Lv[i]] = minLv;
            }
            int pmudgc = pthread_mutex_unlock(data->global);
            if (pmudgc != 0){fprintf(stderr,"Errore unlock data->global dopo minLu in cancella_arco");}
            
            int pmlgi = pthread_mutex_lock(data->g_info);
            if(pmlgi != 0){fprintf(stderr,"Errore lock data->g_info in cancella_arco\n");exit(1);}
            grafo->numCoCo += 1;
            int pmugi = pthread_mutex_unlock(data->g_info);
            if(pmugi != 0){fprintf(stderr,"Errore unlock data->g_info in cancella_arco\n");exit(1);}
            //printf("Tempo Calcolo Nuove cCon: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);
        }
        else{
            minU_V->msf = true;
            makeval(grafo,minU_V,minU_V->msf);

            int pmlgi = pthread_mutex_lock(data->g_info);
            if(pmlgi != 0){fprintf(stderr,"Errore lock data->g_info in cancella_arco\n");exit(1);}

            grafo->costoMSF += minU_V->weight;

            int pmugi = pthread_mutex_unlock(data->g_info);
            if(pmugi != 0){fprintf(stderr,"Errore unlock data->g_info in cancella_arco\n");exit(1);}
            //printf("Tempo Aggiornare Costo: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);
        }
        
        // 6.6) Finire di aggiornare il costo
        int pmlgi2 = pthread_mutex_lock(data->g_info);
        if(pmlgi2 != 0){fprintf(stderr,"Errore lock data->g_info in cancella_arco\n");exit(1);}

        grafo->costoMSF = grafo->costoMSF - w; 

        int pmugi2 = pthread_mutex_unlock(data->g_info);
        if(pmugi2 != 0){fprintf(stderr,"Errore unlock data->g_info in cancella_arco\n");exit(1);}

        free(map);
        free(Lu);
        free(Lv);

        // Stampa risultato
        fprintf(stdout,"- %d %d %d %d %ld\n",u,v,grafo->numArchi,grafo->numCoCo,grafo->costoMSF);

        // THREAD: Liberiamo il mutex 
        int pmldge = pthread_mutex_lock(data->global);
        if(pmldge != 0){fprintf(stderr,"Errore lock data->global in cancella_arco\n");exit(1);}

        data->bool_cCon[oldcConu] = false; // Non ci sto più lavorando
        data->bool_cCon[oldcConv] = false;
        //printf("%d sblocca le cCon[%d] = %d e cCon[%d] = %d\n",gettid(),u,oldcConu,v,oldcConv);
        int pcsdce = pthread_cond_signal(data->cond_cCon);
        if(pcsdce != 0){fprintf(stderr,"Errore signal data->cond_cCon in cancella_arco\n");exit(1);}

        int pmudge = pthread_mutex_unlock(data->global);
        if(pmudge != 0){fprintf(stderr,"Errore unlock data->global in cancella_arco\n");exit(1);}

        //float tem = ((double)clock() - tt)/CLOCKS_PER_SEC;
        //printf("tempo  post-bfs cancella_arco: %f", tem);
        //printf("Tempo Post BFS: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);
        //printf("Fine OP thread n: %d\n",gettid());
        //sleep(0.1);
        continue;
    }
    //free(q);
    //free(visti);
    //printf("------------------ Thread: %d, Num Op: %d ------------------\n",gettid(),numop);
    return NULL;
}

void hashanalisi(arco** gHash,int hash_size,int* nonvuote,float* media,int *massima){
    // Scorro tutta la tabella hash
    int somma = 0; //Somma lunghezza
    for (int i=0;i<hash_size;i++){
        arco *curr = gHash[i];
        int temp = 0;
        if (curr != NULL){
            (*nonvuote) ++;
        }
        while(curr != NULL){
            temp ++;
            curr = curr->next;
        }

        somma += temp;
        *massima = temp > *massima ? temp : *massima;
    }

    *media = (somma)/(*nonvuote);
}

void calcolofinale(grafo* g,int hash_size,int *numArchi,int *numcCon,long *costoMSF){
    // Calcoliamo il numArchi e il costo della MSF
    for (int i=0;i<hash_size;i++){
        arco *curr = g->gHash[i];
        while(curr != NULL){
            (*numArchi) ++;
            (*costoMSF) += curr->msf ? curr->weight : 0;
            curr = curr->next;
        }
    }

    // Calcoliamo il num di cCon
    int *to_remove = malloc(sizeof(int)*g->nNodi);
    if(!to_remove){fprintf(stderr,"Errore malloc to_remove in calcolofinale\n");exit(1);}
    for (int i=0;i<g->nNodi;i++)
        to_remove[i] = g->cCon[i];
    qsort(to_remove,g->nNodi,sizeof(int),compare_int); // Ordiniamo

    int prev = -1;
    for (int i=0;i<g->nNodi;i++){
        if (prev!=to_remove[i]){
            prev = to_remove[i];
            (*numcCon)++;
        }
    }
    free(to_remove);
}