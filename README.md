# Esp32 Safety Band

Código do sistema de uma pulseira de segurança que detecta quedas e emergências médicas em idosos.
Esse codigo se comunica com o código de outro Esp32 rodando o [Safety Band Manager](Link Required), que por fim encaminha todos os dados pro sinric pro e ativa um buzzer quando necessário.

### Como buildar
- Tenha o esp-idf atual e suas dependencias instalados
- Dê source no esp-idf para ter suas libs no $PATH
- Builde com ```idf.py build```

### To-do
- Configurar esp32 e pinout adequadamente
- Configurar Protocolo de comunicação
- Task de comunicação com o esp32 central
- Tasks de sensores

### Recursos:
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32/index.html)
- [Sinric Pro API docs](https://apidocs.sinric.pro/#api-Device-get_api_v1_devices__deviceId_action)
- [Esp Websocket Client](https://docs.espressif.com/projects/esp-protocols/esp_websocket_client/docs/latest/index.html)
- [Sinric Pro ESP-IDF Component Devices](https://github.com/sinricpro/esp-idf/tree/c68910bec1e520c009917579313d62a9fceca084/src/devices)
