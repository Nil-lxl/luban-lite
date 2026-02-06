set gen_rsa=false
set gen_aes=false
if not exist set_aes_key.txt set gen_aes=true

if "%gen_aes%" == "true" (
	..\..\..\..\..\tools\scripts\openssl.exe rand -hex 16 > set_aes_key.txt
)

..\..\..\..\..\tools\scripts\xxd.exe -p -r set_aes_key.txt >spi_aes.key
..\..\..\..\..\tools\scripts\xxd.exe -c 16 -i spi_aes.key > spi_aes_key.h

..\..\..\..\..\tools\scripts\cat.exe spi_aes.key > all.key
..\..\..\..\..\tools\scripts\crc32.exe spi_aes.key > crc32.txt
..\..\..\..\..\tools\scripts\sed.exe -i 's/([0-9]*)$/spi_aes.key/g' crc32.txt
..\..\..\..\..\tools\scripts\crc32.exe all.key >> crc32.txt
..\..\..\..\..\tools\scripts\sed.exe -i 's/([0-9]*)$/all.key/' crc32.txt

copy spi_aes_key.h ..\..\..\..\..\bsp\examples_bare\test-efuse\spi_aes_key.h
