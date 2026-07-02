# Bracelet for monitoring safety of seniors

Código do sistema de uma pulseira de segurança que detecta quedas e emergências médicas em idosos.
Esse repositório possui dois códigos fontes diferentes, um para a pulseira, e outro para um sistema que ativa um alarme.
Mais informações sobre o funcionamento de cada dispositivos podem ser encontradas em arquivos .md em seus respectivos diretórios.

# Visão Geral do projeto

![Circuit Diagram](img/circuit.png)

Temos dois devices: 
A pulseira que possui um oximetro, acelerômetro e botão de emergência. Os sensores se comunicam com o esp via I2C, o botão funciona ligado ao GPIO4 em configuração pull up.

O alarme, possui apenas um esp que fica conectado a uma rede esperando receber a requisição para ligar o buzzer

### Estrutura de Arquivos
TBA

### Como buildar
- Tenha o esp-idf atual e suas dependencias instalados
- Dê source no esp-idf para ter suas libs no $PATH
- Builde com ```idf.py build```

### To-do
- [ ] finish project documentation
- [ ] code review / cleanup / merge
- [ ] test and adjust the final code

### Recursos:
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32/index.html)
- [Sinric Pro API docs](https://apidocs.sinric.pro/#api-Device-get_api_v1_devices__deviceId_action)
- [Esp Websocket Client](https://docs.espressif.com/projects/esp-protocols/esp_websocket_client/docs/latest/index.html)
- [Sinric Pro ESP-IDF Component Devices](https://github.com/sinricpro/esp-idf/tree/c68910bec1e520c009917579313d62a9fceca084/src/devices)
