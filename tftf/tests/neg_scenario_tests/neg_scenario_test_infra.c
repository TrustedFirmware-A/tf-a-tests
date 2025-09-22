/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "neg_scenario_test_infra.h"

/*
 Temporary variables to speed up the authentication parameters search. These
 variables are assigned once during the integrity check and used any time an
 authentication parameter is requested, so we do not have to parse the image
 again
 */


int get_pubKey_from_cert(void *cert, size_t cert_len, void **returnPtr) {

	int ret;
	size_t len;
	unsigned char *p, *end, *crt_end, *pk_end;
	mbedtls_asn1_buf pk;

	/*
	 * The unique ASN.1 DER encoding of [0] EXPLICIT INTEGER { v3(2} }.
	 */
	const char v3[] = {
		/* The outer CONTEXT SPECIFIC 0 tag */
		MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0,
		/* The number bytes used to encode the inner INTEGER */
		3,
		/* The tag of the inner INTEGER */
		MBEDTLS_ASN1_INTEGER,
		/* The number of bytes needed to represent 2 */
		1,
		/* The actual value 2 */
		2,
	};

	p = (unsigned char *)cert;
	len = cert_len;
	crt_end = p + len;
	end = crt_end;

	/*
	 * Certificate  ::=  SEQUENCE  {
	 *      tbsCertificate       TBSCertificate,
	 *      signatureAlgorithm   AlgorithmIdentifier,
	 *      signatureValue       BIT STRING  }
	 */
	ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED |
				   MBEDTLS_ASN1_SEQUENCE);
	if ((ret != 0) || ((p + len) != end))
		return -1;

	/*
	 * TBSCertificate  ::=  SEQUENCE  {
	 */
	ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED |
				   MBEDTLS_ASN1_SEQUENCE);
	if (ret != 0)
		return -1;

	end = p + len;

	/*
	 * Version  ::=  [0] EXPLICIT INTEGER {  v1(0), v2(1), v3(2)  }
	 * -- only v3 accepted
	 */
	if (((end - p) <= (ptrdiff_t)sizeof(v3)) ||
	    (memcmp(p, v3, sizeof(v3)) != 0)) {
		return -1;
	}
	p += sizeof(v3);

	/*
	 * CertificateSerialNumber  ::=  INTEGER
	 */
	ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_INTEGER);
	if (ret != 0)
		return -1;

	p += len;

	/*
	 * signature            AlgorithmIdentifier
	 */

	ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED |
				   MBEDTLS_ASN1_SEQUENCE);
	if (ret != 0)
		return -1;

	p += len;

	/*
	 * issuer               Name
	 */
	ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED |
				   MBEDTLS_ASN1_SEQUENCE);
	if (ret != 0)
		return -1;

	p += len;

	/*
	 * Validity ::= SEQUENCE {
	 *      notBefore      Time,
	 *      notAfter       Time }
	 *
	 */
	ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED |
				   MBEDTLS_ASN1_SEQUENCE);
	if (ret != 0)
		return -1;

	p += len;

	/*
	 * subject              Name
	 */
	ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED |
				   MBEDTLS_ASN1_SEQUENCE);
	if (ret != 0)
		return -1;

	p += len;

	/*
	 * SubjectPublicKeyInfo
	 */
	pk.p = p;
	ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED |
				   MBEDTLS_ASN1_SEQUENCE);
	if (ret != 0)
		return -1;

	pk_end = p + len;
	pk.len = pk_end - pk.p;

	*returnPtr = p;

	return 0;
}
