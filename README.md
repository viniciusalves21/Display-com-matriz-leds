# Projeto Display OLED e Matriz de LEDs WS2812 com Raspberry Pi Pico (Placa BitDogLab)

Este projeto demonstra a integração de um display OLED e uma matriz de LEDs WS2812 controlados por um Raspberry Pi Pico. O código permite exibir caracteres no display OLED, acionar LEDs RGB e controlar uma matriz de LEDs WS2812, exibindo números de 0 a 9.

## Descrição

O projeto combina as seguintes funcionalidades:

- **Display OLED:** Exibe informações textuais, como o caractere digitado e o estado dos LEDs.
- **Matriz de LEDs WS2812:** Exibe números de 0 a 9, fornecendo uma interface visual interativa.
- **Controle de LEDs RGB:** Permite ligar/desligar LEDs RGB através de botões.
- **Interface Serial:** Permite a interação com o usuário através da porta serial.

## Como Usar

1. Abra um terminal serial (ex: Putty, Minicom) e conecte-se à porta serial do Raspberry Pi Pico.
2. Digite um caractere (qualquer um) no terminal e pressione Enter. O caractere será exibido no display OLED.
3. Digite um número de 0 a 9 e pressione Enter. O número correspondente será exibido na matriz de LEDs WS2812.
4. Pressione o botão A para ligar/desligar o LED Verde. O estado do LED será exibido no display OLED.
5. Pressione o botão B para ligar/desligar o LED Azul. O estado do LED será exibido no display OLED.

## Observações

- Os botões utilizam o pull-up interno do Raspberry Pi Pico.
- O código inclui tratamento de debounce para os botões.
- O código utiliza interrupções para os botões, o que permite que o programa execute outras tarefas enquanto aguarda o pressionamento dos botões.

## Videos

- Explicação das principais funcionalidades: [https://youtu.be/juP8_iw_BTo]
- Funcionamento na Placa BitDogLab: [https://drive.google.com/file/d/1iPnxkonFXl_8CyNEJJSiCKJ1Qq8xjBxu/view?usp=sharing]
