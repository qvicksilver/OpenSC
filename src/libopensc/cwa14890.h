/**
 * cwa14890.h: Defines, Typedefs and prototype functions for SM Messaging according CWA-14890 standard.
 *
 * Copyright (C) 2010 Juan Antonio Martinez <jonsito@terra.es>
 *
 * This work is derived from many sources at OpenSC Project site,
 * (see references), and the information made public for Spanish 
 * Direccion General de la Policia y de la Guardia Civil
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __CWA14890_H__
#define __CWA14890_H__

#ifdef ENABLE_OPENSSL

/* Secure Messaging state indicator */
#define CWA_SM_NONE       0x00	/** No SM channel defined */
#define CWA_SM_INPROGRESS 0x01	/** SM channel is being created: don't use */
#define CWA_SM_ACTIVE     0x02	/** SM channel is active */

/* Flags for setting SM status */
#define CWA_SM_OFF        0x00	/** Disable SM channel */
#define CWA_SM_COLD       0x01	/** force creation of a new SM channel */
#define CWA_SM_WARM       0x02	/** Create new SM channel only if state is NONE */

/* TAGS for encoded APDU's */
#define CWA_SM_PLAIN_TAG  0x81	/** Plain value (to be protected by CC) */
#define CWA_SM_CRYPTO_TAG 0x87	/** Padding-content + cryptogram */
#define CWA_SM_MAC_TAG    0x8E	/** Cryptographic checksum (MAC) */
#define CWA_SM_LE_TAG     0x97	/** Le (to be protected by CC ) */
#define CWA_SM_STATUS_TAG 0x99	/** Processing status (SW1-SW2 mac protected ) */

/*************** data structures for CWA14890 SM handling **************/

#include "libopensc/types.h"

#include <openssl/x509.h>
#include <openssl/des.h>

/**
 * Structure used to compose BER-TLV encoded data
 * according to iso7816-4 sect 5.2.2.
 *
 * Notice that current implementation does not handle properly
 * multibyte tag id. Just asume that tag is 1-byte lenght
 * Also, encodings for data lenght longer than 0x01000000 bytes
 * are not supported (tag 0x84)
 */
typedef struct cwa_tlv_st {
	u8 *buf;		/** local copy of TLV byte array */
	size_t buflen;		/** lengt of buffer */
	unsigned int tag;	/** tag ID */
	size_t len;		/** lenght of data field */
	u8 *data;		/** pointer to start of data in buf buffer */
} cwa_tlv_t;

/**
 * Structure used to handle keys and sequence counter once SM session
 * is stablished
 */
typedef struct cwa_sm_session_st {
	/* variables used once SM is started */
	int state;	/** one of NONE, INPROGRESS, or ACTIVE */
	u8 kenc[16];	/** key used for data encoding */
	u8 kmac[16];	/** key for mac checksum calculation */
	u8 ssc[8];	/** send sequence counter */
} cwa_sm_session_t;

/**
 * Estructure used to compose and store variables related to SM setting
 * and encode/decode apdu messages.
 */
typedef struct cwa_sm_status_st {
	/* variables used in SM establishment */
	u8 kicc[32];
	u8 kifd[32];
	u8 rndicc[8];	/** 8 bytes random number generated by card */
	u8 rndifd[8];	/** 8 bytes random number generated by application */
	u8 sig[128];	/** buffer to store & compute signatures (1024 bits) */
	cwa_sm_session_t session; /** current session data */
} cwa_sm_status_t;

/**
 * Data and function pointers to provide information to create and handle
 * Secure Channel.
 */
