#include "libJogo.h"

void limpa_tela()
{
    system("clear");
}

unsigned char *escolhe_cartas(unsigned char numero_cartas, unsigned char *cartas_escolhidas)
{
    unsigned char *cartas, valor;
    cartas = (unsigned char*)malloc(numero_cartas * sizeof(unsigned char));

    unsigned char i = 0;
    while (i < numero_cartas)
    {
        do {
            valor = rand() % 40;
        } while (cartas_escolhidas[valor] != 0);

        cartas_escolhidas[valor] = 1;
        cartas[i] = valor;

        i++;
    }

    return cartas;
}

frame_t monta_mensagem(unsigned char dest, unsigned char orig, unsigned char tam, unsigned char tipo, unsigned char* dados, int crc_flag)
{
    frame_t frame;

    frame.marcadorInicio = 0x7E; // (0111 1110)
    frame.destino = dest;
    frame.origem = orig;
    frame.tamanho = tam;
    frame.tipo = tipo;

    if (dados != NULL)
        memcpy(frame.dados, dados, TAM_DADOS);

    // caso seja necessário, calcula o crc
    if (crc_flag){
        frame.crc8 = calcula_crc(&frame);
    } else {
        frame.crc8 = 0x00;
    }

    frame.recebido = 0;
    frame.sucesso = 0;

    return frame;
}

int envia_mensagem(int sockfd, struct sockaddr_in next_machine_addr, socklen_t addr_len, frame_t *frame, frame_t *frame_resp)
{
    // Envio da mensagem
    // Segue no while enquanto o destinatário não receber com sucesso
    do {

        if (sendto(sockfd, frame, sizeof(*frame), 0, (struct sockaddr *)&next_machine_addr, addr_len) < 0)
        {
            perror("Erro no envio:");
            return 0;
        }

        if (recvfrom(sockfd, frame_resp, sizeof(*frame_resp), 0, NULL, NULL) < 0)
        {
            perror("Erro no recebimento:");
            return 0;
        }

    } while (!frame_resp->recebido || !frame_resp->sucesso);

    return 1;
}

void print_carta(carta_t carta)
{
    switch (carta.valor)
    {
        case 0:
            printf("4 ");
        break;
        case 1:
            printf("5 ");
        break;
        case 2:
            printf("6 ");
        break;
        case 3:
            printf("7 ");
        break;
        case 4:
            printf("Q ");
        break;
        case 5:
            printf("J ");
        break;
        case 6:
            printf("K ");
        break;
        case 7:
            printf("A ");
        break;
        case 8:
            printf("2 ");
        break;
        case 9:
            printf("3 ");
        break;
        
        default:
        break;
    }
    switch (carta.naipe)
    {
        case 0:
            printf("Ouros ");
        break;
        case 1:
            printf("Espadas ");
        break;
        case 2:
            printf("Copas ");
        break;
        case 3:
            printf("Paus ");
        break;
        
        default:
        break;
    }
}

// Função interna para imprimir os bits de cada byte na tela
void print_bits(unsigned char byte, int num_bits) 
{
    for (int i = num_bits - 1; i >= 0; --i)
        printf("%d", (byte >> i) & 1);
}

void print_frame(frame_t *frame) 
{
    printf("\nMarcador de inicio: ");
    print_bits(frame->marcadorInicio, 8);
    printf("\n");

    printf("Destino: ");
    print_bits(frame->destino, 2);
    printf("\n");

    printf("Origem: ");
    print_bits(frame->origem, 2);
    printf("\n");

    printf("Tamanho: ");
    print_bits(frame->tamanho, 4);
    printf("\n");

    printf("Tipo: ");
    print_bits(frame->tipo, 3);
    printf("\n");

    printf("Dados: ");
    for (int i = 0; i < TAM_DADOS; ++i) {
        print_bits(frame->dados[i], 8);
    }
    printf("\n");

    printf("Crc-8: ");
    print_bits(frame->crc8, 8);
    printf("\n\n");

    printf("Recebido: ");
    print_bits(frame->recebido, 1);
    printf("\n");

    printf("Sucesso: ");
    print_bits(frame->sucesso, 1);
    printf("\n");
}

unsigned char laco_crc(unsigned char crc, unsigned char n, unsigned char dados[n])
{
    for (unsigned int i = 0; i < n; i++)
    {
        crc ^= dados[i];

        for (unsigned int j = 0; j < 8; j++)
        {
            if (crc & 0x80){
                crc = (crc << 1) ^ GERADOR;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

unsigned char calcula_crc(frame_t *frame)
{
    unsigned char crc = 0x00; // inicialmente é 0;

    // Combina os campos destino, origem, tamanho e tipo em um array temporário
    unsigned char temp[4];

    // Garante que apenas os bits menos significativos sejam levados em consideração
    temp[0] = (frame->destino & 0x03);
    temp[1] = (frame->origem & 0x03);
    temp[2] = (frame->tamanho & 0x0F);
    temp[3] = (frame->tipo & 0x07);

    crc = laco_crc(crc, 4, temp);
    crc = laco_crc(crc, frame->tamanho, frame->dados);

    return crc;
}

int verifica_crc(frame_t *frame)
{
    if (frame->crc8 == calcula_crc(frame))
        return 1;
    
    return 0;
}

int eh_cartas(frame_t *frame)
{
    if (frame->marcadorInicio != 0x7E || frame->tipo != 0x00)
        return 0;
    return 1;
}

int eh_palpites(frame_t *frame)
{
    if (frame->marcadorInicio != 0x7E || frame->tipo != 0x01)
        return 0;
    return 1;
}

int eh_mostrar_palpites(frame_t *frame)
{
    if (frame->marcadorInicio != 0x7E || frame->tipo != 0x02)
        return 0;
    return 1;
}

int eh_jogadas(frame_t *frame)
{
    if (frame->marcadorInicio != 0x7E || frame->tipo != 0x03)
        return 0;
    return 1;
}

int eh_mostrar_jogadas(frame_t *frame)
{
    if (frame->marcadorInicio != 0x7E || frame->tipo != 0x04)
        return 0;
    return 1;
}

int eh_divulga_vencedor(frame_t *frame)
{
    if (frame->marcadorInicio != 0x7E || frame->tipo != 0x05)
        return 0;
    return 1;
}

int eh_divulga_vidas(frame_t *frame)
{
    if (frame->marcadorInicio != 0x7E || frame->tipo != 0x06)
        return 0;
    return 1;
}

int eh_bastao(frame_t *frame)
{
    if (frame->marcadorInicio != 0x7E || frame->tipo != 0x07)
        return 0;
    return 1;
}