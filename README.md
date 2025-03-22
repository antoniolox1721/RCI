# Redes de Dados Identificados por Nome (NDN)

## Descrição Geral do Projeto

Este projeto implementa uma Rede de Dados Identificados por Nome (NDN) no âmbito da unidade curricular de Redes de Computadores e Internet 2024/2025. Uma rede NDN é um sistema distribuído que visa disponibilizar objetos de dados globalmente, sendo que cada objeto está associado a um nome único e pode ser distribuído por vários nós da rede.

Ao contrário das redes tradicionais baseadas em IP, onde a comunicação é orientada à localização (endereços IP), numa rede NDN a comunicação é orientada ao conteúdo. Os utilizadores solicitam dados pelo seu nome, não pela sua localização, e a rede encarrega-se de localizar e entregar o conteúdo solicitado.

## Características Principais

- **Comunicação Baseada em Nomes**: Os objetos são identificados por nomes únicos, não por endereços
- **Topologia em Árvore**: A rede mantém uma estrutura em árvore, simplificando o encaminhamento
- **Caching Distribuído**: Os objetos são armazenados em cache ao longo do caminho de resposta
- **Recuperação Automática**: A rede é capaz de se reorganizar após a saída de nós
- **Interface por Linha de Comandos**: Gestão da rede e objetos através de comandos simples

## Arquitetura Detalhada

### Estrutura da Rede

A implementação NDN é estabelecida como uma rede de sobreposição sobre a Internet tradicional (TCP/IP). Cada nó é identificado por um endereço IP e um porto TCP de escuta. A topologia da rede é restrita a uma árvore para garantir que existe apenas um caminho único entre quaisquer dois nós.

Na estrutura da árvore:
- Cada nó possui exatamente um "vizinho externo" (exceto o nó raiz)
- Um nó pode ter múltiplos "vizinhos internos"
- Cada nó mantém informação sobre um "nó de salvaguarda" para recuperação em caso de falhas

A restrição à topologia em árvore simplifica o encaminhamento, mas requer mecanismos especiais para manter a conetividade quando nós abandonam a rede.

### Tipos de Nós na Topologia

Para manter a estrutura em árvore e permitir a recuperação de falhas, cada nó classifica os seus vizinhos da seguinte forma:

- **Vizinho Externo**: O único vizinho na direção da raiz da árvore
- **Vizinhos Internos**: Todos os outros vizinhos conectados ao nó
- **Nó de Salvaguarda**: O vizinho externo do vizinho externo, usado para recuperação quando o vizinho externo falha

Dois nós especiais na rede são simultaneamente vizinhos externos um do outro, formando o "núcleo" da rede, e são considerados "salvaguarda de si próprios".

## Protocolos de Comunicação

A implementação utiliza três protocolos distintos:

### 1. Protocolo de Registo (UDP)

Este protocolo é utilizado para comunicação com o servidor de registo central, que mantém informações sobre todos os nós registados em cada rede.

#### Mensagens do Protocolo de Registo:

- **NODES net**: Pedido enviado pelo nó para obter a lista de nós presentes na rede `net`.
  ```
  NODES 076
  ```

- **NODESLIST net<LF> IP1 TCP1<LF> IP2 TCP2<LF> ...**: Resposta do servidor contendo a lista de nós na rede.
  ```
  NODESLIST 076
  193.136.138.142 58001
  192.168.1.10 58002
  ```

- **REG net IP TCP**: Mensagem para registar um nó numa rede.
  ```
  REG 076 193.136.138.142 58001
  ```

- **UNREG net IP TCP**: Mensagem para remover o registo de um nó.
  ```
  UNREG 076 193.136.138.142 58001
  ```

- **OKREG**: Confirmação de registo bem-sucedido.

- **OKUNREG**: Confirmação de remoção de registo bem-sucedida.

#### Processo de Entrada na Rede via Servidor de Registo:

