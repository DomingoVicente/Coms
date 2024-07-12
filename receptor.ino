#include <VirtualWire.h>

#define CRC_POLYNOMIAL 0x25  // Polinomio x^5 + x^2 + 1
#define CRC_INITIAL_REMAINDER 0x1F  // Valor inicial del CRC
#define CRC_WIDTH 5  // Ancho del CRC en bits
#define CRC_TOPBIT (1 << (CRC_WIDTH - 1))  // Bit superior del CRC

void setup() {
    Serial.begin(9600);
    Serial.println("Configurando Receptor");
    vw_set_ptt_inverted(true);
    vw_setup(2000);     // Bits por segundo
    vw_set_rx_pin(2);
    vw_rx_start();       // Comienza a escuchar
}

void loop() {
    uint8_t buf[16];
    uint8_t buflen = sizeof(buf);

    if (vw_get_message(buf, &buflen) && ((buf[2] == 0x00 && buf[3] == 0x08)
        || (buf[2] == 0x00 && buf[3] == 0x00)
        ))
        {  // Si se recibe un mensaje
    
        Serial.print("Mensaje recibido: ");
        for (int i = 0; i < buflen; i++) {
            Serial.print(buf[i], BIN);
            Serial.print(" ");
        }
        Serial.println();

        // Calcular el CRC del mensaje recibido (excepto el quinto octeto que es el CRC recibido)
        uint8_t crc = calculateCRC(buf, 16);

        Serial.print("CRC calculado: ");
        Serial.println(crc, BIN);

        Serial.print("CRC recibido: ");
        Serial.println(buf[5], BIN);  // El CRC recibido está en buf[4]

        // Comparar el CRC calculado con el CRC recibido
        if (crc == buf[5]) {
            Serial.println("CRC correcto");
        } else {
            Serial.println("CRC incorrecto");
        }

        // Mostrar la palabra recibida
        char palabra[9];
        memcpy(palabra, &buf[8], 8);
        palabra[8] = '\0';
        Serial.print("Palabra recibida: ");
        Serial.println(palabra);
    }
}

// Función para calcular el CRC-5-USB
uint8_t calculateCRC(uint8_t *data, uint8_t length) {
    uint8_t remainder = CRC_INITIAL_REMAINDER;

    // Procesar cada byte del data
    for (uint8_t byte = 8; byte < length; ++byte) {
        remainder ^= (data[byte] << (CRC_WIDTH - 8));

        // Procesar cada bit en el byte
        for (uint8_t bit = 8; bit > 0; --bit) {
            if (remainder & CRC_TOPBIT) {
                remainder = (remainder << 1) ^ CRC_POLYNOMIAL;
            } else {
                remainder = (remainder << 1);
            }
        }
    }

    return (remainder & ((1 << CRC_WIDTH) - 1));
}
