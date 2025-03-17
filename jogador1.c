#include "libJogo.h"

#define MY_PORT 26583
#define NEXT_MACHINE_PORT 26584

int main() 
{
    // Inicializa estrutura da rede...

    int sockfd;
    struct sockaddr_in my_addr, next_machine_addr;
    unsigned char dados[TAM_DADOS];
    socklen_t addr_len = sizeof(struct sockaddr_in);

    // Criação do socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configuração do endereço da máquina atual
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(MY_PORT);

    // Vinculação do socket ao endereço da máquina atual
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Configuração do endereço da próxima máquina
    memset(&next_machine_addr, 0, sizeof(next_machine_addr));
    next_machine_addr.sin_family = AF_INET;
    next_machine_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    next_machine_addr.sin_port = htons(NEXT_MACHINE_PORT);

    // Variáveis necessárias:

    frame_t frame, frame_resp;

    unsigned char *cartas_valor, *cartas_escolhidas, 
    vidas[4], palpites_rodada[4], jogadas_rodada[4], vitorias_rodada[4], 
    destino_msg, palpite, jogada, vencedor_rodada,
    todos_vivos = 1, flag_crescente = 0, num_cartas = 10, bastao = 0xFF, id_jogador = 0;
    
    carta_t *cartas_representacao, carta_adversario, maior, atual;

    for (unsigned char i = 0; i < 4; i++)
    {
        vidas[i] = 10;
        printf("Vidas jogador %d: %d\n", (int)(i+1), (int)vidas[i]);
    }
    printf("\n");

    while (todos_vivos)
    {
        // Carteador
        if (bastao == 0xFF)
        {
            srand(time(NULL));
            cartas_escolhidas = (unsigned char*)calloc(40, sizeof(unsigned char));

            // Caso esteja no limite inferior ou superior do número de cartas
            if (num_cartas == 0)
            {
                num_cartas++;
                flag_crescente = 1;
            }
            else if (num_cartas == 11)
            {
                num_cartas--;
                flag_crescente = 0;
            }

            cartas_representacao = (carta_t*)malloc(num_cartas*sizeof(carta_t));

            // Laço para entrega de cartas
            for (unsigned char i = 1; i < 4; i++)
            {
                cartas_valor = escolhe_cartas(num_cartas, cartas_escolhidas);

                destino_msg = (id_jogador + i) % 4;

                frame = monta_mensagem(destino_msg, id_jogador, num_cartas, 0X00, cartas_valor, 1);

                // print_frame(&frame);

                if (!envia_mensagem(sockfd, next_machine_addr, addr_len, &frame, &frame_resp))
                    return EXIT_FAILURE;

                free(cartas_valor);
            }

            cartas_valor = escolhe_cartas(num_cartas, cartas_escolhidas);

            printf("NOVA RODADA -------------------------\n\nSuas cartas são: \n");
            for (unsigned char i = 0; i < num_cartas; i++)
            {
                cartas_representacao[i].valor = cartas_valor[i] % 10;
                cartas_representacao[i].naipe = cartas_valor[i] / 10;
                printf("%d - ", (int)(i+1));
                print_carta(cartas_representacao[i]);
                printf("\n");
            }
            printf("\n");

            // Laço para a coleta de palpites
            for (unsigned char i = 1; i < 4; i++)
            {
                do {
                
                    destino_msg = (id_jogador + i) % 4;

                    frame = monta_mensagem(destino_msg, id_jogador, 0x00, 0X01, 0x00, 1);

                    // print_frame(&frame);

                    if (!envia_mensagem(sockfd, next_machine_addr, addr_len, &frame, &frame_resp))
                        return EXIT_FAILURE;

                    // print_frame(&frame_resp);
                
                } while (!verifica_crc(&frame_resp));

                palpites_rodada[destino_msg] = frame_resp.dados[0];
            }

            printf("Digite quantas rodadas você acredita que vai ganhar: ");
            scanf("%hhu", &palpite);

            while (palpite < 0 || palpite > num_cartas)
            {
                printf("Por favor, escolha um palpite possível: ");
                scanf("%hhu", &palpite);
            }

            palpites_rodada[id_jogador] = palpite;

            // Laço para a divulgação dos palpites
            for (unsigned char i = 1; i < 4; i++)
            {
                destino_msg = (id_jogador + i) % 4;

                frame = monta_mensagem(destino_msg, id_jogador, 0x04, 0X02, palpites_rodada, 1);

                // print_frame(&frame);

                if (!envia_mensagem(sockfd, next_machine_addr, addr_len, &frame, &frame_resp))
                    return EXIT_FAILURE;
            }

            printf("\n");
            for (unsigned char i = 0; i < 4; i++)
                printf("O palpite do jogador %d foi %d vitórias!\n", (int)(i+1), (int)palpites_rodada[i]);
            printf("\n");

            // Zera as vitórias de cada jogador para essa nova rodada
            for (unsigned char i = 0; i < 4; i++)
                vitorias_rodada[i] = 0;

            // Enquanto os jogadores tiverem carta para jogar...
            for (unsigned char turno = 0; turno < num_cartas; turno++)
            {
                // Laço para a coleta de jogadas
                for (unsigned char i = 1; i < 4; i++)
                {
                    do {
                    
                        destino_msg = (id_jogador + i) % 4;

                        frame = monta_mensagem(destino_msg, id_jogador, 0x00, 0X03, 0x00, 1);

                        // print_frame(&frame);

                        if (!envia_mensagem(sockfd, next_machine_addr, addr_len, &frame, &frame_resp))
                            return EXIT_FAILURE;

                    } while (!verifica_crc(&frame_resp));

                    jogadas_rodada[destino_msg] = frame_resp.dados[0];

                    dados[0] = destino_msg;
                    dados[1] = frame_resp.dados[0];

                    // Laço para a divulgação da jogada
                    for (unsigned char j = 1; j < 4; j++)
                    {
                        destino_msg = (id_jogador + j) % 4;

                        frame = monta_mensagem(destino_msg, id_jogador, 0x02, 0X04, dados, 1);

                        if (!envia_mensagem(sockfd, next_machine_addr, addr_len, &frame, &frame_resp))
                            return EXIT_FAILURE;
                    }

                    carta_adversario.valor = dados[1] % 10;
                    carta_adversario.naipe = dados[1] / 10;
                    printf("Jogador %d jogou: ", (int)(dados[0]+1));
                    print_carta(carta_adversario);
                    printf("\n");
                }

                printf("\nEscolha uma carta para jogar: ");
                scanf("%hhu", &jogada);
                
                while (jogada < 1 || jogada > num_cartas || cartas_escolhidas[cartas_valor[jogada-1]] == 2)
                {
                    printf("Por favor, escolha uma carta válida para jogar: ");
                    scanf("%hhu", &jogada);
                }
                printf("\n");

                cartas_escolhidas[cartas_valor[jogada-1]] = 2;

                jogadas_rodada[id_jogador] = cartas_valor[jogada-1];

                dados[0] = id_jogador;
                dados[1] = cartas_valor[jogada-1];

                // Laço para a divulgação da jogada
                for (unsigned char i = 1; i < 4; i++)
                {
                    destino_msg = (id_jogador + i) % 4;

                    frame = monta_mensagem(destino_msg, id_jogador, 0x02, 0X04, dados, 1);

                    if (!envia_mensagem(sockfd, next_machine_addr, addr_len, &frame, &frame_resp))
                        return EXIT_FAILURE;
                }

                // Contabiliza vencedor

                vencedor_rodada = 0;
                maior.valor = jogadas_rodada[0] % 10;
                maior.naipe = jogadas_rodada[0] / 10;
                for (unsigned char i = 1; i < 4; i++)
                {
                    atual.valor = jogadas_rodada[i] % 10;
                    atual.naipe = jogadas_rodada[i] / 10;
                    if (atual.valor > maior.valor || (atual.valor == maior.valor && atual.naipe > maior.naipe))
                    {
                        vencedor_rodada = i;
                        maior.valor = atual.valor;
                        maior.naipe = atual.naipe;
                    }
                }

                vitorias_rodada[vencedor_rodada]++;

                dados[0] = vencedor_rodada;

                // Laço para a divulgação do vencedor da rodada
                for (unsigned char i = 1; i < 4; i++)
                {
                    destino_msg = (id_jogador + i) % 4;

                    frame = monta_mensagem(destino_msg, id_jogador, 0x01, 0X05, dados, 1);

                    if (!envia_mensagem(sockfd, next_machine_addr, addr_len, &frame, &frame_resp))
                        return EXIT_FAILURE;
                }

                printf("\nJogador %d ganhou a rodada!\n\n", (int)(vencedor_rodada+1));

                printf("\n-------------------------------------\n\n");

                printf("Suas cartas são: \n");
                for (unsigned char i = 0; i < num_cartas; i++)
                {
                    cartas_representacao[i].valor = cartas_valor[i] % 10;
                    cartas_representacao[i].naipe = cartas_valor[i] / 10;
                    printf("%d - ", (int)(i+1));
                    print_carta(cartas_representacao[i]);

                    if (cartas_escolhidas[cartas_valor[i]] == 2)
                        printf("X");
                    printf("\n");
                    
                }
                printf("\n");

                for (unsigned char i = 0; i < 4; i++)
                {
                    if (i == vencedor_rodada)
                    {
                        if (vitorias_rodada[i] > palpites_rodada[i])
                        {
                            vidas[i]--;
                            if (vidas[i] == 0)
                            {
                                todos_vivos = 0;
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (palpites_rodada[i] - vitorias_rodada[i] > num_cartas - (turno + 1))
                        {
                            palpites_rodada[i]--;
                            vidas[i]--;
                            if (vidas[i] == 0)
                            {
                                todos_vivos = 0;
                                break;
                            }
                        }
                    }
                }

                if(!todos_vivos)
                    break;
            }

            // Laço para a divulgação das vidas dos jogadores
            for (unsigned char i = 1; i < 4; i++)
            {
                destino_msg = (id_jogador + i) % 4;

                frame = monta_mensagem(destino_msg, id_jogador, 0x04, 0X06, vidas, 1);

                if (!envia_mensagem(sockfd, next_machine_addr, addr_len, &frame, &frame_resp))
                    return EXIT_FAILURE;
            }

            for (unsigned char i = 0; i < 4; i++)
            {
                if (vidas[i] == 0)
                {
                    printf("O jogador %d perdeu o jogo!\n", (int)(i+1));
                }
                else
                {
                    printf("O jogador %d tem %d vidas!\n", (int)(i+1), (int)vidas[i]);
                }
            }
            printf("\n");

            // Passa o bastão

            destino_msg = (id_jogador + 1) % 4;

            dados[0] = bastao;

            if (flag_crescente)
            {
                dados[1] = num_cartas++;
            }
            else
            {
                dados[1] = num_cartas--;
            }

            frame = monta_mensagem(destino_msg, id_jogador, 0x02, 0X07, dados, 1);

            if (!envia_mensagem(sockfd, next_machine_addr, addr_len, &frame, &frame_resp))
                return EXIT_FAILURE;

            bastao = 0x00;
            num_cartas = 0;

            free(cartas_valor);
            free(cartas_escolhidas);
            free(cartas_representacao);
        }
        // Jogador normal
        else
        {
            if (recvfrom(sockfd, &frame, sizeof(frame), 0, NULL, NULL) < 0)
            {
                perror("Erro no recebimento:");
                return EXIT_FAILURE;
            }

            if (frame.destino == id_jogador)
            {
                frame.recebido = 1;

                if (verifica_crc(&frame))
                {
                    frame.sucesso = 1;

                    // O jogador recebe suas cartas para a rodada
                    if (eh_cartas(&frame))
                    {
                        num_cartas = frame.tamanho;

                        cartas_escolhidas = (unsigned char*)calloc(40, sizeof(unsigned char));
                        cartas_valor = (unsigned char*)malloc(num_cartas * sizeof(unsigned char));

                        for (unsigned char i = 0; i < num_cartas; i++)
                            cartas_valor[i] = frame.dados[i];

                        cartas_representacao = (carta_t*)malloc(num_cartas*sizeof(carta_t));

                        printf("NOVA RODADA -------------------------\n\nSuas cartas são: \n");
                        for (unsigned char i = 0; i < num_cartas; i++)
                        {
                            cartas_representacao[i].valor = cartas_valor[i] % 10;
                            cartas_representacao[i].naipe = cartas_valor[i] / 10;
                            printf("%d - ", (int)(i+1));
                            print_carta(cartas_representacao[i]);
                            printf("\n");
                        }
                        printf("\n");
                    }

                    // O jogador da seu palpite
                    else if (eh_palpites(&frame))
                    {
                        printf("Digite quantas rodadas você acredita que vai ganhar: ");
                        scanf("%hhu", &palpite);

                        while (palpite < 0 || palpite > num_cartas)
                        {
                            printf("Por favor, escolha um palpite possível: ");
                            scanf("%hhu", &palpite);
                        }

                        frame.dados[0] = palpite;
                        frame.crc8 = calcula_crc(&frame);
                    }

                    // O jogador recebe os palpites dos outros jogadores
                    else if (eh_mostrar_palpites(&frame))
                    {
                        printf("\n");
                        for (unsigned char i = 0; i < 4; i++)
                        {
                            palpites_rodada[i] = frame.dados[i];
                            printf("O palpite do jogador %d foi %d vitórias!\n", (int)(i+1), (int)frame.dados[i]);
                        }
                        printf("\n");
                    }

                    // O jogador escolhe uma carta para jogar
                    else if (eh_jogadas(&frame))
                    {
                        printf("\nEscolha uma carta para jogar: ");
                        scanf("%hhu", &jogada);

                        while (jogada < 1 || jogada > num_cartas || cartas_escolhidas[cartas_valor[jogada-1]])
                        {
                            printf("Por favor, escolha uma carta válida para jogar: ");
                            scanf("%hhu", &jogada);
                        }
                        printf("\n");

                        frame.dados[0] = cartas_valor[jogada-1];
                        frame.crc8 = calcula_crc(&frame);
                    }

                    // O jogador recebe as jogadas dos outros jogadores
                    else if (eh_mostrar_jogadas(&frame))
                    {
                        jogadas_rodada[frame.dados[0]] = frame.dados[1];

                        // Imprime a escolha dos outros jogadores apenas
                        if (frame.dados[0] != id_jogador)
                        {
                            carta_adversario.valor = frame.dados[1] % 10;
                            carta_adversario.naipe = frame.dados[1] / 10;
                            printf("Jogador %d jogou: ", (int)(frame.dados[0]+1));
                            print_carta(carta_adversario);
                        }
                        printf("\n");
                    }

                    // Divulgação de quem ganhou o turno
                    else if (eh_divulga_vencedor(&frame))
                    {
                        printf("\nJogador %d ganhou a rodada!\n", (int)(frame.dados[0]+1));

                        cartas_escolhidas[cartas_valor[jogada-1]] = 1;

                        printf("\n-------------------------------------\n\n");

                        printf("Suas cartas são: \n");
                        for (unsigned char i = 0; i < num_cartas; i++)
                        {
                            cartas_representacao[i].valor = cartas_valor[i] % 10;
                            cartas_representacao[i].naipe = cartas_valor[i] / 10;
                            printf("%d - ", (int)(i+1));
                            print_carta(cartas_representacao[i]);

                            if (cartas_escolhidas[cartas_valor[i]])
                                printf("X");
                            printf("\n");
                            
                        }
                        printf("\n");
                    }

                    // Divulgação da vida de cada jogador após uma rodada
                    else if (eh_divulga_vidas(&frame))
                    {
                        for (unsigned char i = 0; i < 4; i++)
                        {
                            if ((vidas[i] = frame.dados[i]) == 0)
                            {
                                printf("O jogador %d perdeu o jogo!\n", (int)(i+1));
                                todos_vivos = 0;
                            }
                            else
                            {
                                printf("O jogador %d tem %d vidas!\n", (int)(i+1), (int)vidas[i]);
                            }
                        }
                        printf("\n"); 

                        // Libera pois é o fim da rodada
                        free(cartas_valor);
                        free(cartas_escolhidas);
                        free(cartas_representacao);
                    }

                    // O jogador vira o carteador
                    else if (eh_bastao(&frame))
                    {
                        bastao = frame.dados[0];
                        num_cartas = frame.dados[1];
                    }
                }
            }

            if (sendto(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *)&next_machine_addr, addr_len) < 0)
            {
                perror("Erro no envio:");
                return EXIT_FAILURE;
            }
        }
    }

    close(sockfd);
    return 0;
}