#include "grafo_helper.h"
/*

Inizio progetto

*/

// Stampare tutti gli errori o debug su stderr
// Lasciare stdout libero solo per le informazioni
// Implementazione mutex: todo
int main(int argc,char *argv[]){
    //clock_t t; 
    //t = clock(); 
    //clock_t prima = clock();
    
    // Valori di default
    int num_threads = 3;
    int hash_size = 100000;
    int num_mutex = 1000;
    char *file_grafo = NULL;
    char *file_archi = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "t:H:M:")) != -1) {
        switch (opt) {
            case 't':
                num_threads = atoi(optarg);
                break;
            case 'H':
                hash_size = atoi(optarg);
                break;
            case 'M':
                num_mutex = atoi(optarg);
                break;
            case '?': // Errore dell'input
                fprintf(stderr, "Uso: %s file_grafo file_archi [-t threads] [-H hashsize] [-M nmutex]\n", argv[0]);
                return 1;
            default: // Altro caso, in teoria non viene mai chiamato
                fprintf(stderr,"Errore non specificato in opt\n");
                return 1;
        }
    }

    // Dopo il ciclo getopt, gli argomenti posizionali si trovano a partire da optind
    if (argc - optind != 2) {
        fprintf(stderr, "Uso: %s file_grafo file_archi [-t threads] [-H hashsize] [-M nmutex]\n", argv[0]);
        return 1;
    }

    file_grafo = argv[optind];
    file_archi = argv[optind + 1];

    // DEBUGG 
    /*
    fprintf(
        stdout,"file_grafo: %s, file_archi %s, %d [-t threads], %d [-H hashsize], %d [-M nmutex]\n",
        file_grafo,file_archi,num_threads,hash_size,num_mutex
    );
    */
    // Inizializzazione strutture:

    grafo *g = malloc(sizeof(grafo));
    if(!g){fprintf(stderr,"Errore malloc grafo in main");return 1;}

    g->gHash = malloc(sizeof(arco *)*hash_size);
    if(!(g->gHash)) {fprintf(stderr,"Errore malloc Hash\n");return 1;}
    for(int i=0;i<hash_size;i++)
        g->gHash[i] = NULL; // Per eliminare i valori già presenti in mem
    g->vicini = NULL; // Allocare durante esplorazione file
    g->cCon = NULL; //Allocare dopo esplorazione file
    g->costoMSF = 0;
    g->numCoCo = 0;
    g->numArchi = 0; //Così che il compiler non mi da fastidio
    g->nNodi = 0; //Così che il compiler non mi da fastidio
    
    // Creazione grafo:
    // 0) Lettura file di testo 
    // 0.1) Aprire il file e leggerlo
    FILE *f = fopen(file_grafo,"r");
    if(f==NULL){fprintf(stderr,"Errore apertura file_grafo\n");return 1;}
    char *buffer=NULL;    
    size_t n=0;          
    while(getline(&buffer,&n,f)!=-1) {
        
        // esegue la tokenizzazione di buffer: legge primo token 
        char *s = strtok(buffer," \n");

        if (s == NULL) continue;

        //printf("ATTUALE: %s\n",s);
        if (!strcmp(s,"p")) // ha il numero di nodi e di archi
        {
        
            s = strtok(NULL," \n"); // Saltiamo sp
            
            s = strtok(NULL," \n"); // Numero di nodi
            g->nNodi = atoi(s) + 1;
            g->vicini = malloc(sizeof(elemento *)*(g->nNodi));
            if(!g->vicini){fprintf(stderr,"Errore malloc lista di adiacenza\n");return 1;}
            
            for(int i=0;i<g->nNodi;i++)
                g->vicini[i] = NULL; 


            s = strtok(NULL," \n"); // Numero di archi
            g->numArchi = atoi(s);
        }
        else if(!strcmp(s,"a")){ // lettura di un arco
            
            s = strtok(NULL, " \n"); 
            int u = atoi(s);
            
            s = strtok(NULL, " \n"); 
            int v = atoi(s);

            s = strtok(NULL, " \n"); 
            int w = atoi(s);
            
            //printf("%d %d %d\n",u,v,w);
            // 1) L'insieme degli archi
            // 1.2) Convertire ogni coppia ad un valore 
            unsigned int idx = hash(u,v,hash_size);

            // 1.3) Metterlo nella hash con chaining
            insert_hash(idx,u,v,w,g->gHash);


            // 2) Le liste di adiacenza dei singoli nodi
            // 2.1) Inserire i nella lista di adiacenza
            insert_vicini(u,v,w,g->vicini);
        }
        else{// Commento o qualcos'altro
            continue; //Passiamo alla prossima stringa
        }
            
    }

    //printf("Tempo primo file: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);

    // 2.2) Ordinare la lista
    for (int i=0;i<g->nNodi;i++){
        ordina_lista(g->vicini[i]);
    }

    //printf("Tempo Ordina Lista: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);

    free(buffer);
    int fce = fclose(f);
    if(fce == EOF){fprintf(stderr,"Errore fclose file_grafo\n");return 1;}
    
    // 3) La tabella delle componenti connesse.
    // 3.1) Inizializzare l'array dei nodi 
    g->cCon = malloc(sizeof(int)*g->nNodi);
    if(!g->cCon){fprintf(stderr,"Errore nella malloc cCon\n");return 1;}
    for (int i=0;i<g->nNodi;i++)
        g->cCon[i] = i; // All'inizio ogni nodo è genitore di se stesso

    // 3.2) Attuare una UnionFind sugli archi
    int *rank = malloc(sizeof(int)*g->nNodi);
    if(!rank){fprintf(stderr,"Errore nella malloc per l'array rank\n");return 1;}
    for (int i=0;i<g->nNodi;i++)
        rank[i] = 1; // All'inizio ogni nodo ha rank 1
    unionFind(g->gHash,g->cCon,rank,hash_size);

    // 3.3) Iteriamo ogni nodo e troviamo la sua radice, 
    // teniamo traccia del rank minore associato ad essa
    int *root_min = malloc(sizeof(int)*g->nNodi);
    if(!root_min){fprintf(stderr,"Errore nella malloc per l'array root_min\n");return 1;}
    for (int i=0;i<g->nNodi;i++)
        root_min[i] = -1; // Inizializziamo a -1
        
    for (int i=0;i<g->nNodi;i++){
        // è necessario un find intermedio nel caso grafo.cCon non punti
        // alla radice ma ad un nodo
        int root = find(i, g->cCon); // Radice elemento i 
        if(root_min[root] == -1)
            root_min[root] = i; // Inizializzazione
        else
            root_min[root] = root_min[root]<i? root_min[root]:i;
    }

    // 3.4) Assegnamo ad ogni elemento in cCon il val minimo associato alla sua radice
    for (int i=0;i<g->nNodi;i++)
        g->cCon[i] = root_min[g->cCon[i]];

    // 3.5) Contiamo il numero di componenti connesse 
    int *to_remove = calloc(g->nNodi,sizeof(int));
    if (!to_remove){fprintf(stderr,"Errore calloc to_remove\n");return 1;}

    for (int i=0;i<g->nNodi;i++)
        to_remove[i] = g->cCon[i];
    qsort(to_remove,g->nNodi,sizeof(int),compare_int); // Ordiniamo
    
    // Miglioriamo ancora le op riguardanti le cCon, anche se non so l'ordinamento
    // Quando possa impattare
    int prev = -1;
    for (int i=0;i<g->nNodi;i++){
        if (to_remove[i] != prev){ // Se ancora non ho visto questa cCon
            prev = to_remove[i];
            g->numCoCo ++;
        }
    }

    //printf("Tempo Union Find: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);

    free(to_remove);
    

    // 3.6) Attuare algoritmo MSF, usando Kruskal
    kruskal(g,hash_size);

    //float tem = ((double)clock() - prima)/CLOCKS_PER_SEC;
    //printf("Tempo prima parte: %f\n",tem);
   

    // Stampa valori del grafo 
    fprintf(stdout,"%d %d %ld\n",g->numArchi,g->numCoCo,g->costoMSF);
    
    //printf("Tempo Kruskal Prima Parte: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);
    
    // ----------------------------- // Inizio Parte 2
    
    // 4.0) inizializzazione buffer reader writer e strutture per il multi-threading
    
    /*
    grafo.nmutex = num_mutex;
    grafo.bool_cCon = malloc(sizeof(bool)*num_mutex);
    grafo.mut_gHash = malloc(sizeof(pthread_mutex_t)*num_mutex);
    for (int i=0;i<num_mutex;i++){
        pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
        grafo.mut_gHash[i] = m1;
        grafo.bool_cCon[i] = false;
    }
    */
    g->nummutex = num_mutex;

    int idx = 0;
    operazione *buff[Buf_size];

    // Inizializzo l'array degli attribbuti
    attr *cons_atr = malloc(sizeof(attr)*num_threads);
    if(!cons_atr){fprintf(stderr,"Errore malloc cons_atr\n");return 1;}

    // Inizializzazione array ausialiari
    bool *bool_cCon = malloc(sizeof(bool)*g->nNodi);
    if(!bool_cCon){fprintf(stderr,"Errore malloc bool_cCon\n");return 1;}
    for (int i=0;i<g->nNodi;i++){
        bool_cCon[i] = false;
    }
    pthread_mutex_t *mut_gHash = malloc(sizeof(pthread_mutex_t)*num_mutex);
    if(!mut_gHash){fprintf(stderr,"Errore malloc mut_gHash\n");return 1;}

    pthread_cond_t *cond_cCon = malloc(sizeof(pthread_cond_t));
    if(!cond_cCon){fprintf(stderr,"Errore malloc cond_cCon\n");return 1;}
    int pcice = pthread_cond_init(cond_cCon,NULL);
    if(pcice != 0){fprintf(stderr,"Errore inizializzazione cond_cCon in main\n");exit(1);}
    for (int i=0;i<num_mutex;i++){
        pthread_mutex_init(&mut_gHash[i],NULL);
        //pthread_cond_init(&cond_cCon[i],NULL);
    }
    g->mut_gHash = mut_gHash;

    // Semafori per il buffer e mutex globale per bool_cCon
    sem_t *free_sem = malloc(sizeof(sem_t));
    if(!free_sem){fprintf(stderr,"Errore malloc free_sem in main\n");exit(1);}

    sem_t *full_sem = malloc(sizeof(sem_t));
    if(!full_sem){fprintf(stderr,"Errore malloc full_sem in main\n");exit(1);}


    int fre = sem_init(free_sem,0,Buf_size);
    if (fre==-1){fprintf(stderr,"Errore creazione free_sem\n");return 1;}

    int fue = sem_init(full_sem,0,0);
    if (fue==-1){fprintf(stderr,"Errore creazione full_sem\n");return 1;}

    pthread_mutex_t *m_buff = malloc(sizeof(pthread_mutex_t));
    if(!m_buff){fprintf(stderr,"Errore malloc m_buff in main");return 1;}
    pthread_mutex_init(m_buff,NULL);

    pthread_mutex_t *global = malloc(sizeof(pthread_mutex_t));
    if(!global){fprintf(stderr,"Errore malloc global in main");return 1;}
    pthread_mutex_init(global,NULL);

    pthread_mutex_t *g_info = malloc(sizeof(pthread_mutex_t));
    if(!global){fprintf(stderr,"Errore malloc g_info in main");return 1;}
    pthread_mutex_init(g_info,NULL);

    int idx_cons = 0;
    // 4.0.1) Far partire i thread consumatori, i quali implementeranno la logica del while
    pthread_t *consumatori = malloc(sizeof(pthread_t)*num_threads); 
    if(!consumatori){fprintf(stderr,"Errore malloc consumatori\n");return 1;}
    for(int i=0;i<num_threads;i++){
        cons_atr[i].bool_cCon = bool_cCon;
        cons_atr[i].buffer = buff;
        cons_atr[i].cond_cCon = cond_cCon;
        cons_atr[i].free = free_sem;
        cons_atr[i].full = full_sem;
        cons_atr[i].g = g; // Necessario per accedere ai dati del grafo
        cons_atr[i].global = global;
        cons_atr[i].g_info = g_info;
        cons_atr[i].idx = &idx_cons;
        cons_atr[i].mut_gHash = mut_gHash;
        cons_atr[i].m_buffer = m_buff;
        cons_atr[i].hash_size = hash_size;
        int te = pthread_create(&consumatori[i],NULL,&cancella_arco,&cons_atr[i]);
        if(te!=0){fprintf(stderr,"Errore creazione thread consumatore\n");return 1;}
    }
    
    //printf("Tempo Inizializzazione Thread: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);
    // clock_t tt = clock();
    // Operazioni sul grafo
    // 4) Lettura file testo delle operazioni
    FILE *f2 = fopen(file_archi,"r");
    if(f2 == NULL){fprintf(stderr,"Errore apertura file_archi\n");return 1;}

    char *buffer2=NULL;    
    size_t n2=0;  
    // 4.1) Interpretare le righe e fare la corretta operazione        
    while(getline(&buffer2,&n2,f2)!=-1) {
        
        int frw = sem_wait(free_sem); // Aspettiamo che ci siano slot liberi
        if(frw == -1){fprintf(stderr,"Errore wait free_sem in main\n");return 1;}

        char *s = strtok(buffer2," \n");
        if (s == NULL){
            int frp = sem_post(free_sem); // Liberiamo lo slot
            if(frp == -1){fprintf(stderr,"Errore post free_sem in main\n");return 1;}

            continue;
        }
        else if(!strcmp(s,"-")){ // Cancellazione
            // 6) Cancellazione
            s = strtok(NULL, " \n"); 
            int u = atoi(s);
            
            s = strtok(NULL, " \n"); 
            int v = atoi(s);

            // Scrivere nel buffer 
            //pthread_mutex_lock(&m_buff);
            operazione* el = malloc(sizeof(operazione));
            if(!el){fprintf(stderr,"Errore malloc el\n");return 1;}
            el->u = u;
            el->v = v;
            buff[idx] = el;
            idx = (idx + 1)%Buf_size;
            //pthread_mutex_unlock(&m_buff);

            int flp = sem_post(full_sem); // Segnalo che ho slot pronto
            if(flp == -1){fprintf(stderr,"Errore post full_sem in main\n");return 1;}
        }
        else if(!strcmp(s,"+")){ // Aggiunta, da ignorare
            s = strtok(NULL, " \n"); 
            int u = atoi(s);
            
            s = strtok(NULL, " \n"); 
            int v = atoi(s);

            s = strtok(NULL, " \n");
            int w = atoi(s);

            fprintf(stdout,"+ %d %d %d 0\n",u,v,w);
            int frp = sem_post(free_sem); // Liberiamo lo slot
            if(frp == -1){fprintf(stderr,"Errore post free_sem in main\n");return 1;}
            continue;
        }
        else{ // Qualche altra op illegale
            int frp = sem_post(free_sem); // Liberiamo lo slot
            if(frp == -1){fprintf(stderr,"Errore post free_sem in main\n");return 1;}
            continue;
        }
    }
    int fce2 = fclose(f2);
    if(fce2 == EOF){fprintf(stderr,"Errore fclose file_archi\n");return 1;}
    free(buffer2);

    //printf("Tempo Op Cancellazione: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);

    // Segnalo che i consumatori possono terminare
    for (int i=0;i<num_threads;i++){
        int frw = sem_wait(free_sem);
        if(frw == -1){fprintf(stderr,"Errore wait free_sem in main inserimento elemento che segnala la terminazione\n");return 1;}

        operazione *el = malloc(sizeof(operazione));
        if(!el){fprintf(stderr,"Errore malloc el\n");return 1;}
        el->u = -1;
        el->v = -1;
        buff[idx] = el;
        idx = (idx + 1) % Buf_size;

        int fup = sem_post(full_sem);
        if(fup == -1){fprintf(stderr,"Errore post full_sem in main inserimento elemento che segnala la terminazione\n");return 1;}
    }

    //printf("Tempo Terminazione: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);

    // Aspetto che gli altri thread terminino
    for(int i=0;i<num_threads;i++){
        int ptje = pthread_join(consumatori[i],NULL);
        if(ptje != 0){fprintf(stderr,"Errore join in main\n");return 1;}
        //printf("HERE finito t: %d\n",i);
    }
    //printf("Tempo Op: %f\n", ((double)clock() - tt)/CLOCKS_PER_SEC);

    //printf("Tempo Join: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);

    // Al termine delle operazioni dei thread calcolare i valori della tabella hash
    // E ricalcolare i 3 valori del grafo visitato le strutture dati
    int nonvuote = 0;
    float media = 0;
    int massima = 0;
    fprintf(stdout,"Operazioni terminate\n");
    hashanalisi(g->gHash,hash_size,&nonvuote,&media,&massima);
    fprintf(stdout,"Numero posizioni non vuote: %d\n",nonvuote);
    fprintf(stdout,"Lunghezza media liste: %f\n",media);
    fprintf(stdout,"Lunghezza massima liste: %d\n",massima);

    // Ricalcoliamo il num di archi, num di cCon e costo MSF
    int numArchi = 0;
    int numcCon = 0;
    long costoMSF = 0;
    calcolofinale(g,hash_size,&numArchi,&numcCon,&costoMSF);
    fprintf(stdout,"%d %d %ld\n",numArchi,numcCon,costoMSF);


    // Deallocazion
    free(g->cCon); 
    liberahash(g,hash_size);
    liberavicini(g);
    free(rank);
    free(root_min);

    int sdfue = sem_destroy(full_sem);
    if(sdfue != 0){fprintf(stderr,"Errore destroy full_sem\n");exit(1);}
    int sdfre = sem_destroy(free_sem);
    if(sdfre != 0){fprintf(stderr,"Errore destroy free_sem\n");exit(1);}

    free(free_sem);
    free(full_sem);
    free(bool_cCon);

    int pmdge = pthread_mutex_destroy(global);
    if(pmdge != 0){fprintf(stderr,"Errore destroy global mutex\n");exit(1);}
    free(global);
    int pmdgd = pthread_mutex_destroy(g_info);
    if(pmdgd != 0){fprintf(stderr,"Errore destroy g_info mutex\n");exit(1);}
    free(g_info);
    int pmdme = pthread_mutex_destroy(m_buff);
    if(pmdme != 0){fprintf(stderr,"Errore destroy m_buff mutex\n");exit(1);}
    free(m_buff);

    for (int i=0; i<num_mutex; i++){
        int pmdge = pthread_mutex_destroy(&mut_gHash[i]);
        if (pmdge != 0){fprintf(stderr,"Errore destroy mut_gHash[%d] mutex\n",i);exit(1);}
    }
    free(mut_gHash); 

    /*
    for (int i=0; i<num_mutex; i++){
        pthread_cond_destroy(&cond_cCon[i]);
    }
    */
    int pcdce= pthread_cond_destroy(cond_cCon);
    if(pcdce != 0){fprintf(stderr,"Errore destroy cond_cCon\n");exit(1);}
    free(cond_cCon); 

    free(g);
    free(consumatori);
    free(cons_atr);

    //t = clock() - t; 
    //double time_taken = ((double)t)/CLOCKS_PER_SEC; 

    // printf("ci sono voluti %f per exec il main \n", time_taken);
    //printf("Tempo Totale: %f\n", ((double)clock() - t)/CLOCKS_PER_SEC);
    return 0;
}