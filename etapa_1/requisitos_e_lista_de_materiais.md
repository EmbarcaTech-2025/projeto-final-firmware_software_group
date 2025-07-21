# Projeto: **Mala Autônoma Seguidora com Visão Computacional e Comunicação Remota**

## 🎯 Problema a Ser Resolvido

Em aeroportos, eventos e ambientes urbanos, pessoas frequentemente enfrentam dificuldades ao transportar bagagens por longas distâncias ou em terrenos irregulares. Isso se agrava para idosos, pessoas com deficiência ou quando há múltiplas malas envolvidas.

**Objetivo**: Desenvolver uma mala autônoma que **segue seu dono de forma segura e inteligente**, desviando de obstáculos, respondendo a comandos por toque e permitindo rastreamento remoto via GPS.

---

## ✅ Requisitos Funcionais

1. **Seguimento autônomo** do usuário utilizando **TinyML** e câmera OV7670.
2. **Desvio de obstáculos** com sensores ultrassônicos e visão computacional.
3. **Parada imediata** ao toque humano (sensor capacitivo).
4. **Reprodução de alertas sonoros** (por exemplo, quando parada, em erro ou ao localizar o usuário).
5. **Envio periódico da posição GPS** ao dono via Wi-Fi (HTTP ou MQTT).
6. **Correção de trajetória** usando giroscópio.
7. **Indicação do estado do sistema** por meio de LEDs (seguindo, parado, erro, etc.).
8. **Execução concorrente** com tarefas distribuídas em tempo real utilizando **FreeRTOS**.

---

## ❌ Requisitos Não Funcionais

1. **Formato portátil e leve**, com estrutura em 3D no estilo mala.
2. **Autonomia mínima de 1 hora**, com bateria recarregável.
3. **Alta segurança operacional**, com comportamento previsível e interrupção segura em caso de falha.
4. **Custo reduzido**, utilizando componentes acessíveis e reutilizáveis.
5. **Arquitetura modular** de hardware e software, facilitando manutenção e expansão futura (ex: controle por app).
6. **Qualidade de som suficiente** para alertas claros (via I2S digital).
7. **Interface amigável** com configuração simples e manutenção fácil.

---

## 🧰 Lista de Materiais Necessários

### 🧩 Kit Base

* ✅ **BitDogLab Kit**

  * Raspberry Pi Pico W (com Wi-Fi integrado)
  * Protoboard, jumpers e microfone embutido

### ⚙️ Eletrônicos e Sensores

* ✅ **Ponte H TB6612FNG** – controle de dois motores DC
* ✅ **Motores DC + rodas** (x2) e **roda livre**
* ✅ **Sensor ultrassônico HC-SR04** – detecção de obstáculos
* ✅ **Sensor de toque capacitivo (TTP223)** – parada ao toque
* ✅ **Câmera OV7670** – visão computacional (TinyML embarcado)
* ✅ **Módulo GPS (ex: NEO-6M)** – rastreamento geográfico
* ✅ **Giroscópio (MPU-6050 ou compatível)** – orientação e equilíbrio
* ✅ **Amplificador MAX98357 (I2S)** – saída de áudio digital
* ✅ **Micro alto-falante** – emissão de sons de status
* ✅ **LEDs RGB ou simples** – indicadores visuais de status

### 🔋 Energia e Estrutura

* ✅ **Bateria Li-ion 7.4V** ou **Power Bank USB-C**
* ✅ **Conversores de tensão** (5V e 3.3V, conforme periféricos)
* ✅ **Estrutura física 3D** no formato de uma mala pequena (impressão 3D)

---

## 💻 Ambiente e Tecnologias

* **Linguagem de Programação**: C
* **Plataforma de Desenvolvimento**: Linux + Sublime Text
* **SDK**: Raspberry Pi Pico C SDK
* **RTOS**: FreeRTOS
* **Compilador**: `arm-none-eabi-gcc` + CMake
* **Ferramentas**:

  * `picotool` (upload USB)
  * `openocd` (debug SWD, opcional)

### 📡 Comunicação dos Módulos

| Módulo                | Interface          |
| --------------------- | ------------------ |
| Câmera OV7670         | SPI / PIO          |
| GPS (NEO-6M)          | UART               |
| Giroscópio (MPU6050)  | I2C                |
| Sensor de toque       | GPIO + interrupção |
| Amplificador MAX98357 | I2S                |

---

## 📌 Considerações Finais

* O microfone do kit pode ser utilizado em versões futuras para comandos por voz ou detecção de ambiente.
* O projeto se baseia em princípios de modularidade e eficiência energética.
* A TinyML será usada com modelos previamente treinados e otimizados para o Raspberry Pi Pico W.
* O sistema será multitarefa com FreeRTOS, com tasks independentes para:

  * Visão (seguimento)
  * Controle motor
  * Leitura de sensores
  * Comunicação (HTTP/MQTT)
  * Reprodução de áudio
  * LED/status
