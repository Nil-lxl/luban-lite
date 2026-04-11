#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <finsh.h>
#include <aic_core.h>
#include <aic_utils.h>


#include "srp.h"


#define NITER          100
#define TEST_HASH      SRP_SHA512
#define TEST_NG        SRP_NG_2048

unsigned long long get_usec()
{
    return aic_get_time_us();
}

const char * test_n_hex = "EEAF0AB9ADB38DD69C33F80AFA8FC5E86072618775FF3C0B9EA2314C9C256576D674DF7496"
   "EA81D3383B4813D692C6E0E0D5D8E250B98BE48E495C1D6089DAD15DC7D7B46154D6B6CE8E"
   "F4AD69B15D4982559B297BCF1885C529F566660E57EC68EDBC3C05726CC02FD4CBF4976EAA"
   "9AFD5138FE8376435B9FC61D2FC0EB06E3";
const char * test_g_hex = "2";


int cmd_srp_main( int argc, char * argv[] )
{
    struct SRPSession  * session = NULL;
    struct SRPVerifier * ver = NULL;
    struct SRPUser     * usr = NULL;

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
    int i;

    unsigned long long start;
    unsigned long long duration;

    const char * username = "testuser";
    const char * password = "password";

    const char * auth_username = 0;
    const char * n_hex         = 0;
    const char * g_hex         = 0;

    SRP_HashAlgorithm alg     = TEST_HASH;
    SRP_NGType        ng_type = SRP_NG_8192; //TEST_NG;

    if (ng_type == SRP_NG_CUSTOM)
    {
        n_hex = test_n_hex;
        g_hex = test_g_hex;
    }

    session = srp_session_new( alg, ng_type, n_hex, g_hex );

    srp_create_salted_verification_key( session, username,
                (const unsigned char *)password,
                strlen(password),
                &bytes_s, &len_s, &bytes_v, &len_v );



    start = get_usec();

    for( i = 0; i < NITER; i++ )
    {
        printf("loop %d\n", i);
        usr =  srp_user_new( session, username,
                             (const unsigned char *)password,
                             strlen(password));

        srp_user_start_authentication( usr, &auth_username, &bytes_A, &len_A );

        /* User -> Host: (username, bytes_A) */
        ver =  srp_verifier_new( session, username, bytes_s, len_s, bytes_v, len_v,
                                 bytes_A, len_A, & bytes_B, &len_B);

        if ( !bytes_B )
        {
            printf("Verifier SRP-6a safety check violated!\n");
            goto cleanup;
        }

        /* Host -> User: (bytes_s, bytes_B) */
        srp_user_process_challenge( usr, bytes_s, len_s, bytes_B, len_B, &bytes_M, &len_M );

        if ( !bytes_M )
        {
            printf("User SRP-6a safety check violation!\n");
            goto cleanup;
        }

        /* User -> Host: (bytes_M) */
        srp_verifier_verify_session( ver, bytes_M, &bytes_HAMK );

        if ( !bytes_HAMK )
        {
            printf("User authentication failed!\n");
            goto cleanup;
        }

        /* Host -> User: (HAMK) */
        srp_user_verify_session( usr, bytes_HAMK );

        if ( !srp_user_is_authenticated(usr) )
        {
            printf("Server authentication failed!\n");
        }

cleanup:
        if (ver) {
            srp_verifier_delete( ver );
        }
        if (usr) {
            srp_user_delete( usr );
        }
        printf("%s:%d\n", __func__, __LINE__);
    }

    if (session) {
        srp_session_delete ( session );
    }
    duration = get_usec() - start;

    printf("Usec per call: %d\n", (int)(duration / NITER));


    free( (char *)bytes_s );
    free( (char *)bytes_v );

    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_srp_main, mbedtls_srp, mbedtls srp example);