1. O nó envia uma mensagem `NODES net` ao servidor de registo
2. O servidor responde com `NODESLIST` contendo os nós registados
3. O nó seleciona aleatoriamente um nó da lista e conecta-se a ele via TCP
4. O nó envia uma mensagem `ENTRY` ao nó selecionado (Protocolo de Topologia)
5. O nó regista-se no servidor com `REG net IP TCP`
6. O servidor confirma com `OKREG`

### 2. Protocolo de Topologia (TCP)

Este protocolo é utilizado para manter a topologia em árvore e gerir a entrada e saída de nós na rede.

#### Mensagens do Protocolo de Topologia:

- **ENTRY IP TCP<LF>**: Mensagem enviada por um nó para informar outro da sua entrada na rede.
  ```
  ENTRY 193.136.138.142 58001
  ```

- **SAFE IP TCP<LF>**: Mensagem que contém informação sobre o nó de salvaguarda.
  ```
  SAFE 192.168.1.10 58002
  ```

#### Manutenção da Topologia:

**Entrada de um Nó:**
1. Um nó entrante conecta-se a um nó existente via TCP
2. O nó entrante envia `ENTRY IP TCP` com o seu endereço e porto de escuta
3. O nó recetor adiciona o remetente como vizinho interno
4. O nó recetor responde com `SAFE IP TCP` contendo informação do seu vizinho externo
5. O nó entrante configura o nó recetor como seu vizinho externo
6. O nó entrante define o nó recebido na mensagem `SAFE` como seu nó de salvaguarda

**Saída de um Nó:**
Quando um nó decide sair da rede:
1. Remove o seu registo no servidor com `UNREG`
2. Fecha todas as ligações TCP com os seus vizinhos

**Reação à Saída de um Nó:**
Quando um nó deteta que o seu vizinho externo saiu da rede:
- Se não for salvaguarda de si próprio:
  1. Conecta-se ao seu nó de salvaguarda
  2. Envia-lhe uma mensagem `ENTRY`
  3. Define-o como novo vizinho externo
  4. Envia mensagens `SAFE` a todos os seus vizinhos internos

- Se for salvaguarda de si próprio e tiver vizinhos internos:
  1. Escolhe um dos seus vizinhos internos para ser o novo vizinho externo
  2. Envia-lhe uma mensagem `ENTRY`
  3. Envia mensagens `SAFE` a todos os seus vizinhos internos

- Se for salvaguarda de si próprio e não tiver vizinhos internos:
  1. Torna-se um nó isolado, aguardando novas conexões

### 3. Protocolo NDN (TCP)

Este protocolo é responsável pela pesquisa e recuperação de objetos na rede NDN.

#### Mensagens do Protocolo NDN:

- **INTEREST nome<LF>**: Pedido de um objeto específico pelo seu nome.
  ```
  INTEREST objeto123
  ```

- **OBJECT nome<LF>**: Resposta contendo o objeto solicitado.
  ```
  OBJECT objeto123
  ```

- **NOOBJECT nome<LF>**: Mensagem indicando que o objeto não foi encontrado.
  ```
  NOOBJECT objeto123
  ```

#### Estados de Interface na Tabela de Interesses:

Cada nó mantém uma "tabela de interesses" que regista o estado de cada interface relativamente a pedidos pendentes:

- **RESPONSE**: Interface por onde uma mensagem de resposta deve ser encaminhada
- **WAITING**: Interface por onde uma mensagem de interesse foi enviada
- **CLOSED**: Interface por onde uma mensagem NOOBJECT foi recebida

#### Processo de Pesquisa de Objeto:

1. Um nó emite uma mensagem `INTEREST nome` por todas as suas interfaces
2. Quando um nó recebe uma mensagem `INTEREST nome`:
   - Se tiver o objeto, responde com `OBJECT nome`
   - Se não tiver o objeto e não estiver a processá-lo, encaminha a mensagem para outras interfaces
   - Se já tiver recebido `NOOBJECT` de todas as interfaces, responde com `NOOBJECT nome`

