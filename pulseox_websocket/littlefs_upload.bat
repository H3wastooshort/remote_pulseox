mklittlefs -c data -s 0x10000 littlefs.bin
python -m esptool -p COM12 --chip esp8266 --after no_reset write_flash 0xEB000 littlefs.bin