typedef struct cwa_provider_st {
    /************ data related with SM operations *************************/

	cwa_sm_status_t status; /** sm status for this provider */

    /************ operations related with secure channel creation *********/

	/* pre and post operations */

	/** 
 	* CWA-14890 SM stablisment pre-operations.
	*
	* This code is called before any operation required in
	* standard cwa14890 SM stablisment process. It's usually
	* used for adquiring/initialize data to be used in the
	* process (i.e: retrieve card serial number), to make sure
	* that no extra apdu is sent during the SM stablishment procedure
	*
	* @param card pointer to card driver structure
	* @param provider pointer to SM data provider for DNIe
	* @return SC_SUCCESS if OK. else error code
	*/
	int (*cwa_create_pre_ops) (sc_card_t * card,
				   struct cwa_provider_st * provider);

	/** 
 	* CWA-14890 SM stablisment post-operations.
	*
	* This code is called after sucessfull SM chanel stablishment
	* procedure, and before returning from create_sm_chanel() function
	* May be use for store data, trace, logs and so
	*
	* @param card pointer to card driver structure
	* @param provider pointer to SM data provider for DNIe
	* @return SC_SUCCESS if OK. else error code
	*/
	int (*cwa_create_post_ops) (sc_card_t * card,
				    struct cwa_provider_st * provider);

	/**
	* Get ICC (card) intermediate CA Certificate.
	*
	* @param card Pointer to card driver structure
	* @param cert where to store resulting certificate
	* @return SC_SUCCESS if ok; else error code
	*/
	int (*cwa_get_icc_intermediate_ca_cert) (sc_card_t * card,
						 X509 ** cert);

	/**
	* Get ICC (card) certificate.
	*
	* @param card Pointer to card driver structure
	* @param cert where to store resulting certificate
	* @return SC_SUCCESS if ok; else error code
	*/
	int (*cwa_get_icc_cert) (sc_card_t * card, X509 ** cert);

	/** 
	* Obtain RSA public key from RootCA.
	*
	* @param root_ca_key pointer to resulting returned key
	* @return SC_SUCCESS if ok; else error code
	*/
	int (*cwa_get_root_ca_pubkey) (sc_card_t * card, EVP_PKEY ** key);

	/**
	* Get RSA IFD (Terminal) private key data.
	* 
	* Notice that resulting data should be keept in memory as little 
	* as possible Erasing them once used
	*
	* @param card pointer to card driver structure
	* @param ifd_privkey where to store IFD private key
	* @return SC_SUCCESS if ok; else error code
	*/
	int (*cwa_get_ifd_privkey) (sc_card_t * card, EVP_PKEY ** key);

	/* TODO:
	 * CVC handling routines should be grouped in just retrieve CVC
	 * certificate. The key reference, as stated by CWA should be
	 * extracted from CVC...
	 *
	 * But to do this, an special OpenSSL with PACE extensions is
	 * needed. In the meantime, let's use binary buffers to get
	 * CVC and key references, until an CV_CERT hancling API 
	 * become available in standard OpenSSL
	 *
	 *@see http://openpace.sourceforge.net
	 */

	/**
 	* Retrieve IFD (application) CVC intermediate CA certificate and length.
	*
	* Returns a byte array with the intermediate CA certificate
	* (in CardVerifiable Certificate format) to be sent to the
	* card in External Authentication process
	*
	* @param card Pointer to card driver Certificate
	* @param cert Where to store resulting byte array
	* @param length len of returned byte array
	* @return SC_SUCCESS if ok; else error code
	*/
	int (*cwa_get_cvc_ca_cert) (sc_card_t * card, u8 ** cert,
				    size_t * lenght);

	/**
	* Retrieve IFD (application) CVC certificate and length.
	*
	* Returns a byte array with the application's certificate
	* (in CardVerifiable Certificate format) to be sent to the
	* card in External Authentication process
	*
	* @param card Pointer to card driver Certificate
	* @param cert Where to store resulting byte array
	* @param length len of returned byte array
	* @return SC_SUCCESS if ok; else error code
	*/
	int (*cwa_get_cvc_ifd_cert) (sc_card_t * card, u8 ** cert,
				     size_t * lenght);

	/**
	* Retrieve public key reference for Root CA to validate CVC intermediate CA certs.
	*
	* This is required in the process of On card external authenticate
	* @param card Pointer to card driver structure
	* @param buf where to store resulting key reference
	* @param len where to store buffer length
	* @return SC_SUCCESS if ok; else error code
	*/
	int (*cwa_get_root_ca_pubkey_ref) (sc_card_t * card, u8 ** buf,
					   size_t * len);

	/**
	* Get public key reference for intermediate CA to validate IFD cert.
	*
	* This is required in the process of On card external authenticate
	*
	* @param card Pointer to card driver structure
	* @param buf where to store resulting key reference
	* @param len where to store buffer length
	* @return SC_SUCCESS if ok; else error code
	*/
	int (*cwa_get_intermediate_ca_pubkey_ref) (sc_card_t * card, u8 ** buf,
						   size_t * len);

	/**
	*  Retrieve public key reference for IFD certificate.
	*
	* This tells the card with in memory key reference is to be used
	* when CVC cert is sent for external auth procedure
	*
	* @param card pointer to card driver structure
	* @param buf where to store data to be sent
	* @param len where to store data length
	* @return SC_SUCCESS if ok; else error code
	*/
	int (*cwa_get_ifd_pubkey_ref) (sc_card_t * card, u8 ** buf,
				       size_t * len);

	/**
	* Retrieve key reference for ICC private key.
	* 
	* @param card pointer to card driver structure
	* @param buf where to store data
	* @param len where to store data length
	* @return SC_SUCCESS if ok; else error
	*/
	int (*cwa_get_icc_privkey_ref) (sc_card_t * card, u8 ** buf,
					size_t * len);

	/**
	* Retrieve SN.IFD - Terminal Serial Number.
	*
	* Result SN is 8 bytes long left padded with zeroes if required.
	*
	* @param card pointer to card structure
	* @param buf where to store result (8 bytes)
	* @return SC_SUCCESS if ok; else error
	*/
	int (*cwa_get_sn_ifd) (sc_card_t * card, u8 ** buf);

	/**
	*  Get SN.ICC - Card Serial Number.
	*  
	*  Result value is 8 bytes long left padded with zeroes if needed)
	*
	* @param card pointer to card structure
	* @param buf where to store result (8 bytes)
	* @return SC_SUCCESS if ok; else error
	*/
	int (*cwa_get_sn_icc) (sc_card_t * card, u8 ** buf);

    /************** operations related with APDU encoding ******************/

	/**
 	* Operation to be done before any APDU encode procedure.
 	*
 	* @param card Pointer to card driver data structure
 	* @param provider pointer to cwa1890 SM provider
 	* @param from APDU to be encoded
 	* @param to resulting APDU to be sent to encode procedure
 	* @return SC_SUCCESS if OK, else error code
 	*/
	int (*cwa_encode_pre_ops) (sc_card_t * card,
				   struct cwa_provider_st * provider,
				   sc_apdu_t * from, sc_apdu_t * to);

	/**
 	* Operation to be done after APDU encode process finished ok.
 	*
 	* @param card Pointer to card driver data structure
 	* @param provider pointer to cwa1890 SM provider
 	* @param from encoded APDU
 	* @param to resulting encoded APDU to be returned to libopensc
 	* @return SC_SUCCESS if OK, else error code
 	*/
	int (*cwa_encode_post_ops) (sc_card_t * card,
				    struct cwa_provider_st * provider,
				    sc_apdu_t * from, sc_apdu_t * to);

    /************** operations related APDU response decoding **************/

	/**
 	* Operation to be done before any APDU Response decode procedure.
 	*
 	* @param card Pointer to card driver data structure
 	* @param provider pointer to cwa1890 SM provider
 	* @param from APDU Response to be decoded
 	* @param to resulting APDU response to be sent to decode procedure
 	* @return SC_SUCCESS if OK, else error code
 	*/
	int (*cwa_decode_pre_ops) (sc_card_t * card,
				   struct cwa_provider_st * provider,
				   sc_apdu_t * from, sc_apdu_t * to);

	/**
 	* Operation to be done after APDU Response decode process finished ok.
 	*
 	* @param card Pointer to card driver data structure
 	* @param provider pointer to cwa1890 SM provider
 	* @param from decoded APDU Response
 	* @param to resulting APDU Response to be returned to libopensc
 	* @return SC_SUCCESS if OK, else error code
 	*/
	int (*cwa_decode_post_ops) (sc_card_t * card,
				    struct cwa_provider_st * provider,
				    sc_apdu_t * from, sc_apdu_t * to);
} cwa_provider_t;

