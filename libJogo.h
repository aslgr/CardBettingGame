#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

// A maior mensagem será quando o carteador passar 10 cartas
// para um determinado jogador...
// Como precisamos de 1 byte para representar uma carta,
// teremos 8*10 = 80 bits
#define TAM_DADOS 10 // 10 bytes = 80 bits
#define GERADOR 0x07; // o bit mais significativo é implicitamente 1

// tipos:
// cartas               -> 000 : Carteador enviando as cartas para um jogador
// palpites             -> 001 : Carteador coletando o palpite dos jogadores
// mostrar_palpites     -> 010 : Carteador mostra os palpites aos jogadores
// jogadas              -> 011 : Carteador coletando a jogada dos jogadores
// mostrar_jogadas      -> 100 : Carteador mostra as jogadas aos jogadores
// divulga_vencedor     -> 101 : Carteador divulga o vencedor da rodada
// divulga_vidas        -> 110 : Carteador divulga as vidas dos jogadores
// bastão               -> 111 : Carteador passando o bastão

typedef struct
{
    unsigned char marcadorInicio;               // 0111 1110
    unsigned char destino: 2;                   // 2 bits para representar os 4 jogadores
    unsigned char origem: 2;                    // 2 bits para representar os 4 jogadores
    unsigned char tamanho: 4;                   // 3 bits para representar os 5 bytes
    unsigned char tipo: 3;                      // 3 bits para representar os 6 tipos
    unsigned char dados[TAM_DADOS];             
    unsigned char crc8;                         // Detecção de erros
    unsigned char recebido: 1;                  // bit indicando se mensagem chegou no destino
    unsigned char sucesso: 1;                   // bit indicando se mensagem chegou íntegra

} frame_t;

// Estrutura para representar uma carta
typedef struct 
{
    unsigned char valor;
    unsigned char naipe;

} carta_t;

// Limpa a tela
void limpa_tela();

// Escolhe aleatoriamente cartas para distribuir entre os jogadores
unsigned char *escolhe_cartas(unsigned char numero_cartas, unsigned char *cartas_escolhidas);

// Monta a mensagem baseada na estrutura do enunciado e nos parâmetros passados
frame_t monta_mensagem(unsigned char dest, unsigned char orig, unsigned char tam, unsigned char tipo, unsigned char* dados, int crc_flag);

// Envia a mensagem e aguarda o retorno
int envia_mensagem(int sockfd, struct sockaddr_in next_machine_addr, socklen_t addr_len, frame_t *frame, frame_t *frame_resp);

// Imprime a carta de uma maneira inteligível
void print_carta(carta_t carta);

// Imprime o frame na tela
void print_frame(frame_t *frame);

// Calcula o CRC-8 da mensagem e o retorna
unsigned char calcula_crc(frame_t *frame);

// Detecta erros no frame a partir do crc
int verifica_crc(frame_t *frame);

// Analisa se a mensagem é válida e se é "cartas"
int eh_cartas(frame_t *frame);

// Analisa se a mensagem é válida e se é "palpites"
int eh_palpites(frame_t *frame);

// Analisa se a mensagem é válida e se é "mostrar_palpites"
int eh_mostrar_palpites(frame_t *frame);

// Analisa se a mensagem é válida e se é "jogadas"
int eh_jogadas(frame_t *frame);

// Analisa se a mensagem é válida e se é "mostrar_jogadas"
int eh_mostrar_jogadas(frame_t *frame);

// Analisa se a mensagem é válida e se é "divulga_vencedor"
int eh_divulga_vencedor(frame_t *frame);

// Analisa se a mensagem é válida e se é "divulga_vidas"
int eh_divulga_vidas(frame_t *frame);

// Analisa se a mensagem é válida e se é "bastão"
int eh_bastao(frame_t *frame);
