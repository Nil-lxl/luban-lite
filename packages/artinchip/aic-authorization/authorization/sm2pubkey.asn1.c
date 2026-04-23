/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <asn1_ber_bytecode.h>
#include "sm2pubkey.asn1.h"

enum sm2pubkey_actions {
	ACT_sm2_get_p = 0,
	NR__sm2pubkey_actions = 1
};

static const asn1_action_t sm2pubkey_action_table[NR__sm2pubkey_actions] = {
	[0] = sm2_get_p,
};

static const unsigned char sm2pubkey_machine[] = {
	// Sm2PubKey
	[0] = ASN1_OP_MATCH,
	[1] = _tag(UNIV, CONS, SEQ),
	// AlgorithmIdentifier
	[2] =  ASN1_OP_MATCH,
	[3] =  _tag(UNIV, CONS, SEQ),
	[4] =   ASN1_OP_MATCH,		// algorithm
	[5] =   _tag(UNIV, PRIM, OID),
	[6] =   ASN1_OP_MATCH,		// parameters
	[7] =   _tag(UNIV, PRIM, OID),
	[8] =  ASN1_OP_END_SEQ,
	[9] =  ASN1_OP_MATCH_ACT,		// p
	[10] =  _tag(UNIV, PRIM, BTS),
	[11] =  _action(ACT_sm2_get_p),
	[12] = ASN1_OP_END_SEQ,
	[13] = ASN1_OP_COMPLETE,
};

const struct asn1_decoder sm2pubkey_decoder = {
	.machine = sm2pubkey_machine,
	.machlen = sizeof(sm2pubkey_machine),
	.actions = sm2pubkey_action_table,
};
