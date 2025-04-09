set gen_rsa=false
set gen_aes=false
if not exist set_aes_key.txt set gen_aes=true

if "%gen_aes%" == "true" (
	..\..\..\..\..\tools\scripts\openssl.exe rand -hex 16 > set_aes_key.txt
)

..\..\..\..\..\tools\scripts\xxd.exe -p -r set_aes_key.txt >spi_aes.key
..\..\..\..\..\tools\scripts\xxd.exe -c 16 -i spi_aes.key > spi_aes_key.h

copy spi_aes_key.h ..\..\..\..\..\bsp\examples_bare\test-efuse\spi_aes_key.h
