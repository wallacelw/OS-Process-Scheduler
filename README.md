# OS-Process-Scheduler

Implement a process scheduler controlled by the Operating System.

# Environment

Ubuntu 22.04.4 LTS, 64-bit

gcc version 11.4.0 (Ubuntu 11.4.0-1ubuntu1~22.04)

# Scheduling Algorithm

**DAG prunning:** Remove leaves and update neighbors until there is no vertice left.

# Execution Command

```bash
gcc -g -Wall -Wextra -o process15 process15.c

gcc -g -Wall -Wextra -o process30 process30.c

gcc -g -Wall -Wextra -o escalona scheduler.c

./escalona <cores_qtd> <nome_do_arquivo>

```

# Members

211010280 - Breno Costa Avelino Lima

211036052 - Henrique de Oliveira Ramos  

200028880 - Wallace Ben Teng Lin Wu