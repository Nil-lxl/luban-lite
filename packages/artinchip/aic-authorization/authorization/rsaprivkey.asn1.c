/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <asn1_ber_bytecode.h>
#include "rsaprivkey.asn1.h"

enum rsaprivkey_actions {
    ACT_rsa_get_d = 0,
    ACT_rsa_get_dp = 1,
    ACT_rsa_get_dq = 2,
    ACT_rsa_get_e = 3,
    ACT_rsa_get_n = 4,
    ACT_rsa_get_p = 5,
    ACT_rsa_get_q = 6,
    ACT_rsa_get_qinv = 7,
    NR__rsaprivkey_actions = 8
};

static const asn1_action_t rsaprivkey_action_table[NR__rsaprivkey_actions] = {
    [0] = rsa_get_d,
    [1] = rsa_get_dp,
    [2] = rsa_get_dq,
    [3] = rsa_get_e,
    [4] = rsa_get_n,
    [5] = rsa_get_p,
    [6] = rsa_get_q,
    [7] = rsa_get_qinv,
};

static const unsigned char rsaprivkey_machine[] = {
    // RsaPrivKey
    [0] = ASN1_OP_MATCH,
    [1] = _tag(UNIV, CONS, SEQ),
    [2] = ASN1_OP_MATCH, // version
    [3] = _tag(UNIV, PRIM, INT),
    [4] = ASN1_OP_MATCH_ACT, // n
    [5] = _tag(UNIV, PRIM, INT),
    [6] = _action(ACT_rsa_get_n),
    [7] = ASN1_OP_MATCH_ACT, // e
    [8] = _tag(UNIV, PRIM, INT),
    [9] = _action(ACT_rsa_get_e),
    [10] = ASN1_OP_MATCH_ACT, // d
    [11] = _tag(UNIV, PRIM, INT),
    [12] = _action(ACT_rsa_get_d),
    [13] = ASN1_OP_MATCH_ACT, // prime1
    [14] = _tag(UNIV, PRIM, INT),
    [15] = _action(ACT_rsa_get_p),
    [16] = ASN1_OP_MATCH_ACT, // prime2
    [17] = _tag(UNIV, PRIM, INT),
    [18] = _action(ACT_rsa_get_q),
    [19] = ASN1_OP_MATCH_ACT, // exponent1
    [20] = _tag(UNIV, PRIM, INT),
    [21] = _action(ACT_rsa_get_dp),
    [22] = ASN1_OP_MATCH_ACT, // exponent2
    [23] = _tag(UNIV, PRIM, INT),
    [24] = _action(ACT_rsa_get_dq),
    [25] = ASN1_OP_MATCH_ACT, // coefficient
    [26] = _tag(UNIV, PRIM, INT),
    [27] = _action(ACT_rsa_get_qinv),
    [28] = ASN1_OP_END_SEQ,
    [29] = ASN1_OP_COMPLETE,
};

const struct asn1_decoder rsaprivkey_decoder = {
    .machine = rsaprivkey_machine,
    .machlen = sizeof(rsaprivkey_machine),
    .actions = rsaprivkey_action_table,
};