3. Quando um nó recebe uma mensagem `OBJECT nome`:
   - Adiciona o objeto à sua cache
   - Encaminha a mensagem para todas as interfaces marcadas como RESPONSE
   - Remove a entrada da tabela de interesses

4. Quando um nó recebe uma mensagem `NOOBJECT nome`:
   - Marca a interface como CLOSED
   - Se não restarem interfaces em WAITING, encaminha `NOOBJECT nome` para interfaces em RESPONSE

## Lógica de Manutenção da Rede

### Estrutura de Salvaguarda

Para garantir a robustez da rede face à saída de nós, cada nó mantém as seguintes informações:

- **vizinho_externo**: Endereço e porto do vizinho na direção da raiz
- **nó_salvaguarda**: Endereço e porto do vizinho externo do vizinho externo
- **vizinhos_internos**: Lista de todos os outros vizinhos

Quando um nó sai da rede, os seus vizinhos usam esta informação para reconectar-se e reorganizar a topologia.

### Deteção de Saída de Nós

A saída de um nó é detetada quando uma ligação TCP é encerrada. Quando isto ocorre:

1. Os nós vizinhos identificam se o nó que saiu era um vizinho externo ou interno
2. Se era um vizinho interno, simplesmente removem-no da lista
3. Se era um vizinho externo, inicia-se o procedimento de recuperação

### Procedimento de Recuperação

O procedimento de recuperação varia conforme o tipo de nó:

1. **Nós Normais** (não salvaguarda de si próprio):
   - Conectam-se ao nó de salvaguarda
   - Enviam mensagem `ENTRY`
   - Atualizam os seus vizinhos internos com nova informação de salvaguarda

2. **Nós de Núcleo** (salvaguarda de si próprio):
   - Se tiverem vizinhos internos, escolhem um como novo vizinho externo
   - Enviam mensagem `ENTRY` ao novo vizinho externo
   - Atualizam os seus vizinhos internos

3. **Nó Isolado** (sem conexões):
   - Aguarda por novas conexões para se juntar à rede novamente

### Casos Especiais

- **Rede com Dois Nós**: Ambos são vizinhos externos um do outro e salvaguarda de si próprios
- **Rede com Um Nó**: Um nó isolado é considerado um caso válido, aguardando conexões
- **Partição da Rede**: Se ocorrer uma partição, formam-se duas redes independentes

## Implementação Detalhada

### Gestão de Objetos e Cache

Cada nó mantém duas listas de objetos:
- **Objetos Locais**: Criados localmente pelo utilizador
- **Objetos em Cache**: Obtidos durante o encaminhamento de respostas

A cache tem um tamanho máximo definido na inicialização do nó. Quando a cache está cheia, o objeto mais antigo é removido para dar lugar ao novo, seguindo uma política LRU (Least Recently Used).

### Tabela de Interesses

A tabela de interesses é uma estrutura fundamental que regista:
- Nome do objeto solicitado
- Estado de cada interface (RESPONSE, WAITING, CLOSED)
- Timestamp para gestão de timeouts

Esta tabela é atualizada à medida que mensagens `INTEREST`, `OBJECT` e `NOOBJECT` são processadas, permitindo o encaminhamento correto das respostas.

### Timeouts

Os interesses têm um tempo limite associado (tipicamente 10 segundos). Se uma resposta não for recebida dentro deste período:
- As interfaces em estado WAITING são consideradas inacessíveis
- Mensagens `NOOBJECT` são enviadas para interfaces em estado RESPONSE
- A entrada é removida da tabela de interesses

## Comandos Disponíveis

A interface de utilizador suporta os seguintes comandos:

### Gestão de Rede

- **join (j) net**: Entrar numa rede através do servidor de registo
  ```
  j 076
  ```

