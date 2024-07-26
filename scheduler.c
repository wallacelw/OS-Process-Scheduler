#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#define MAXN 1000

enum Type { T15, T30 };

bool adj[MAXN][MAXN];
enum Type procType[MAXN];

struct Process {
    int id;
    enum Type type;
};

typedef struct {
    int id;
    char command[20];
    char input_dependency[50];
} Data;

// Função para ler o arquivo e armazenar os dados em uma lista
Data* readFile(const char *filename, int *numLines) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        return NULL;
    }

    Data *dataList = NULL;
    *numLines = 0;
    char line[100];

    while (fgets(line, sizeof(line), file)) {
        dataList = realloc(dataList, (*numLines + 1) * sizeof(Data));
        if (dataList == NULL) {
            perror("Erro ao realocar memória");
            fclose(file);
            return NULL;
        }

        sscanf(line, "%d %s %s", &dataList[*numLines].id, dataList[*numLines].command, dataList[*numLines].input_dependency);
        (*numLines)++;
    }

    fclose(file);
    return dataList;
}

// Função para imprimir a lista de dados
void printDataList(Data *dataList, int numLines) {
    for (int i = 0; i < numLines; i++) {
        printf("ID: %d, Command: %s, Input Dependency: %s\n", dataList[i].id, dataList[i].command, dataList[i].input_dependency);
        if(strcmp(dataList[i].command, "teste15")==0){
            procType[dataList[i].id]=T15;
        }else{
            procType[dataList[i].id]=T30;
        }
    }
}

// Função para gerar o grafo
void make_graph(Data* dataList,int num){
    for (int i = 0; i < num; i++) {
        int curNum=0;
        for(int j=0;j<50;j++){
            char curChar=dataList[i].input_dependency[j];
            if(curChar==','){
                adj[curNum][dataList[i].id]=true;
                curNum=0;
                continue;
            }

            if(curChar=='#')break;
            curNum*=10;curNum+=(curChar-'0');
        }
    }
}

// Busca em profundidade para conseguir a pós ordem
void dfs(int v,int n,bool *vis, int*stack,int *idx){
    vis[v]=true;
    for(int i = 0;i<=n;i++){
        if(adj[v][i] && !vis[i]){
            dfs(i,n,vis,stack,idx);
        }
    }

    stack[*idx]=v;
    (*idx)++;
}

// O inverso da pós ordem é uma ordem topológica
int* topoSort(int n){
    bool vis[n+1];
    memset(vis, false, sizeof(vis));

    int* stack = (int*)malloc(n * sizeof(int));
    int idx = 0;

    for(int i = 1;i<=n;i++){
        if(vis[i]==false)dfs(i,n,vis,stack,&idx);
    }

    return stack;
}

// função para imprimir o tempo atual do processo sendo executado
void print_time() {
    double time = clock();
    time /= CLOCKS_PER_SEC;
    printf("[%010.6lf] ", time);
}

// função para verificar se algum processo filho deu exit()
void busy_waiting(int cores, int *available_cores, pid_t child_pid[], int child_id[]) {
    for(int j=0; j<cores; j++) if (child_pid[j]) {

        int status;
        pid_t result = waitpid(child_pid[j], &status, WNOHANG);

        if (result > 0) { // o filho deu exit()
            print_time();
            printf("+ (pid %d) {id %d} Exited with status %d\n", child_pid[j], child_id[j], WEXITSTATUS(status));
            
            // libera um core
            *available_cores += 1;
            child_pid[j] = 0;
            break;
        }

        else if (result == -1) { // erro no waitpid
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        // else, o processo ainda está executando e não deu exit()
    }
}

int main(int argc, char *argv[]) {
    
    // verifica se foram passsados 2 atributos
    if (argc != 3) {
        printf("O usuário não informou os dois parâmetros obrigatórios! \n");
        return 0;
    }

    // Salva o número de cores disponíveis pelo sistema
    const int cores = atoi(argv[1]);
    printf("NÚMERO DE NÚCLEOS: %s\n", argv[1]);

    // variável utilizado como semáforo
    int available_cores = cores;

    // Tabelas para salvar o PID e ID do processo sendo executado em cada core
    pid_t child_pid[cores];
    int child_id[cores];
    
    // Inicializar os cores
    // child_pid[i] = 0 significa que o core i está disponível
    memset(child_pid, 0, sizeof(child_pid));

    // Nome do arquivo a ser lido, passado como segundo argumento da chamada do programa
    const char *filename = argv[2];

    // Lê o arquivo de entrada
    int numLines;
    Data *dataList = readFile(filename, &numLines);

    // Gera o grafo
    make_graph(dataList, numLines);
    
    // Testa o grafo
    for(int i = 0;i<MAXN;i++){
        for(int j=0;j<MAXN;j++){
            if(adj[i][j]){
                printf("aresta de %d para %d\n",i,j);
            }
        }
    }

    // Obtém uma ordem topológica
    int* stack = topoSort(numLines);


    if (dataList != NULL) {
        printDataList(dataList, numLines);
        free(dataList);
    }

    printf("TopoSort:\n");
    printf("%d\n",numLines);
    for(int i =numLines-1;i>=0;i--){
        printf("%d, ",stack[i]);
    }
    printf("\n");
    for(int i = 1;i<=numLines;i++){
        printf("%d\n",(procType[i]==T15));
    }

    // inicializa o temporizador do Makespan
    double start_time = clock();
    start_time /= CLOCKS_PER_SEC; 

    // Começa a execução dos processos
    for(int i = numLines-1; i >=0; i--) {
        
        // Trave de execução quando não há nenhum core disponível
        // Fica em busy_waiting até um processo filho alocado da exit() e libera um core
        while(available_cores == 0) {
            busy_waiting(cores, &available_cores, child_pid, child_id);
        }

        // Trave de execução quando não há nenhum processo que pode ser executado no momento
        // Devido a lista de dependências

        // Aloca o core para o processo
        available_cores -= 1;

        int id = stack[i];
        printf("id: %d\n",id);
        enum Type type = procType[id];

        // Cria um fork do processo atual
        pid_t pid = fork();

        if (pid == 0) { // Somente o processo filho entra aqui

            if (type == T15) {
                char *args[] = {"./process15", NULL};
                execvp(args[0], args);
            }

            else if (type == T30) {
                char *args[] = {"./process30", NULL};
                execvp(args[0], args);
            }
            
            // Após chamar exec(), o filho não tem mais o código do pai
            // Logo, se o código chegar aqui, quer dizer que houve um erro com execvp 
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        else if (pid > 0) { // Somente o processo pai entra aqui
            child_pid[i % cores] = pid; 
            child_id[i % cores] = id;

            print_time();
            printf("/ (pid %d) forked (pid %d)\n", getpid(), pid);

            print_time();
            printf("- (pid %d) {id %d} Execute {type %d}\n", pid, id, type);
        }

        else { // Se (pid < 0) => Erro no fork()
            perror("fork");
            exit(EXIT_FAILURE);
        }

    }

    // Acabaram os processes que deviam ser executados
    // Mas eles podem estar sendo executados ainda
    // Trava a execução até TODOS os filhos derem exit()
    while(available_cores < cores) {
        busy_waiting(cores, &available_cores, child_pid, child_id);
    }

    // termina o temporizador do Makespan
    double end_time = clock();
    end_time /= CLOCKS_PER_SEC;

    // Calcula o Makespan e imprime
    double run_time = end_time - start_time;
    printf("Makespan time: %.6lf seconds \n", run_time);
}