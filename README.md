# Bracelet for monitoring safety of seniors

Código do sistema de uma pulseira de segurança que detecta quedas e emergências médicas em idosos.

# Visão Geral do projeto

![Circuit Diagram](img/circuit.png)

A pulseira que possui um oximetro, acelerômetro e botão de emergência. Os sensores se comunicam com o esp via I2C, o botão funciona ligado ao GPIO4 em configuração pull up.

O alarme, possui apenas um esp que fica conectado a uma rede esperando receber a requisição para ligar o buzzer

### Building and Flashing

inside the project dir, run.

``pio run -t upload``

To build/flash with debugging enabled run,

``pio run -e debug -t upload``

### Estrutura de Arquivos
TBA

### To-do
- [x] code review / cleanup / merge
- [ ] test and adjust the final code
- [ ] Document how to setup platformio and compile the project
- [ ] Document file structure
- [ ] Update project documentation
