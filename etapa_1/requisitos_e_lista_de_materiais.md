# Projeto: Mala Autônoma Seguidora

## 🧰 Materiais Necessários

### 🔧 Kit Principal
- [x] **BitDogLab Kit** (contém Raspberry Pi Pico W)
  - Raspberry Pi Pico W (com Wi-Fi)
  - Protoboard e jumpers diversos

### 🧩 Componentes Adicionais
- [x] **Ponte H TB6612FNG** — controle de dois motores DC
- [x] **Motores DC com rodas** (x2) + **roda livre**
- [x] **Sensor ultrassônico HC-SR04** — para desvio de obstáculos
- [x] **Sensor de Toque Capacitivo (TTP223)** — para parada ao toque
- [x] **Câmera OV7670 (VGA)** — para visão computacional (TinyML)
- [x] **Módulo GPS (ex: NEO-6M)** — rastreamento geográfico
- [x] **Giroscópio (MPU-6050 ou similar)** — equilíbrio e orientação
- [x] **Amplificador de áudio MAX98357** — reprodução de som via I2S
- [x] **Micro alto-falante** — saída de áudio conectada ao MAX98357
- [x] LEDs para status do sistema
- [x] **Bateria Li-ion 7.4V ou Power Bank**
- [x] Conversores de tensão (5V e 3.3V)
- [x] **Estrutura impressa em 3D** no formato de uma pequena mala

---

## 💻 Ambiente de Desenvolvimento

- **Linguagem**: C
- **Sistema Operacional**: Linux
- **Editor**: Sublime Text
- **SDK**: Raspberry Pi Pico C SDK
- **RTOS**: FreeRTOS
- **TinyML**: Para detecção embarcada
- **Câmera OV7670**: Comunicação via PIO/SPI
- **GPS**: Comunicação via UART
- **Giroscópio**: Comunicação via I2C
- **Sensor de toque**: GPIO com interrupção
- **Amplificador MAX98357**: I2S (áudio digital)
- **Compilação**: CMake + `arm-none-eabi-gcc`
- **Ferramentas**:
  - `picotool` (upload via USB)
  - `openocd` (debug SWD opcional)

---

## ✅ Requisitos Funcionais

1. **Detectar e seguir uma pessoa** com TinyML (usando câmera OV7670).
2. **Evitar obstáculos** com sensores e visão computacional.
3. **Parar ao ser tocada** (sensor capacitivo).
4. **Emitir sons de status** via amplificador MAX98357 e alto-falante.
5. **Enviar posição GPS** ao dono por HTTP ou MQTT.
6. **Corrigir deslocamento/orientação** com giroscópio.
7. **Indicar estados com LEDs** (seguindo, parado, erro, etc.).
8. **Executar tarefas em tempo real** com FreeRTOS (visão, sensores, áudio, comunicação, controle motor).

---

## ❌ Requisitos Não Funcionais

1. **Formato físico portátil**: estrutura 3D compacta em forma de mala.
2. **Autonomia de no mínimo 1 hora de uso** com bateria.
3. **Alta segurança operacional**, parando em falhas ou obstáculos.
4. **Baixo custo e componentes reutilizáveis**.
5. **Arquitetura modular** de código e hardware.
6. **Qualidade sonora básica** para alertas (áudio digital via I2S).
7. **Fácil manutenção** e expansão futura (ex: integração com app).

---

## 📌 Considerações Técnicas

- O **microfone já incluso no kit BitDogLab** pode ser usado futuramente para comandos ou ambiente.
- O **alto-falante** permite reprodução clara de alertas sonoros via **MAX98357** (áudio digital I2S).
- Os dados GPS são transmitidos ao dono via HTTP/MQTT (por Wi-Fi).
- A TinyML será usada para detectar a pessoa a ser seguida e obstáculos, com modelo treinado externamente.
- A estrutura do software será baseada em FreeRTOS, com tasks separadas para:
  - Visão (TinyML)
  - Controle motor
  - Sensores (toque, distância, giroscópio)
  - Comunicação (HTTP/MQTT)
  - Áudio
  - LED/status

