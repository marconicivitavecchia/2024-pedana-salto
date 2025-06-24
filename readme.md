## Specifiche Cella di Carico YZC-516C

## Capacità e Range
- Range disponibili: 0.1, 0.2, 0.3, 0.5, 1, 2 tonnellate

## Specifiche Elettriche
- Tensione di uscita: 2.0±0.05 mV/V
- Resistenza di ingresso: 365±5 Ω
- Resistenza di uscita: 350±3 Ω
- Resistenza di isolamento: >5000 MΩ/50V
- Tensione di eccitazione raccomandata: 5~12V

## Prestazioni e Precisione
- Classe di precisione: C2
- Errore combinato: ≤ ±0.030
- Creep: 0.02
- Effetto temperatura su sensibilità: 0.0017 %RO/°C
- Effetto temperatura su zero: 0.005 %RO/°C
- Bilanciamento zero: ±1.0 %RO

## Range di Temperatura
- Range temperatura compensata: -10~+40°C
- Range temperatura operativa: -30~+80°C

## Sovraccarichi
- Sovraccarico di sicurezza: 150% RO
- Sovraccarico ultimo: 200% RO

## Specifiche Meccaniche
- Materiale: Lega di acciaio
- Cavo: Schermato Ø5.0X3.5m

## Dimensioni
| Capacità    | B (mm) | D          |
|-------------|--------|------------|
| 100-300kg   | 19.05  | M12x1.75   |
| 500kg       | 25.4   | M12x1.75   |
| 1t-2t       | 25.4   | M16x2      |

## Caratteristiche Speciali
- Alta resistenza all'acqua
- Cavo schermato standard
- Connessione aerea su richiesta

# ADC per Celle di Carico in Ordine di Velocità

| ADC      | Max Sample Rate | Bits | PGA Max | Canali Diff. | Alimentazione | Note |
|----------|----------------|------|---------|--------------|---------------|------|
| ADS1256  | 30k SPS        | 24   | 64x     | 4            | AVDD: 5V, DVDD: 1.8-3.6V | Più veloce ma PGA insufficiente |
| ADS1261  | 40k SPS        | 32   | 32x     | 1            | AVDD: 2.7-3.6V, DVDD: 1.65-3.6V | Veloce, singolo canale |
| ADS1262  | 38k SPS        | 32   | 32x     | 4            | AVDD: 2.7-3.6V, DVDD: 1.65-3.6V | Alta risoluzione, basso rumore |
| ADS1263  | 38k SPS        | 32+24 | 128x   | 6            | AVDD: 2.7-3.6V, DVDD: 1.65-3.6V | Come 1262 ma con più canali e gain esteso |
| MCP3561  | 15k SPS        | 24   | 64x     | 2            | 2.7-3.6V     | Buon compromesso velocità/prestazioni |
| ADS1234  | 2k SPS         | 24   | 128x    | 4            | AVDD: 5V, DVDD: 2.7-5.25V | Ottimizzato per celle di carico |
| ADS1232  | 1.6k SPS       | 24   | 128x    | 2            | AVDD: 5V, DVDD: 2.7-5.25V | Versione 2 canali dell'ADS1234 |
| NAU7802  | 320 SPS        | 24   | 128x    | 1            | 2.7-3.6V     | I2C limita la velocità |
| HX711    | 80 SPS         | 24   | 128x    | 1            | AVDD: 4.8-5.5V | Lento ma ampiamente utilizzato |

## Note aggiuntive
* ADS1256: 8 canali single-ended configurabili come 4 differenziali 
* ADS1261: Singolo canale ad alte prestazioni, velocità massima, 32-bit
* ADS1262: PGA fino a 32x e alta velocità, ottimo per celle di carico, 32-bit
* ADS1263: ADC1 32-bit principale + ADC2 24-bit ausiliario, PGA fino a 128x
* MCP3561: 4 canali single-ended / 2 differenziali con multiplexer
* ADS1234: 4 canali differenziali, 24-bit, ottimizzato per celle di carico
* ADS1232: Come ADS1234 ma con 2 canali differenziali
* NAU7802: singolo canale differenziale dedicato
* HX711: singolo canale differenziale più un canale ausiliario

