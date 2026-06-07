# Esp32 Safety Band Manager

Código do sistema de gerenciamento das pulseiras que detecam quedas e emergências médicas em idosos. Esse sistema deve se comunicar com todas as pulseiras, ativar um buzzer caso necessário, e repassar os dados para api do sinric pro. 

### Como buildar
- Tenha o esp-idf 5.x e suas dependencias instalados
- Instale o componente do sinricpro e do websocket
- Dê source no esp-idf para ter suas libs no $PATH
- Builde com ```idf.py build```

### To-do
- Configurar esp32 e pinout adequadamente
- Configurar Websockets
- Criar e configurar os Devices necessários pra integrar com sinricpro
- Task de comunicação com outros esps
- Task do buzzer
- Taks pra repassar dados do websocket para o sinric pro

### Recursos:
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32/index.html)
- [Sinric Pro API docs](https://apidocs.sinric.pro/#api-Device-get_api_v1_devices__deviceId_action)
- [Esp Websocket Client](https://docs.espressif.com/projects/esp-protocols/esp_websocket_client/docs/latest/index.html)
- [Sinric Pro ESP-IDF Component Devices](https://github.com/sinricpro/esp-idf/tree/c68910bec1e520c009917579313d62a9fceca084/src/devices)
