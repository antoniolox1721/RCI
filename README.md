# Implementação de Rede de Dados Identificados por Nome (NDN)

## Visão Geral do Projeto

Este projeto implementa uma Rede de Dados Identificados por Nome (NDN) conforme especificado para a unidade curricular de Redes de Computadores e Internet (2024/2025). A implementação cria um sistema de rede distribuído onde os objetos de dados são identificados e recuperados pelos seus nomes em vez das suas localizações, construído como uma rede de sobreposição sobre TCP/IP.

## Características Principais

- Manutenção de topologia de rede em árvore
- Descoberta e recuperação de conteúdo baseada em interesses
- Cache de objetos para melhor desempenho
- Tratamento robusto de falhas de nós e partições de rede
- Interface de linha de comandos para gestão de rede e conteúdo

## Arquitetura

A implementação NDN segue estes princípios fundamentais:

- Cada nó é identificado por um endereço IP e porto TCP
- Os objetos de dados são identificados por nomes únicos globalmente
- A topologia de rede é mantida como uma estrutura em árvore
- O conteúdo é armazenado em cache ao longo dos caminhos de recuperação
- As mensagens viajam na direção oposta aos interesses

## Protocolos de Rede

### Protocolo de Registo (UDP)
- `NODES <net>` - Solicita lista de nós numa rede
- `NODESLIST <net>\n<IP1> <TCP1>\n<IP2> <TCP2>\n...` - Resposta com nós registados
- `REG <net> <IP> <TCP>` - Regista um nó numa rede
- `UNREG <net> <IP> <TCP>` - Cancela o registo de um nó numa rede
- `OKREG` - Confirmação de registo bem-sucedido
- `OKUNREG` - Confirmação de cancelamento de registo bem-sucedido

**Sequência de Registo de Nó:**
1. Cliente envia `NODES <net>` ao servidor de registo
2. Servidor responde com `NODESLIST <net>\n<IP1> <TCP1>\n...`
3. Cliente seleciona um nó e conecta-se via TCP
4. Cliente envia `REG <net> <IP> <TCP>` para se registar
5. Servidor confirma com `OKREG`

**Sequência de Cancelamento de Registo de Nó:**
1. Cliente envia `UNREG <net> <IP> <TCP>` ao servidor de registo
2. Servidor confirma com `OKUNREG`
3. Cliente fecha todas as ligações TCP com vizinhos

### Protocolo de Topologia (TCP)
- `ENTRY <IP> <TCP>\n` - Informa um nó da entrada na rede
- `SAFE <IP> <TCP>\n` - Fornece informação do nó de salvaguarda

**Sequência de Entrada de Nó:**
1. Novo nó conecta-se a um nó existente via TCP
2. Novo nó envia `ENTRY <IP> <TCP>\n` com o seu endereço de escuta
3. Nó recetor adiciona o remetente como vizinho interno
4. Nó recetor responde com `SAFE <EXT_IP> <EXT_TCP>\n` contendo informação do seu vizinho externo

**Tratamento de Saída de Nó:**
1. Quando um nó deteta um vizinho externo desconectado:
   - Se não for auto-salvaguardado: Conecta-se ao nó de salvaguarda, envia `ENTRY`, atualiza vizinhos internos
   - Se for auto-salvaguardado: Escolhe novo externo entre vizinhos internos, envia `ENTRY`, atualiza outros
2. Para cada vizinho interno, envia mensagens `SAFE` com informação atualizada do nó de salvaguarda

### Protocolo NDN (TCP)
- `INTEREST <name>\n` - Pedido de um objeto pelo nome
- `OBJECT <name>\n` - Resposta contendo o objeto solicitado
- `NOOBJECT <name>\n` - Notificação de que o objeto não está disponível

**Sequência de Recuperação de Objeto:**
1. Nó solicitante envia `INTEREST <name>\n` para todas as interfaces
2. Cada nó recetor:
   - Se tiver o objeto: Responde com `OBJECT <name>\n`
   - Se não tiver o objeto: Reencaminha `INTEREST <name>\n` para outras interfaces
   - Se todos os caminhos estiverem fechados: Responde com `NOOBJECT <name>\n`
3. Nós intermédios:
   - Armazenam objetos em cache à medida que passam
   - Reencaminham respostas de volta para a interface solicitante
   - Atualizam estados da tabela de interesses conforme necessário

**Transições de Estado da Tabela de Interesses:**
- Ao receber `INTEREST`: Marca interface de origem como `RESPONSE`, outras interfaces como `WAITING`
- Ao receber `OBJECT`: Reencaminha para todas as interfaces `RESPONSE`, adiciona à cache, remove entrada de interesse
- Ao receber `NOOBJECT`: Marca interface de origem como `CLOSED`, verifica se alguma `WAITING` permanece

## Comandos

### Gestão de Rede
- `join (j) <net>` - Aderir a uma rede através do servidor de registo
- `direct join (dj) <connectIP> <connectTCP>` - Aderir diretamente a um nó
- `leave (l)` - Sair da rede atual
- `exit (x)` - Fechar a aplicação

### Gestão de Objetos
- `create (c) <name>` - Criar um novo objeto
- `delete (dl) <name>` - Eliminar um objeto existente
- `retrieve (r) <name>` - Procurar e recuperar um objeto

### Visualização de Informações
- `show topology (st)` - Mostrar ligações de rede
- `show names (sn)` - Listar objetos armazenados no nó
- `show interest table (si)` - Mostrar interesses pendentes

## Utilização

### Compilação
```bash
make
```

### Execução da Aplicação
```bash
./ndn <tamanho_cache> <IP> <porto_TCP> [regIP] [regUDP]
```

### Exemplo
```bash
# Iniciar um nó com tamanho de cache 10 em localhost:58001
./ndn 10 127.0.0.1 58001

# Iniciar um nó com servidor de registo específico
./ndn 10 127.0.0.1 58001 193.136.138.142 59000
```

### Testes
O projeto inclui scripts de teste para validar as funcionalidades:
```bash
# Executar a suite de testes completa
./test.sh

# Executar os testes específicos de NDN
./ndn_test.sh
```

## Detalhes de Implementação

### Tabela de Interesses
A tabela de interesses rastreia pedidos pendentes para objetos:
- Interfaces `RESPONSE` onde mensagens de objeto/ausência devem ser reencaminhadas
- Interfaces `WAITING` onde mensagens de interesse foram enviadas
- Interfaces `CLOSED` onde mensagens de ausência foram recebidas

### Recuperação de Rede
Quando um nó se desconecta:
- Se o vizinho externo se desconecta, conecta-se ao nó de salvaguarda
- Se um nó é o seu próprio nó de salvaguarda, escolhe um novo vizinho externo
- Propaga informação de topologia atualizada para todos os vizinhos internos

## Estrutura do Projeto
- `main.c` - Ponto de entrada da aplicação e ciclo principal
- `ndn.h` - Estruturas de dados principais e definições
- `commands.c/h` - Processamento de comandos do utilizador
- `network.c/h` - Comunicação de rede e tratamento de protocolos
- `objects.c/h` - Gestão de objetos e operações na tabela de interesses
- `debug_utils.c/h` - Utilitários de depuração e relatório de erros

## Autores
Bárbara Gonçalves Modesto e António Pedro Lima Loureiro Alves
Redes de Computadores e Internet, 2024/2025

## Agradecimentos
- Professores e assistentes da unidade curricular
- IST - Instituto Superior Técnico, Universidade de Lisboa