- **direct join (dj) connectIP connectTCP**: Entrar numa rede diretamente através de um nó
  ```
  dj 193.136.138.142 58001
  ```
  Se connectIP for 0.0.0.0, cria uma nova rede apenas com este nó

- **leave (l)**: Sair da rede atual
  ```
  l
  ```

- **exit (x)**: Fechar a aplicação
  ```
  x
  ```

### Gestão de Objetos

- **create (c) name**: Criar um objeto localmente
  ```
  c objeto123
  ```

- **delete (dl) name**: Remover um objeto local
  ```
  dl objeto123
  ```

- **retrieve (r) name**: Obter um objeto da rede
  ```
  r objeto123
  ```

### Visualização de Informações

- **show topology (st)**: Mostrar a topologia da rede
  ```
  st
  ```

- **show names (sn)**: Listar objetos armazenados no nó
  ```
  sn
  ```

- **show interest table (si)**: Mostrar a tabela de interesses
  ```
  si
  ```

## Compilação e Execução

### Requisitos
- Sistema operativo tipo UNIX (Linux, macOS)
- Compilador GCC
- Make

### Compilação
```bash
make
```

### Execução
```bash
./ndn <tamanho_cache> <IP> <porto_TCP> [regIP] [regUDP]
```

#### Parâmetros:
- **tamanho_cache**: Tamanho máximo da cache de objetos
- **IP**: Endereço IP do nó
- **porto_TCP**: Porto TCP de escuta
- **regIP** (opcional): Endereço IP do servidor de registo (predefinido: 193.136.138.142)
- **regUDP** (opcional): Porto UDP do servidor de registo (predefinido: 59000)

### Exemplo de Execução
```bash
# Iniciar um nó com cache de tamanho 10 em localhost:58001
./ndn 10 127.0.0.1 58001

# Iniciar um nó com servidor de registo específico
./ndn 10 127.0.0.1 58001 193.136.138.142 59000
```

## Cenários de Utilização

### Criação de uma Nova Rede
1. Iniciar o primeiro nó:
   ```
   ./ndn 10 127.0.0.1 58001
   ```
2. Criar uma rede isolada:
   ```
   dj 0.0.0.0 0
   ```

### Juntar-se a uma Rede Existente
1. Iniciar um nó:
   ```
   ./ndn 10 127.0.0.1 58002
   ```
2. Juntar-se a uma rede utilizando o servidor de registo:
   ```
   j 076
   ```
   Ou diretamente através de um nó conhecido:
   ```
   dj 127.0.0.1 58001
   ```

### Partilha de Objetos
1. Criar objetos no nó:
   ```
   c objeto1
   c objeto2
   ```
2. Listar objetos disponíveis:
   ```
   sn
   ```
3. Noutro nó, obter estes objetos:
   ```
   r objeto1
   r objeto2
   ```

### Visualização da Topologia
Para verificar a estrutura atual da rede:
```
st
```
Isto mostra o vizinho externo, o nó de salvaguarda e todos os vizinhos internos.

## Considerações de Implementação

### Segurança
Esta implementação foca-se nos aspetos fundamentais do NDN e não inclui mecanismos de segurança. Num ambiente de produção, seria necessário adicionar:
- Autenticação de nós
- Verificação de integridade de objetos
- Controlo de acesso

### Desempenho
O desempenho da implementação é afetado por:
- Tamanho da topologia
- Frequência de entrada/saída de nós
- Tamanho da cache
- Padrão de acesso aos objetos

### Extensões Possíveis
A implementação atual poderia ser estendida para incluir:
- Suporte a topologias mais complexas (além da árvore)
- Políticas de cache mais sofisticadas
- Compressão de objetos
- Fragmentação de objetos grandes

## Autores
Bárbara Gonçalves Modesto e António Pedro Lima Loureiro Alves

Unidade Curricular de Redes de Computadores e Internet, 2024/2025
Instituto Superior Técnico, Universidade de Lisboa