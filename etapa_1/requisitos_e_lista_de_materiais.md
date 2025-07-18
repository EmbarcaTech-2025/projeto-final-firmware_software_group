# Projeto: Mala Autônoma Seguidora

## 🧰 Materiais Necessários

### 🔧 Kit Principal
- [x] **BitDogLab Kit** (contém Raspberry Pi Pico W)
  - Raspberry Pi Pico W (com Wi-Fi)
  - Sensor ultrassônico HC-SR04
  - Motor driver (L298N ou similar)
  - Roda com motor DC (x2)
  - Roda livre (caster wheel)
  - Protoboard
  - Jumpers diversos

### 🧩 Componentes Adicionais
- [x] **Câmera OV7670 (VGA)** — captura de imagem para detecção de pessoas
- [x] **Módulo GPS (ex: NEO-6M)** — localização e rastreamento geográfico
- [x] **Sensor de Toque Capacitivo (ex: TTP223)** — para detectar quando a pessoa toca na mala
- [x] **Bateria Li-ion 7.4V ou Power Bank**
- [x] Conversores de tensão (5V e 3.3V)
- [x] Buzzer (alarme sonoro)
- [x] LEDs indicadores (status do sistema)
- [x] Estrutura 3D impressa no formato de uma pequena mala

---

## 💻 Ambiente de Desenvolvimento

- **Linguagem**: C
- **Sistema Operacional**: Linux
- **Editor**: Sublime Text
- **SDK**: Raspberry Pi Pico C SDK
- **RTOS**: FreeRTOS
- **TinyML**: Para detecção de obstáculos e identificação da pessoa
- **Câmera OV7670**: Comunicação via PIO ou SPI
- **GPS**: Comunicação via UART
- **Sensor de Toque**: Entrada digital (interrupção ou polling)
- **Compilação**: CMake + `arm-none-eabi-gcc`
- **Ferramentas**:
  - `picotool` (upload via USB)
  - `openocd` (debug opcional)

---

## ✅ Requisitos Funcionais

1. **Detectar a pessoa** usando câmera OV7670 e TinyML embarcado.
2. **Seguir automaticamente** a pessoa detectada com controle de motores.
3. **Evitar obstáculos** com sensores ultrassônicos e visão.
4. **Parar imediatamente ao toque** usando sensor capacitivo (ex: quando a pessoa segura a mala).
5. **Emitir sinais visuais e sonoros** para indicar o estado do sistema.
6. **Obter localização GPS** para rastreamento ou futuras extensões.
7. **Executar tarefas paralelas com FreeRTOS** (sensores, motores, visão, interface).

---

## ❌ Requisitos Não Funcionais

1. **Portabilidade**: Estrutura 3D compacta e funcional no formato de mala.
2. **Baixo consumo energético**: Projetado para operar por pelo menos 1 hora.
3. **Modularidade do código**: Separação clara entre sensores, controle, visão e sistema.
4. **Resiliência**: Capaz de lidar com falhas ou perda temporária do alvo.
5. **Segurança operacional**: Parada imediata com toque e em situações críticas.
6. **Estética**: Impressão 3D com acabamento limpo e formato prático.
7. **Facilidade de montagem**: Estrutura planejada para encaixe dos componentes eletrônicos.

---

## 📌 Considerações Finais

- O uso do **sensor de toque capacitivo** substitui a necessidade de botões físicos e torna a interação mais intuitiva.
- A **estrutura 3D** pode ser projetada em CAD (ex: FreeCAD ou Fusion 360) e impressa em PLA ou ABS.
- A arquitetura do software será baseada em FreeRTOS, com tarefas separadas para visão, motores, sensores e controle geral.
- A TinyML será responsável por classificar imagens em tempo real para detecção de humanos/obstáculos.

