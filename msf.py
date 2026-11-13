
import sys
import time
import heapq # è un heap così che il file USA-NW non mi faccia aspettare troppo

# Implementiamo heapq in modo nativo

def main(argomenti):
    def find(u):
        if (u != cCon[u]):
            cCon[u] = find(cCon[u])
            return cCon[u]
        return u
    
    def union(u,v):
        x = find(u)
        y = find(v)
        
        if x == y:
            return

        # Così che x ha sempre il rank >=
        if rank[x] < rank[y]:
            x,y = y,x
        
        cCon[y] = x

        if rank[x] == rank[y]:
            rank[x] += 1
    
    def prim(start,connessi):
        '''
        idxcCon = cCon[start] # Componente connesse

        nonesplorati = set()
        for el in cCon_map[idxcCon]:
            nonesplorati.add(el)
            seen[el] = True # Necessario per il for fuori
        '''
        peso = {i: float("inf") for i in connessi}
        #minarco = [None] * n
        peso[start] = 0 # Partiamo da start
        nonesplorati = set(connessi) # Set elementi ancora non estratti

        # Invece di eplorati ho la priority queu
        # Che gestisco con l'heap di python
        h = [(0,start)]#MinHeap([(0,start)])

        #print("Tempo pre Heap: \n",time.time()-start_time)
        while len(h):
            # Vediamo di usare heapq
            _, curr = heapq.heappop(h)

            # Il nodo potrebbe esser già stato rimosso da nonesplorati 
            # Nel caso adesso io stia processando coppie peso,id con peso maggiore 
            if curr not in nonesplorati:
                continue

            nonesplorati.remove(curr)
            #print("A: \n",grafo[curr])
            for ngb,w in grafo[curr].items():
                if ngb in nonesplorati and w < peso[ngb]:
                    peso[ngb] = w
                    #minarco[ngb] = w

                    # Aggiungo nell'heap (vecchio esplorati add)
                    heapq.heappush(h,(w,ngb)) #h.insert((w, ngb))
        #print("Tempo post Heap: \n",time.time()-start_time)
        #temp = [minarco[i] for i in range(n) if minarco[i] != None]
        return sum(peso.values())

    start_time = time.time()

    # Inizializzazione elementi
    archi = dict()
    cCon = [] #Da fare durante la lettura del file

    file_grafo = argomenti[0]
    file_archi = argomenti[1]
    n = 0
    # 0) Lettura file di testo 
    # 0.1) Aprire il file e creare la hash degli archi e dei nodi
    with open(file_grafo,"r") as f:
        for line in f:
            l = line.split(" ")
            if l[0] == 'p':
                data = [int(el) for el in l[2:] if el.strip()]
                if len(data)!=2:
                    raise RuntimeError("Lettura non andata a buon fine nel file_grafo per i requisiti")
                n = int(data[0]) + 1
                cCon = [i for i in range(n)]
                #print("INFO")
            elif l[0] == 'a':
                #print("ADD\n")
                data = [int(el) for el in l[1:] if el.strip()]
                #print("DATA: ", data)
                if len(data)!=3:
                    raise RuntimeError("Lettura non andata a buon fine nel file_grafo per l'arco")
                archi[ (int(data[0]), int(data[1])) ] = int(data[2])    
    
    #print("Tempo Primo File:\n",time.time() - start_time)

    # Creazione del grafo
    grafo = {i:{} for i in range(n)}
    for (u,v),w in archi.items():
        grafo[u][v] = w
        grafo[v][u] = w

    # print("Grafo: ",grafo)
    #print("Tempo Prima parte:\n",time.time() - start_time)
    # Leggere le operazioni del secondo file

    # ------- Parte 2 ----------
    with open(file_archi,"r") as f:
        for line in f:
            l = line.split(" ")
            if l[0] == '+': # Aggiunta
                data = [int(el) for el in l[1:] if el.strip()]
                
                if len(data) != 3: 
                    raise RuntimeError("Lettura non andata a buon fine per l'operazione di aggiunta")
                u,v,w = data[0], data[1], data[2]

                # Vuol dire che stiamo aggiungendo un arco con nodi non nel grafo
                if(u not in grafo or v not in grafo): continue

                archi[(u,v)] = w
                grafo[u][v] = w
                grafo[v][u] = w
            elif l[0] == '-': # Sottrazione
                
                data = [int(el) for el in l[1:] if el.strip()]
                
                u,v = data[0], data[1]
                if len(data) != 2: 
                    raise RuntimeError("Lettura non andata a buon fine per l'operazione di rimozione")
                
                if ((u,v) not in archi): continue
                #print(archi,u,v)
                #print(grafo)
                del grafo[u][v]
                del grafo[v][u] 
                del archi[(u,v)]
            
    #print("Tempo lettura secondo file:\n",time.time() - start_time)

    # Attuare union find sui nodi
    rank = [1]*n
    for u,v in archi:
        union(u,v)
    
    min_root = [float("inf")]*n # Mi associa ad ogni radice i, il minimo che ce l'ha come radice

    for i in range(n):
        root = find(i) #Nel caso siamo fermi a puntare ad un nodo intermedio
        min_root[root] = min(min_root[root],i)
    
    # Dopo aver calcolato il valore minimo per ogni radice, lo riassegno a cCon[i]
    for i in range(n):
        cCon[i] = min_root[cCon[i]]


    #print("Tempo Union Find: \n",time.time() - start_time)

    # Per il calcolo del MSF devo Usare Prim
    # seen = [False for i in range(n)]

    # Per velocizzare
    cCon_map = dict()
    for i in range(n):
        if cCon[i] not in cCon_map:
            cCon_map[cCon[i]] = [i]
        else:
            cCon_map[cCon[i]].append(i)

    res = 0
    for k in cCon_map:
        #if seen[i]: continue
        res += prim(cCon_map[k][0],cCon_map[k]) #Non è necessario Seen dato che abbiamo già la divisione per cCon
        
    #print("Tempo Prim: \n",time.time() - start_time)
    # print("Componenti: ",cCon)
    # print("Res: ",res)

    # Print finale
    print(len(archi),len(cCon_map),res)
    #print("TempoFinale: \n",time.time() - start_time)
    
if len(sys.argv) == 3:
    main([sys.argv[1],sys.argv[2]])
else:
    raise RuntimeError("Uso:", sys.argv[0], "file_grafo file_archi")