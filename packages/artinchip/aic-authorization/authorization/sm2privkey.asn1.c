/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <asn1_ber_bytecode.h>
#include "sm2privkey.asn1.h"

enum sm2privkey_actions {
	ACT_sm2_get_d = 0,
	ACT_sm2_get_p = 1,
	NR__sm2privkey_actions = 2
};

static const asn1_action_t sm2privkey_action_table[NR__sm2privkey_actions] = {
	[0] = sm2_get_d,
	[1] = sm2_get_p,
};

static const unsigned char sm2privkey_machine[] = {
	// Sm2PrivKey
	[0] = ASN1_OP_MATCH,
	[1] = _tag(UNIV, CONS, SEQ),
	[2] =  ASN1_OP_MATCH,		// version
	[3] =  _tag(UNIV, PRIM, INT),
	[4] =  ASN1_OP_MATCH_ACT,		// d
	[5] =  _tag(UNIV, PRIM, OTS),
	[6] =  _action(ACT_sm2_get_d),
	[7] =  ASN1_OP_MATCH,		// oid
	[8] =  _tagn(CONT, CONS,  0),
	[9] =   ASN1_OP_MATCH,		// oid
	[10] =   _tag(UNIV, PRIM, OID),
	[11] =  ASN1_OP_END_SEQ,
	[12] =  ASN1_OP_MATCH,		// p
	[13] =  _tagn(CONT, CONS,  1),
	[14] =   ASN1_OP_MATCH_ACT,		// p
	[15] =   _tag(UNIV, PRIM, BTS),
	[16] =   _action(ACT_sm2_get_p),
	[17] =  ASN1_OP_END_SEQ,
	[18] = ASN1_OP_END_SEQ,
	[19] = ASN1_OP_COMPLETE,
};

const struct asn1_decoder sm2privkey_decoder = {
	.machine = sm2privkey_machine,
	.machlen = sizeof(sm2privkey_machine),
	.actions = sm2privkey_action_table,
};
