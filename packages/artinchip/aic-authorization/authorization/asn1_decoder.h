/*
 * Copyright (C) 2012-2024 Red Hat, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Written by David Howells (dhowells@redhat.com)
 */

#ifndef __ASN1_DECODER_H
#define __ASN1_DECODER_H

#include <asn1.h>

struct asn1_decoder;

extern int asn1_ber_decoder(const struct asn1_decoder *decoder, void *context,
                            const unsigned char *data, size_t datalen);

#endif /* __ASN1_DECODER_H */
