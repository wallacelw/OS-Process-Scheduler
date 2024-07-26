#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#define MAXN 10000

// T15 = processo que demora 15 segundos
// T30 = processo que demora 30 segundos
enum Type { T15, T30 };

// Lista de adjacência do grafo
bool adj[MAXN][MAXN];

// Grau de entrada de cada nó
int indeg[MAXN];

// Dado o ID do processo, ele me informa o tipo desse processo
enum Type procType[MAXN];

// Estrutura de cada nó
typedef struct {
    int id;
    char command[20];
    char input_dependency[100];
} Data;

// O tempo de execução de cada processo
double execution_time[MAXN];

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
            procType[dataList[i].id] = T15;
        }
        else {
            procType[dataList[i].id] = T30;
        }
    }
}

// Função para gerar o grafo
void make_graph(Data* dataList, int num){
    for (int i = 0; i < num; i++) {
        int curNum=0;
        for(int j=0;j<100;j++){
            char curChar=dataList[i].input_dependency[j];
            if(curChar==','){
                if(curNum!=0){
                    adj[curNum][dataList[i].id]=true;
                    indeg[dataList[i].id]++;
                }
                curNum=0;
                continue;
            }

            if(curChar=='#')break;
            curNum*=10;curNum+=(curChar-'0');
        }
    }
}

// Função para imprimir o tempo atual do processo sendo executado
void print_time() {
    double time = clock();
    time /= CLOCKS_PER_SEC;
    printf("[%010.5lf] ", time);
}

// Lê o próximo elemento disponível na lista
int readFromQueue(int *readPtr, int* queue) {
    int aux = queue[*readPtr];
    if (aux != -1) (*readPtr)++;
    return aux;
}

// Adiciona um nó com grau de entrada 0 na fila
void addToQueue(int v, int* insertPtr, int* queue) {
    printf("adding: %d\n", v);
    queue[*insertPtr] = v;
    (*insertPtr)++;
    // printf("adding2: %d\n", v);
}

// dado que um processo finalizou,
// atualiza o grau de entrada dos nós adjacentes
void attQueue(int v, int n, int* insertPtr, int* queue) {
    for(int i=1; i<=n; i++){
        if(adj[v][i]){
            indeg[i]--;
            if(indeg[i]==0) addToQueue(i, insertPtr, queue);
        }
    }
}

// Função para verificar se algum processo filho deu exit()
void busy_waiting(int cores, int *available_cores, pid_t child_pid[], int child_id[], int numLines, int* insertPtr, int *queue, int (*fd)[2]) {

    for(int j=0; j<cores; j++) if (child_pid[j]) {

        int status;
        pid_t result = waitpid(child_pid[j], &status, WNOHANG);

        if (result > 0) { // o filho deu exit()
            print_time();
            printf("+ (pid %d) {id %d} Exited with status %d\n", child_pid[j], child_id[j], WEXITSTATUS(status));
            
            // Atualiza o grau de entrada dos nós adjacentes
            attQueue(child_id[j], numLines, insertPtr, queue);

            // libera um core
            *available_cores += 1;

            // lê o tempo de execução escrito no pipe pelo processo filho
            char buffer[15]; // 4 digits + '.' + 6 digits + '\0'
            ssize_t count = read(fd[child_id[j]][0], buffer, sizeof(buffer));
            if (count < 0) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            char *trash;
            execution_time[child_id[j]] = strtod(buffer, &trash);
            
            // Fecha o pipe de escrita
            close(fd[child_id[j]][0]);

            // libera os valores armazenados no core
            child_id[j] = -1;
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
        exit(EXIT_FAILURE);
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
    for(int i=0; i<MAXN; i++){
        for(int j=0; j<MAXN; j++){
            if(adj[i][j]){
                printf("aresta de %d para %d\n",i,j);
            }
        }
    }

    // Imprime a entrada lida e libera o espaço de memória
    if (dataList != NULL) {
        printDataList(dataList, numLines);
        free(dataList);
    } 

    // inicializa o temporizador do Makespan
    double start_time = clock();
    start_time /= CLOCKS_PER_SEC; 

    // cria a queue, que seria a lista dos nós que podem ser executados
    int* queue = (int*) malloc(numLines* sizeof(int));
    int insertPtr = 0;
    int readPtr = 0;
    memset(queue, -1, numLines * sizeof(int));

    // inicializa a lista, com todos os nós com grau 0 de entrada
    for(int i = 1; i<=numLines; i++){
        printf("i: %d, indeg = %d\n",i,indeg[i]);
        if(indeg[i]==0){
            printf("i: %d\n",i);
            addToQueue(i, &insertPtr, queue);
        }
    }

    // Tabela para armazenar os pipes de cada processo
    int fd[numLines+10][2];

    // Começa a execução dos processos
    for(int i = 0; i < numLines; i++) {

        // Trave de execução quando não há nenhum core disponível
        // Fica em busy_waiting até um processo filho alocado da exit() e libera um core
        while(available_cores == 0) {
            busy_waiting(cores, &available_cores, child_pid, child_id, numLines, &insertPtr, queue, fd);
        }

        // Trava a execução quando não há nenhum processo que pode ser executado no momento
        // devido a lista de dependências até que exista um elemento,
        // então obtém o próximo nó a ser executado, que não possui pendências
        int id = -1;
        do { 
            busy_waiting(cores, &available_cores, child_pid, child_id, numLines, &insertPtr, queue, fd);
            id = readFromQueue(&readPtr, queue);
        } while (id == -1);

        printf("v: %d\n", id);
        enum Type type = procType[id];

        // Aloca um core para esse processo
        int used_core = -1;
        for(int j=0; j<cores; j++) {
            if (child_pid[j] == 0) {
                used_core = j; 
                break;
            }
        }
        available_cores -= 1;

        // Cria um pipe para o filho informar o pai do seu tempo de execução
        if(pipe(fd[id]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        // Cria um fork do processo atual
        pid_t pid = fork();

        if (pid == 0) { // Somente o processo filho entra aqui

            // Fecha o pipe de leitura, já que o filho apenas escreve
            close(fd[id][0]);

            // converte o STDOUT do processo filho para o pipe de escrita
            dup2(fd[id][1], STDOUT_FILENO);

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

            // Fecha o pipe de escrita, já que o pai apenas lê
            close(fd[id][1]);

            // salva na tabela o id e o pid do processo a ser executado
            child_pid[used_core] = pid;
            child_id[used_core] = id;

            print_time();
            printf("/ (pid %d) Forked (pid %d)\n", getpid(), pid);

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
        busy_waiting(cores, &available_cores, child_pid, child_id, numLines, &insertPtr, queue, fd);
    }

    // termina o temporizador do Makespan
    double end_time = clock();
    end_time /= CLOCKS_PER_SEC;

    // Calcula o Makespan e imprime
    double run_time = end_time - start_time;
    printf("Tempo de Makespan: %.6lf segundos \n", run_time);

    for(int i=1; i<=numLines; i++) {
        printf("Tempo de Execução do processo com {id = %d}: %010.5lf \n", i, execution_time[i]);
    }
}