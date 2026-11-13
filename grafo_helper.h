#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> // Per getopt

#define Buf_size 100
//#define P1 11
//#define P2 17
//#define P3 73244475 // Un numero molto grande che è stato visto cambiare molto i bit quando usato per moltiplicare
#define P4 0x9e3779b9 // Anche questo numero particolare

// Strutture per gli archi
typedef struct arco {
    int u, v;
    int weight;
    bool msf;
    struct arco* next;
} arco;

// Struttura per un elemento della lista di adiacenza
typedef struct elemento{
    int id;   // indice del nodo       
    int w;    // peso 
    bool msf; // true se questo arco appartiene alla MSF
    struct elemento* next;
} elemento;

// Struttura usata per descrivere la foresta
typedef struct {
    arco** gHash;       // tabella hash (array di liste di archi)
    elemento** vicini;  // array di liste di adiacenza
    int* cCon;          // array delle componenti connesse
    long costoMSF;       // costo della MSF 
    int numCoCo;        // numero di componente connesse

    // Altri campi 
    int numArchi;       // numero totale di archi nel grafo
    int nNodi;          // numero totale di nodi
    int nummutex;      // numero totale di mutex assegnabili ad ogni zona
    pthread_mutex_t* mut_gHash; // array di mutex per la hash table
} grafo;

// Struttura usata l'ordinamento della lista
typedef struct {
    int id;
    int w;
    bool msf;
} temp_elemento;

typedef struct deque{
    int val;
    struct deque* next; 
    struct deque* last; // Punta all'ultimo elemento, usata solo dal primo elemento della deque
} deque;

typedef struct{
    int u;
    int v;
} operazione;

typedef struct {
    pthread_mutex_t* mut_gHash; // array di mutex per la hash table
    bool* bool_cCon;  // array di boolean per le componenti connesse

    pthread_mutex_t* global; //Mutex globale per accedere a bool_cCon;
    pthread_cond_t* cond_cCon; // conditional variable 
    pthread_mutex_t* g_info; //Mutex global per accedere e modificare le info del grafo

    int* idx; // Indice del buffer consumatori
    sem_t* free; // Usato nel buffer prod consummatori
    sem_t* full; // Usato nel buffer prod consummatori

    grafo* g;
    operazione** buffer; //Array di puntatori di buffer
    pthread_mutex_t* m_buffer; // Mutex del buffer

    int hash_size;
}attr;




// Funzioni
void stampa_grafo(grafo* g, int h_size);
unsigned int hash(int u, int v, int size);
void insert_hash(int i,int u,int v,int w,arco** gHash);
void insert_vicini(int u,int v,int w,elemento** vicini);
int compare_temp(const void* a, const void* b);
void ordina_lista(elemento* vicini);
int find(int x,int* cCon);
void union_(int u,int v,int* cCon,int* rank);
void unionFind(arco** hash,int* cCon,int* rank,int hash_size);
int compare_int(const void* a, const void* b);
int compare_archi(const void* a, const void* b);
void makeval(grafo *g, arco *a, bool val);
void kruskal(grafo *g, int hash_size);
void liberahash(grafo* g, int size);
void liberavicini(grafo *g);
arco* findHash(arco** gHash,pthread_mutex_t* mut_gHash,int u,int v,int hash_size,int nummutex);
void removeHash(grafo* g, arco* a, int hash_size);
deque *deque_init();
void push(deque* q,int id);
int popleft(deque* q);
//int *bfs(grafo* g,int start,int* len,int size,long* visti,long gen,int* q);
int *bfs(grafo* g,int start,int *len);
void *cancella_arco(void* arg);
void hashanalisi(arco** gHash,int hash_size,int* nonvuote,float* media,int *massima);
void calcolofinale(grafo* g,int hash_size,int *numArchi,int *numcCon,long *costoMSF);