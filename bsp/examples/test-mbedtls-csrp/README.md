mbedtls-csrp
============
Derived from https://github.com/cocagne/csrp

mbedtls-csrp is a minimal C implementation of the [Secure Remote Password
protocol](http://srp.stanford.edu/). The project consists of a single
C file and is intended for direct inclusion into utilizing programs. 
It's only dependency is mbedtls (https://github.com/ARMmbed/mbedtls).

SRP Overview
------------

SRP is a cryptographically strong authentication
protocol for password-based, mutual authentication over an insecure
network connection.

Unlike other common challenge-response autentication protocols, such
as Kereros and SSL, SRP does not rely on an external infrastructure
of trusted key servers or certificate management. Instead, SRP server
applications use verification keys derived from each user's password
to determine the authenticity of a network connection.

SRP provides mutual-authentication in that successful authentication
requires both sides of the connection to have knowledge of the
user's password. If the client side lacks the user's password or the
server side lacks the proper verification key, the authentication will
fail.

Unlike SSL, SRP does not directly encrypt all data flowing through
the authenticated connection. However, successful authentication does
result in a cryptographically strong shared key that can be used
for symmetric-key encryption.

Entropy
-------

You need to take care of entropy to achieve cryptograhically sound random values.
For real world use, you should change the implementation in init_random() to supply your own seed
values. Also make sure to add entropy sources to your mbedtls port.

Usage Example
-------------

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "srp.h"


int main( int argc, char * argv[] )
{
    int auth_failed = 1;
    
    struct SRPSession  * session;
    struct SRPVerifier * ver;
    struct SRPUser     * usr;
    
    const unsigned char * bytes_s = 0;
    const unsigned char * bytes_v = 0;
    const unsigned char * bytes_A = 0;
    const unsigned char * bytes_B = 0;
    
    const unsigned char * bytes_M    = 0;
    const unsigned char * bytes_HAMK = 0;
    
    int len_s   = 0;
    int len_v   = 0;
    int len_A   = 0;
    int len_B   = 0;
    int len_M   = 0;
    
    const char * username = "testuser";
    const char * password = "password";
    
    const char * auth_username = 0;
    
    SRP_HashAlgorithm alg     = SRP_SHA1;
    SRP_NGType        ng_type = SRP_NG_2048;

    /* Create a session, this avoids adding a lot of parameters to all
     * other functions
     */
    session = srp_session_new( alg, ng_type, NULL, NULL );

    /* Create a salt+verification key for the user's password. The salt and
     * key need to be computed at the time the user's password is set and
     * must be stored by the server-side application for use during the
     * authentication process.
     */
    srp_create_salted_verification_key( session, username, 
                                        (const unsigned char *)password, 
                                        strlen(password), 
                                        &bytes_s, &len_s,
                                        &bytes_v, &len_v);
    
    /* Begin authentication process */
    usr =  srp_user_new( session, username, 
                         (const unsigned char *)password, 
                         strlen(password));

    srp_user_start_authentication( usr, &auth_username, &bytes_A, &len_A );

    /* User -> Host: (username, bytes_A) */
    ver =  srp_verifier_new( session, username, bytes_s, len_s, bytes_v, len_v, 
                             bytes_A, len_A, & bytes_B, &len_B);
        
    if ( !bytes_B ) {
       printf("Verifier SRP-6a safety check violated!\n");
       goto auth_failed;
    }
        
    /* Host -> User: (bytes_s, bytes_B) */
    srp_user_process_challenge( usr, bytes_s, len_s, bytes_B, len_B, &bytes_M, &len_M );
        
    if ( !bytes_M ) {
       printf("User SRP-6a safety check violation!\n");
       goto auth_failed;
    }
        
    /* User -> Host: (bytes_M) */
    srp_verifier_verify_session( ver, bytes_M, &bytes_HAMK );
        
    if ( !bytes_HAMK ) {
       printf("User authentication failed!\n");
       goto auth_failed;
    }
        
    /* Host -> User: (HAMK) */
    srp_user_verify_session( usr, bytes_HAMK );
        
    if ( !srp_user_is_authenticated(usr) ) {
       printf("Server authentication failed!\n");
       goto auth_failed;
    }

    auth_failed = 0; /* auth success! */
        
auth_failed:
    srp_verifier_delete( ver );
    srp_user_delete( usr );
    
    free( (char *)bytes_s );
    free( (char *)bytes_v );
        
    return auth_failed;
}
```
