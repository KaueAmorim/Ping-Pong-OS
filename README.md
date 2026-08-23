# Ping-Pong OS (PPOS) - v2

O **Sistema Operacional Ping-Pong**, ou **PPOS** (*Ping-Pong Operating System*), é um projeto acadêmico que visa construir um sistema operacional didático (*toy operating system*) em linguagem C.

## Sobre o Projeto

Ao contrário de projetos similares, o PPOS executa inteiramente dentro de um processo do Linux. A maior vantagem em emular um SO dentro de um processo em modo usuário é a possibilidade de usar ferramentas padrão de depuração, como o Valgrind e o GDB no modo *default*, o que reduz drasticamente a curva de aprendizado do aluno.

O PPOS segue a estrutura típica de um **RTOS** (*Real-Time Operating System*), um sistema operacional com espaço de memória único, no qual o núcleo (*kernel*) e as aplicações são compilados juntos, formando uma única imagem binária.

## Recursos Suportados

A versão atual do PPOS possui suporte para as seguintes funcionalidades:

- [x] Múltiplas tarefas com preempção por tempo
- [x] Escalonador baseado em prioridades
- [x] Semáforos e concorrência
- [x] Filas de mensagens
- [x] Alocador de memória *heap*
- [x] Acesso a disco e sistema de arquivos

## Limitações

Devido à sua característica de ser executado dentro de um processo Linux em modo usuário, o PPOS **não suporta**:

- Memória virtual
- Separação de privilégios usuário/kernel
- Chamadas de sistema (*syscalls*) por interrupções/*traps*

## Estrutura do Código-Fonte

O código-fonte do PPOS versão 2 está organizado na seguinte hierarquia de diretórios:

```text
ppos/
├── hardware/              # Emulação de componentes de hardware
│   ├── disk*              # Disco rígido
│   ├── cpu.*              # Operações da CPU e chipset
│   ├── serial.*           # Porta serial (console)
│   └── makefile
├── kernel/                # Núcleo do sistema operacional PPOS
│   ├── ppos.*             # Núcleo (arquivo principal)
│   ├── macros.h           # Macros de uso geral
│   ├── ctx.*              # Trocas de contexto (assembly e C)
│   ├── tcb.h              # Task Control Block (descritor de tarefa)
│   ├── task.*             # Gestão básica de tarefas
│   ├── dispatcher.*       # Despachante de tarefas
│   ├── scheduler.*        # Escalonador de tarefas
│   ├── time.*             # Gestão do tempo e timers
│   ├── semaphore.*        # Spinlocks e semáforos
│   ├── mqueue.*           # Filas de mensagens
│   ├── memory.*           # Gestão de alocação de memória heap
│   ├── block.*            # Acesso aos blocos do disco
│   ├── syscall.h          # Funções acessíveis às aplicações (API)
│   └── makefile
├── lib/                   # Bibliotecas de apoio e utilitários
│   ├── map.*              # Biblioteca de mapas genéricos
│   ├── queue.*            # Biblioteca de filas genéricas
│   ├── pplibc.*           # Mini-biblioteca C padrão
│   └── makefile
├── test/                  # Programas e cenários de teste
│   ├── pingpong-*.c       # Código-fonte dos programas de teste
│   ├── pingpong-*.txt     # Saídas (outputs) esperadas dos testes
│   └── makefile
└── makefile               # Makefile global na raiz do projeto
```
