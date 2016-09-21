# Embedded_Auto_Irrigation
Embedded system using microcontroller ATMEGA328P.

N (MAX = 10) End Devices , 1 Base Station at star network topology.
End Device is a custom pcb with few sensors like Dht22,Soil Moisture, Analog Temperature,a 328p microcontroller and a Zigbee S2 for communication with Base Station powered by a 9V battery and a solar panel. 
End device custom pcb schema designed by Eagle.
Base station is an Arduino Mega ADK with Arduino Sim Shield V2 for transmitting data to server via Gsm Gprs.
Rtc is included so lcd panel, solenoids for the irrigation powered by solar panel and batteries.

Low Power Consumption:End devices were set at Sleep Mode when we dont to take measures and base station wakes each end device via receiver interrupt.

