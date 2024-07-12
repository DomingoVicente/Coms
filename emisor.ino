#include <VirtualWire.h>

#define CRC_POLYNOMIAL 0x25  // Polinomio x^5 + x^2 + 1
#define CRC_INITIAL_REMAINDER 0x1F  // Valor inicial del CRC
#define CRC_WIDTH 5  // Ancho del CRC en bits

void fillPacket(uint8_t *packet, uint8_t origen, uint8_t destino, uint8_t secuencia, uint8_t total, const char *carga);
uint8_t calculateCRC(uint8_t *data, uint8_t length);

void setup() {
    Serial.begin(9600);
    Serial.println("Configurando Transmisor");
    vw_set_ptt_inverted(true);
    vw_setup(2000);
    vw_set_tx_pin(2);
}

void loop() {
    static enum { ESPERANDO_DESTINO, ESPERANDO_MENSAJE, ESPERANDO_DECISION } estado = ESPERANDO_DESTINO;
    static uint8_t destino;
    static String mensaje;
    static bool enviarOtroMensaje = true;  // Variable para capturar la decisión del usuario

    if (estado == ESPERANDO_DESTINO && Serial.available() > 0) {
        // Leer el destino desde el puerto serie
        destino = Serial.parseInt();

        Serial.println("Destino ingresado: " + String(destino));
        estado = ESPERANDO_MENSAJE;  // Cambiar al estado de espera del mensaje
    }
    
    if (estado == ESPERANDO_MENSAJE && Serial.available() > 0) {
        // Leer el mensaje desde el puerto serie
        mensaje = Serial.readStringUntil('\n');

        if (mensaje.length() > 0) {
            Serial.println("Mensaje ingresado: " + mensaje);

            int totalPackets = (mensaje.length() + 7) / 8;  // Calcular el número total de paquetes necesarios

            for (int i = 0; i < totalPackets; i++) {
                uint8_t packet[16];
                String segment = mensaje.substring(i * 8, (i + 1) * 8);

                fillPacket(packet, 8, destino, i + 1, totalPackets, segment.c_str());

                vw_send(packet, sizeof(packet));
                vw_wait_tx();  // Esperar a que se complete la transmisión

                Serial.print("Paquete Enviado = ");
                for (int j = 0; j < 15; j++) {
                    Serial.print(packet[j], BIN);
                    Serial.print(" ");
                }
                Serial.println();
                delay(1000);  // Enviar un paquete cada segundo
            }

            // Preguntar al usuario si quiere enviar otro mensaje
            estado = ESPERANDO_DECISION;
        }
    }

    if (estado == ESPERANDO_DECISION) {
        Serial.println("¿Desea enviar otro mensaje? (si/no)");
        while (Serial.available() == 0) {
            // Esperar a que el usuario ingrese una respuesta
        }
        String respuesta = Serial.readStringUntil('\n');
        respuesta.trim();  // Limpiar cualquier espacio en blanco alrededor de la respuesta

        if (respuesta.equalsIgnoreCase("si")) {
            estado = ESPERANDO_DESTINO;  // Volver a solicitar destino
            mensaje = "";  // Limpiar el mensaje anterior
        } else {
            Serial.println("Fin del programa.");
            while (true) {
                // Programa detenido aquí, puedes agregar cualquier lógica adicional si es necesario
            }
        }
    }
}



// Función para llenar el paquete de datos
void fillPacket(uint8_t *packet, uint8_t origen, uint8_t destino, uint8_t secuencia, uint8_t total, const char *carga) {
    memset(packet, 0, 16);  // Inicializar el paquete con ceros

    // Llenar origen (2 octetos, derecha a izquierda)
    packet[1] = origen & 0xFF;
    packet[0] = (origen >> 8) & 0xFF;

    // Llenar destino (2 octetos, derecha a izquierda)
    packet[3] = destino & 0xFF;
    packet[2] = (destino >> 8) & 0xFF;

    // Llenar secuencia (1 octeto)
    packet[6] = secuencia;

    // Llenar total (1 octeto)
    packet[7] = total;

    // Llenar carga (8 octetos, ASCII a bits)
    for (int i = 0; i < 8; i++) {
        if (i < strlen(carga)) {
            packet[8 + i] = carga[i];  
        } else {
            packet[8 + i] = 0; 
        }
    }

    // Calcular CRC sobre los primeros 8 octetos del paquete
    uint8_t crc = calculateCRC(packet, 16);
    packet[5] = crc;  // Guardar CRC en el paquete
}

// Función para calcular el CRC-5-USB
uint8_t calculateCRC(uint8_t *data, uint8_t length) {
    uint8_t remainder = CRC_INITIAL_REMAINDER;

    // Procesar cada byte del data
    for (uint8_t byte = 8; byte < length; ++byte) {
        remainder ^= data[byte];

        // Procesar cada bit en el byte
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (remainder & 0x10) {
                remainder = (remainder << 1) ^ CRC_POLYNOMIAL;
            } else {
                remainder = (remainder << 1);
            }
        }
    }
    
    return (remainder & 0x1F);
}
