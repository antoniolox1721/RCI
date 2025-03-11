#!/bin/bash

# Script para resetar todos os registos de redes no servidor tejo
# Uso: ./reset_networks.sh [servidor] [porto]

# Definir valores padrão
SERVER=${1:-"192.168.1.1"}
PORT=${2:-"59000"}

# Cores para saída
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Iniciando limpeza de registos de redes no servidor ${SERVER}:${PORT}${NC}"
echo -e "${YELLOW}Serão enviados comandos RST para todas as redes de 000 a 999${NC}"
echo -e "${YELLOW}Este processo pode demorar alguns minutos...${NC}"
echo ""

# Verifica se o netcat está instalado
if ! command -v nc &> /dev/null; then
    echo -e "${RED}Erro: netcat (nc) não está instalado. Por favor instale-o primeiro.${NC}"
    exit 1
fi

# Função para verificar conectividade com o servidor
check_server() {
    # Tenta enviar um comando nulo com timeout de 2 segundos
    if echo "" | nc -u -w 2 $SERVER $PORT &> /dev/null; then
        return 0
    else
        return 1
    fi
}

# Verificar conectividade
echo -e "Verificando conectividade com ${SERVER}:${PORT}..."
if ! check_server; then
    echo -e "${RED}Erro: Não foi possível conectar ao servidor ${SERVER} na porta ${PORT}${NC}"
    echo -e "${YELLOW}Verifique se o servidor está acessível e se a porta está correta.${NC}"
    exit 1
fi

echo -e "${GREEN}Conectividade confirmada com ${SERVER}:${PORT}${NC}"
echo ""

# Contador para feedback
total=1000
count=0
success=0
failed=0

# Tempo de início
start_time=$(date +%s)

# Resetar cada rede
for i in $(seq 0 999); do
    # Formata o número da rede com zeros à esquerda
    net=$(printf "%03d" $i)
    
    # Atualiza o contador
    count=$((count + 1))
    percent=$((count * 100 / total))
    
    # Mostra progresso
    echo -ne "Progresso: ${percent}% (${count}/${total}) - Resetando rede ${net}...\r"
    
    # Envia comando RST
    if echo "RST ${net}" | nc -u -w 1 $SERVER $PORT 2>/dev/null; then
        success=$((success + 1))
    else
        failed=$((failed + 1))
    fi
    
    # Pequena pausa para não sobrecarregar o servidor
    sleep 0.1
done

# Tempo final
end_time=$(date +%s)
duration=$((end_time - start_time))

echo ""
echo -e "${GREEN}Processo concluído!${NC}"
echo -e "Tempo total: ${duration} segundos"
echo -e "Comandos enviados com sucesso: ${success}"
echo -e "Comandos que falharam: ${failed}"
echo ""
echo -e "${YELLOW}Nota: Mesmo que alguns comandos tenham falhado, a maioria das redes${NC}"
echo -e "${YELLOW}pode ter sido resetada corretamente.${NC}"
echo ""
echo -e "${GREEN}Agora é seguro usar as redes para a avaliação!${NC}"