set gen_rsa=false
set gen_aes=false

if not exist rsa_private_key.der set gen_rsa=true
if not exist rsa_public_key.der set gen_rsa=true
if not exist set_aes_key.txt set gen_aes=true
if not exist set_nonce.txt set gen_aes=true

if "%gen_rsa%" == "true" (
	..\..\..\..\..\tools\scripts\openssl.exe genrsa -out rsa_private_key.pem 2048
	..\..\..\..\..\tools\scripts\openssl.exe rsa -in rsa_private_key.pem -pubout -out rsa_public_key.pem
	..\..\..\..\..\tools\scripts\openssl.exe base64 -d -in rsa_public_key.pem -out rsa_public_key.der
	..\..\..\..\..\tools\scripts\openssl.exe base64 -d -in rsa_public_key.pem -out rsa_public_key.der
	..\..\..\..\..\tools\scripts\openssl.exe base64 -d -in rsa_private_key.pem -out rsa_private_key.der
)
if "%gen_aes%" == "true" (
	..\..\..\..\..\tools\scripts\openssl.exe rand -hex 16 > set_aes_key.txt
	..\..\..\..\..\tools\scripts\openssl.exe rand -hex 8 > set_nonce.txt
)

..\..\..\..\..\tools\scripts\openssl.exe dgst -md5 -binary -out rotpk.bin rsa_public_key.der

..\..\..\..\..\tools\scripts\xxd.exe -p -r set_aes_key.txt >spi_aes.key
..\..\..\..\..\tools\scripts\xxd.exe -p -r set_nonce.txt >spi_nonce.key
..\..\..\..\..\tools\scripts\xxd.exe -c 16 -i spi_aes.key > spi_aes_key.h
..\..\..\..\..\tools\scripts\xxd.exe -c 16 -i spi_nonce.key >> spi_aes_key.h
..\..\..\..\..\tools\scripts\xxd.exe -c 16 -i rotpk.bin >> spi_aes_key.h

copy spi_aes_key.h ..\..\..\..\..\bsp\examples_bare\test-efuse\spi_aes_key.h
