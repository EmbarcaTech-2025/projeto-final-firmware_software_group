# Proposta de Arquitetura do Sistema: Mala Autônoma Seguidora

## Diagrama de Hardware
 
A arquitetura de hardware da mala autônoma é projetada para ser modular, de baixo custo e eficiente, utilizando o **Raspberry Pi Pico W** como núcleo de controle. O diagrama a seguir ilustra os principais componentes e suas conexões:
![diagrama de hardware](diagrama_de_hardware.jpg)

### Descrição das Conexões
- **Raspberry Pi Pico W**: Microcontrolador principal, gerencia todas as tarefas via FreeRTOS, processa dados de sensores, executa TinyML para visão computacional e se comunica via Wi-Fi (HTTP/MQTT).
- **Câmera OV7670**: Conectada via SPI/PIO para captura de imagens, usada para seguimento do usuário com TinyML.
- **Giroscópio MPU6050**: Conectado via I2C, fornece dados de orientação para correção de trajetória.
- **GPS NEO-6M**: Conectado via UART, envia dados de localização para rastreamento remoto.
- **Sensor Ultrassônico HC-SR04**: Conectado via GPIO, detecta obstáculos frontais.
- **Sensor de Toque TTP223**: Conectado via GPIO com interrupção, para parada imediata ao toque.
- **Ponte H TB6612FNG**: Controla dois motores DC para movimentação, conectada via GPIO (PWM).
- **Amplificador MAX98357**: Conectado via I2S, emite alertas sonoros.
- **LEDs RGB**: Conectados via GPIO, indicam estados do sistema (seguindo, parado, erro, etc.).
- **Bateria Li-ion 7.4V**: Alimenta o sistema com conversores para 5V e 3.3V, garantindo autonomia mínima de 1 hora.

---

## Blocos Funcionais

Os blocos funcionais representam os módulos principais do sistema, divididos em hardware e software, com interdependências claras:

1. **Módulo de Visão Computacional**:
   - **Função**: Processa imagens da OV7670 para identificar e seguir o usuário.
   - **Componentes**: Câmera OV7670, TinyML no Raspberry Pi Pico W.
   - **Saída**: Dados de posição relativa do usuário.

2. **Módulo de Navegação e Controle**:
   - **Função**: Controla motores para movimento e correção de trajetória.
   - **Componentes**: Ponte H TB6612FNG, motores DC, giroscópio MPU6050.
   - **Saída**: Movimentação suave e ajustes baseados em orientação.

3. **Módulo de Detecção de Obstáculos**:
   - **Função**: Detecta obstáculos e evita colisões.
   - **Componentes**: Sensor ultrassônico HC-SR04, dados da câmera OV7670.
   - **Saída**: Sinal para parada ou desvio.

4. **Módulo de Interação Humana**:
   - **Função**: Responde a toques e exibe estados do sistema.
   - **Componentes**: Sensor TTP223, LEDs RGB, amplificador MAX98357, alto-falante.
   - **Saída**: Parada imediata, alertas sonoros, indicadores visuais.

5. **Módulo de Comunicação Remota**:
   - **Função**: Envia posição GPS e recebe comandos remotos.
   - **Componentes**: GPS NEO-6M, Wi-Fi do Raspberry Pi Pico W.
   - **Saída**: Dados de localização via HTTP/MQTT.

6. **Módulo de Gerenciamento de Energia**:
   - **Função**: Fornece energia estável e eficiente.
   - **Componentes**: Bateria Li-ion, conversores de tensão.
   - **Saída**: Alimentação para todos os componentes.

---

## Fluxograma do Software

O software é estruturado com **FreeRTOS** para gerenciar tarefas concorrentes em tempo real. O fluxograma a seguir descreve o fluxo principal:


### Detalhamento das Tarefas
1. **Task de Visão Computacional**:
   - Prioridade alta, executa a cada 100ms.
   - Captura imagens, processa com modelo TinyML, atualiza posição do usuário.

2. **Task de Navegação**:
   - Prioridade média, executa a cada 50ms.
   - Lê giroscópio, ajusta PWM dos motores para seguir o usuário.

3. **Task de Detecção de Obstáculos**:
   - Prioridade alta, executa a cada 50ms.
   - Integra dados do HC-SR04 e visão para evitar colisões.

4. **Task de Interação Humana**:
   - Prioridade média, executa sob demanda (interrupção do TTP223).
   - Atualiza LEDs e reproduz alertas sonoros.

5. **Task de Comunicação Remota**:
   - Prioridade baixa, executa a cada 5s.
   - Envia dados GPS via HTTP/MQTT.

### Considerações de Software
- **FreeRTOS**: Garante execução concorrente e determinística.
- **Interrupções**: Usadas para sensor de toque (alta prioridade).
- **TinyML**: Modelo otimizado para baixo consumo, treinado previamente.
- **Comunicação**: Wi-Fi configurado para HTTP ou MQTT, com buffer para dados GPS.
- **Segurança**: Timeout para falhas de sensores, parada de emergência.

---

## Considerações Finais
A arquitetura proposta é modular e escalável, permitindo futuras expansões (ex.: controle por app, comandos por voz). O uso do FreeRTOS garante eficiência em tempo real, enquanto o hardware de baixo custo mantém o projeto acessível. A integração de TinyML com sensores tradicionais oferece robustez ao seguimento e desvio de obstáculos.