/************************** external function prototypes ******************/

/**
 * Create Secure channel.
 *
 * Based on Several documents:
 * - "Understanding the DNIe"
 * - "Manual de comandos del DNIe"
 * - ISO7816-4 and CWA14890-{1,2}
 *
 * @param card card info structure
 * @param provider pointer to cwa provider
 * @param flag Requested SM final state (OFF,COLD,WARM)
 * @return SC_SUCCESS if OK; else error code
 */
extern int cwa_create_secure_channel(sc_card_t * card,
				     cwa_provider_t * provider, int flag);

/**
 * Decode an APDU response.
 *
 * Calling this functions means that It's has been verified
 * That apdu response comes in TLV encoded format and needs decoding
 * Based on section 9 of CWA-14890 and Sect 6 of iso7816-4 standards
 * And DNIe's manual
 *
 * @param card card info structure
 * @param provider cwa provider data to handle SM channel
 * @param from apdu to be decoded
 * @param to   where to store decoded apdu
 * @return SC_SUCCESS if ok; else error code
 */
extern int cwa_decode_response(sc_card_t * card,
			       cwa_provider_t * provider,
			       sc_apdu_t * from, sc_apdu_t * to);

/**
 * Encode an APDU.
 *
 * Calling this functions means that It's has been verified
 * That source apdu needs encoding
 * Based on section 9 of CWA-14890 and Sect 6 of iso7816-4 standards
 * And DNIe's manual
 *
 * @param card card info structure
 * @param provider cwa provider data to handle SM channel
 * @param from apdu to be encoded
 * @param to Where to store encoded apdu
 * @return SC_SUCCESS if ok; else error code
 */
extern int cwa_encode_apdu(sc_card_t * card,
			   cwa_provider_t * provider,
			   sc_apdu_t * from, sc_apdu_t * to);

/**
 * Gets a default cwa_provider structure.
 *
 * @param card Pointer to card driver information
 * @return default cwa_provider data, or null on error
 */
extern cwa_provider_t *cwa_get_default_provider(sc_card_t * card);

#endif				/* ENABLE_OPENSSL */

#endif
