@echo off
echo -----------------------------------------
echo *** SPIFFS Flasher for ESP32-C6 ***
echo -----------------------------------------

if %1.==. (
    echo No COM Port Provided
	echo Exiting Now
	exit \B
)

if %2.==. (
    echo No spiffs_data folder provided
	echo Exiting Now
	exit \B
)


echo Selected COM Port : %1
echo Selected Spiffs Folder : %2

set /p DUMMY=Hit ENTER to continue...

echo Generating SPIFFS Data for ESP32-C6
echo Make sure fpga.bin is available in the path ../spiffs_data

python spiffsgen.py 1048576 %2 storage.bin 
esptool --chip esp32c6 --port %1 --baud 460800 write_flash -z 0x90000 storage.bin

echo -----------------------------------------
echo *** Flashing Completed ***
echo -----------------------------------------
