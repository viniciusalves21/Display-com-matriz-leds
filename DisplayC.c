#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/time.h"
#include "pico/bootrom.h"

#include "inc/ssd1306.h" // Biblioteca para o display OLED
#include "inc/font.h" // Biblioteca para as fontes

#include "build/pio_matrix.pio.h" // Arquivo gerado pelo pioasm para controlar a matriz de LEDs


// Definições dos pinos e endereços
#define I2C_PORT        i2c1 // Porta I2C utilizada
#define I2C_SDA         14 // Pino SDA do I2C
#define I2C_SCL         15 // Pino SCL do I2C
#define OLED_ADDR       0x3C // Endereço I2C do display OLED
#define WIDTH           128 // Largura do display OLED
#define HEIGHT          64 // Altura do display OLED

// LEDs e botões
#define LED_G   11 // LED Verde
#define LED_B   12 // LED Azul
#define LED_R   13 // LED Vermelho (opcional nesse codigo)
#define BTN_A   5 // Botão A
#define BTN_B   6 // Botão B

// WS2812 (Matriz de LEDs)
#define OUT_PIN         7 // Pino de saída para a matriz WS2812
#define NUM_PIXELS      25 // Número de LEDs na matriz

static volatile bool ledG_state = false; // Estado do LED verde
static volatile bool ledB_state = false; // Estado do LED azul
static volatile bool need_display_update = false; // Sinaliza ao loop principal para atualizar o display

// Debounce (para evitar leituras múltiplas do botão)
static volatile uint32_t ultimo_tempo_a = 0; // Tempo da última leitura do botão A
static volatile uint32_t ultimo_tempo_b = 0; // Tempo da última leitura do botão B
const uint32_t debounce_delay_us = 200000; // Tempo de debounce em microssegundos (200ms)

static ssd1306_t ssd; // Instância da estrutura do display OLED

// Função para converter cores RGB para o formato da matriz WS2812
uint32_t matrix_rgb(double r, double g, double b) {
    double brilho = 0.1; // Ajuste de brilho
    unsigned char R = (unsigned char)(r * 255 * brilho);
    unsigned char G_ = (unsigned char)(g * 255 * brilho);
    unsigned char B_ = (unsigned char)(b * 255 * brilho);
    // Formato para a matriz: GRB
    return ((uint32_t)(G_) << 24) | ((uint32_t)(R) << 16) | ((uint32_t)(B_) << 8);
}

// Função para exibir um número [0..9] na matriz 5x5 de LEDs
void exibir_numero(PIO pio, uint sm, int numero) {
    // Matriz com os padrões de bits para cada número
    static uint32_t numeros[10][NUM_PIXELS] = {
        // 0
        {0,1,1,1,0, 1,0,0,0,1, 1,0,0,0,1, 1,0,0,0,1, 0,1,1,1,0},
        // 1
        {0,1,1,1,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,1,0, 0,0,1,0,0},
        // 2
        {1,1,1,1,1, 0,0,0,0,1, 0,1,1,1,0, 1,0,0,0,0, 1,1,1,1,1},
        // 3
        {1,1,1,1,1, 1,0,0,0,0, 0,1,1,1,0, 1,0,0,0,0, 1,1,1,1,1},
        // 4
        {0,0,0,0,1, 1,0,0,0,0, 1,1,1,1,1, 1,0,0,0,1, 1,0,0,0,1},
        // 5
        {1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1},
        // 6
        {1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1},
        // 7
        {1,0,0,0,0, 0,0,0,1,0, 0,0,1,0,0, 0,1,0,0,0, 1,1,1,1,1},
        // 8
        {1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1},
        // 9
        {1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1}
    };
    for (int i = 0; i < NUM_PIXELS; i++) {
        // Espelhamento horizontal dos LEDs (para corrigir a ordem física)
        int mirrored_index = (i / 5) * 5 + (4 - (i % 5));
        uint32_t val = numeros[numero][mirrored_index];
        pio_sm_put_blocking(pio, sm, matrix_rgb(val, val, val)); // Envia a cor para o LED
    }
}


// Função de callback para as interrupções dos botões
void gpio_callback(uint gpio, uint32_t events) {
    uint32_t agora = time_us_32(); // Tempo atual em microssegundos

    if (gpio == BTN_A) {
        if ((agora - ultimo_tempo_a) > debounce_delay_us) { // Verifica debounce
            ultimo_tempo_a = agora;
            ledG_state = !ledG_state; // Inverte o estado do LED verde
            gpio_put(LED_G, ledG_state); // Atualiza o LED verde
            printf("Botao A pressionado. LED Verde %s\n", ledG_state ? "LIGADO" : "DESLIGADO");
            need_display_update = true; // Marca para atualizar o display
        }
    } else if (gpio == BTN_B) {
        if ((agora - ultimo_tempo_b) > debounce_delay_us) { // Verifica debounce
            ultimo_tempo_b = agora;
            ledB_state = !ledB_state; // Inverte o estado do LED azul
            gpio_put(LED_B, ledB_state); // Atualiza o LED azul
            printf("Botao B pressionado. LED Azul %s\n", ledB_state ? "LIGADO" : "DESLIGADO");
            need_display_update = true; // Marca para atualizar o display
        }
    }
}