## Fasi progetto:
- [Dimensionamento ADC](dimens_ADC.md)
- [Polarizzazione celle](polarizzazione.md)
- [Campionamento](sampling.md)
- [Filtro antialiasing](aliasing.md)
- [Calibrazione statica](calibrazione_statica.md)
- [Calcolo altezza](accelerazione.md)
- [Definizione soglie](soglie.md)
- [Filtri per Motion Tracking](filtri.md)
- [Filtro di Kalman da accelerazione](kalman_accelerazione.md)
- [Filtro di Kalman da tempo di volo](kalman_tempovolo.md)
- [Streaming dei campioni ADC](adcstreaming.md)
- [Progetto con ESP32](ads1256.md)


**Materiale**:
- ADC ADS1232 https://it.aliexpress.com/item/1005007195802960.html?spm=a2g0o.order_list.order_list_main.5.31183696R6xAH9&gatewayAdapt=glo2ita
- ADC ADS1256 https://it.aliexpress.com/item/1005006296794440.html?spm=a2g0o.order_list.order_list_main.21.76bc3696RICV2y&gatewayAdapt=glo2ita
- https://github.com/Protocentral/ProtoCentral_ads1262
- https://github.com/OLIMEX/BB-ADS1262
- https://www.olimex.com/Products/Breadboarding/BB-ADS1262/open-source-hardware
- https://www.industrialshields.com/blog/arduino-industrial-1/how-to-use-spi-on-an-esp32-based-plc-528#:~:text=The%20ESP32%20microcontroller%20has%20four,SPI%20drivers%2C%20open%20to%20users.
- https://community.element14.com/challenges-projects/project14/dataconversion/b/blog/posts/oscilloscope-on-a-24-bit-adc-chip-ads1256
- https://digitaltown.co.uk/components6ads1256ADC.php

**Tutorials su ADC differenziali:**
- [Misure con ponti](/manuali/sbaa532a.pdf)
- [Misure single ended](manuali/sbaa133a.pdf)

**Tutorials su ADS1234:**
- https://www.youtube.com/watch?v=NklVhEleiHI
- https://www.youtube.com/watch?v=4HYsbSOxGqY

**Tutorials filtraggio digitale:**
- https://ww1.microchip.com/downloads/en/Appnotes/ProcessAnalogSensorDataDigitalFiltering-DS00004515.pdf
- http://musicweb.ucsd.edu/~trsmyth/filters/filters.html
- https://github.com/sunsided/kalman-clib/tree/master?tab=readme-ov-file
- https://github.com/sunsided/minikalman-rs/tree/main
- https://www.ti.com/lit/ab/sbaa587/sbaa587.pdf?ts=1732984488882#:~:text=Digital%20filters%20are%20commonly%20implemented,micro%2Dcontrollers%2C%20and%20FPGAs.&text=In%20general%2C%20there%20are%20three,)%20filters%2C%20and%20sinc%20filters.
- https://www.ti.com/lit/an/sbaa230a/sbaa230a.pdf?ts=1732979002663
- https://cal.unibg.it/wp-content/uploads/controlli_automatici/Lez06.pdf
- https://www.youtube.com/watch?v=D0lEiOFf9TM
- https://www.youtube.com/watch?v=ts23oF3AZmc
- https://moodle2.units.it/mod/folder/view.php?id=296060
- https://github.com/rlabbe/Kalman-and-Bayesian-Filters-in-Python
- https://www.dmi.unict.it/santoro/teaching/psr/slides/KalmanFiltering.pdf
- https://www.dmi.unict.it/santoro/teaching/psr/slides/
- https://github.com/rfetick/Kalman
