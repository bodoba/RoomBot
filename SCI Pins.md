# Roomba Serial Adapter Pinout

             1       2 
           Orange   Yellow
            Vpwr     Vpwr
      3                    4 
     Red                 Green
     RXD                  TXD
         5         6        7
       Brown     Purple    Blue
        DD        GND      GND


| Position | Color   | Pin-Function                              |
|----------|---------|-------------------------------------------| 
| 1        | Orange	 | Vpwr (Battery not regulated; ~12-20V)     |
| 2        | Yellow	 | Vpwr (Battery not regulated;  ~12-20V)    |
| 3        | Red	   | RXD (Host -> Roomba) 0-5V (TTL)           | 
| 4        | Green	 | TXD (Roomba -> Host) 0-5V (TTL)           | 
| 5        | Brown	 | Device Detect input (active low)          |  
| 6        | Blue	   | GND                                       | 
| 7        | Purple	 | GND                                       | 

# Orang PI PC+ Pinout

| GPIO No.| GPIO | Function | Pin | Pin | Function | GPIO | GPIO No.| 
|---------|------|----------|-----|-----|----------|------|---------|
|         |      | 3.3v     | 1   | 2   |  5v      |      |         | 
| 12      | PA12 | SDA.0.   | 3   | 4   |  5V      |      |         | 
| 11      | PA11 | SCL.0    | 5   | 6   |  GND     |      |         |
| 6       | PA6  | PA6      | 7   | 8   |  TXD.3   | PA13 | 13      |
|         |      | GND      | 9   | 10  |  RXD.3   | PA14 | 14      |
| 1       | PA1  | RXD.2    | 11  | 12  |  PD14    | PD14 | 110     |
| 0       | PA0  | TXD.2    | 13  | 14  |  GND     |      |         |
| 3       | PA3  | CTS.2    | 15  | 16  |  PC4     | PC4  | 68      |
|         |      | 3.3v     | 17  | 18  |  PC7     | PC7  | 71      |
| 64      | PC0  | MOSI.0   | 19  | 20  |  GND     |      |         |
| 65      | PC1  | MISO.0   | 21  | 22  |  RTS.2   | PA2  | 2       |
| 66      | PC2  | SCLK.0   | 23  | 24  |  CE.0    | PC3  | 67      |
|         |      | GND      | 25  | 26  |  PA21    | PA21 | 21      |
| 19      | PA19 | SDA.1    | 27  | 28  |  SCL.1   | PA18 | 18      |
| 7       | PA7  | PA7      | 29  | 30  |  GND     |      |         |
| 8       | PA8  | PA8      | 31  | 32  |  RTS.1   | PG8  | 200     |
| 9       | PA9  | PA9      | 33  | 34  |  GND     |      |         |
| 10      | PA10 | PA10     | 35  | 36  |  CTS.1   | PG9  | 201     |
| 20      | PA20 | PA20     | 37  | 38  |  TXD.1   | PG6  | 198     |
|         |      | GND      | 39  | 40  |  RXD.1   | PG7  | 199     |