// Função para atualizar as informações no display OLED
void atualiza_display_com_informacoes(char digitado) {
    ssd1306_fill(&ssd, false); // Limpa o display

    ssd1306_rect(&ssd, 3, 3, 122, 58, true, false); // Desenha um retângulo

    char str[2] = {digitado, '\0'}; // Caractere digitado
    ssd1306_draw_string(&ssd, str, 10, 10); // Exibe o caractere

    // Exibe o estado dos LEDs
    ssd1306_draw_string(&ssd, "LED Verde:", 10, 30);
    ssd1306_draw_string(&ssd, ledG_state ? "ON " : "OFF", 80, 30); // Estado do LED Verde

    ssd1306_draw_string(&ssd, "LED Azul :", 10, 45);
    ssd1306_draw_string(&ssd, ledB_state ? "ON " : "OFF", 80, 45); // Estado do LED Azul

    ssd1306_send_data(&ssd); // Envia os dados para o display
}

int main() {
    stdio_init_all(); // Inicializa a comunicação serial

    // Inicializa o I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, OLED_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_send_data(&ssd);

    // Inicializa a matriz de LEDs WS2812
    PIO pio = pio0;
    uint sm = pio_claim_unused_sm(pio, true); // Aloca um state machine não utilizado
    uint offset = pio_add_program(pio, &pio_matrix_program); // Carrega o programa PIO
    pio_matrix_program_init(pio, sm, offset, OUT_PIN); // Inicializa o state machine

    // Apaga todos os LEDs da matriz
    for (int i = 0; i < NUM_PIXELS; i++) {
        pio_sm_put_blocking(pio, sm, matrix_rgb(0, 0, 0));
    }

    // Inicializa o LED Verde
    gpio_init(LED_G); // Inicializa o pino LED_G
    gpio_set_dir(LED_G, GPIO_OUT); // Define o pino LED_G como saída
    gpio_put(LED_G, ledG_state); // Define o estado inicial do LED Verde (ligado ou desligado)

    // Inicializa o LED Azul
    gpio_init(LED_B); // Inicializa o pino LED_B
    gpio_set_dir(LED_B, GPIO_OUT); // Define o pino LED_B como saída
    gpio_put(LED_B, ledB_state); // Define o estado inicial do LED Azul

    // Inicializa o LED Vermelho (opcional)
    gpio_init(LED_R); // Inicializa o pino LED_R
    gpio_set_dir(LED_R, GPIO_OUT); // Define o pino LED_R como saída
    gpio_put(LED_R, 0); // Desliga o LED Vermelho (nível lógico 0)

    // Inicializa o Botão A
    gpio_init(BTN_A); // Inicializa o pino BTN_A
    gpio_set_dir(BTN_A, GPIO_IN); // Define o pino BTN_A como entrada
    gpio_pull_up(BTN_A); // Habilita o resistor pull-up interno do pino BTN_A (mantém o pino em nível alto quando o botão não está pressionado)

    // Inicializa o Botão B
    gpio_init(BTN_B); // Inicializa o pino BTN_B
    gpio_set_dir(BTN_B, GPIO_IN); // Define o pino BTN_B como entrada
    gpio_pull_up(BTN_B); // Habilita o resistor pull-up interno do pino BTN_B

    // Habilita interrupções para os botões
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback); // Habilita interrupção na borda de descida (quando o botão é pressionado) para o botão A, usando a função gpio_callback
    gpio_set_irq_enabled(BTN_B, GPIO_IRQ_EDGE_FALL, true); // Habilita interrupção na borda de descida para o botão B, também usando gpio_callback

    // Loop principal
    while (true) {
        // Lê um caractere da entrada serial
        int ch = getchar_timeout_us(0); // Tenta ler um caractere, sem esperar (timeout de 0 microssegundos)
        if (ch != PICO_ERROR_TIMEOUT && ch != EOF) { // Verifica se um caractere foi lido
            char c = (char)ch; // Converte o valor lido para um caractere
            printf("digitado: %c\n", c); // Imprime o caractere digitado

            // Se o caractere for um dígito (0-9)
            if (c >= '0' && c <= '9') {
                int num = c - '0'; // Converte o caractere para um número inteiro (ex: '5' - '0' = 5)
                exibir_numero(pio, sm, num); // Exibe o número na matriz de LEDs
            }

            atualiza_display_com_informacoes(c); // Atualiza o display OLED com o caractere digitado e o estado dos LEDs
        }

        // Se a flag need_display_update estiver definida (um botão foi pressionado)
        if (need_display_update) {
            need_display_update = false; // Reseta a flag
            atualiza_display_com_informacoes(' '); // Atualiza o display OLED (' ' indica que nenhum caractere novo foi digitado)
        }

        sleep_ms(50); // Pausa por 50 milissegundos (para evitar uso excessivo da CPU)
    }

    return 0;
}