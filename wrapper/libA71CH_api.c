/**
 * @file libA71CH_api.c
 * @author Atmark Techno
 * @version 1.0
 * @par License
 * Copyright 2019 Atmark Techno
 *
 * This software is owned or controlled by Atmark Techno and may only be used
 * strictly in accordance with the applicable license terms.  By expressly
 * accepting such terms or by downloading, installing, activating and/or
 * otherwise using the software, you are agreeing that you have read, and
 * that you agree to comply with and are bound by, such license terms.  If
 * you do not agree to be bound by the applicable license terms, then you
 * may not retain, install, activate or otherwise use the software.
 *
 * @par Description
 * This file provides the public interface of the Atmark Techno A71CH library.
 * @par History
 * 1.0   20-sep-2019 : Initial version
 *
 *****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libA71CH_api.h"
#include "a71ch_api.h"
#include "a71ch_const.h"
#include "a71_debug.h"
#include "ax_api.h"

static const unsigned short GP_UNIT_BYTE = 128;
static const unsigned short GP_UNIT_NUM = 32;

#if 0
static const unsigned char unlockKey[16] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};
#endif

static int isOk(unsigned short sw)
{
	return (sw == SW_OK)?1:0;
}

#if 0
static int lock()
{
	unsigned short sw;
	sw = A71_LockModule();
	return isOk(sw)?0:-1;
}

static int unlock()
{
	unsigned short sw;
	unsigned char  challenge[A71CH_MODULE_UNLOCK_CHALLENGE_LEN];
	unsigned short challengeLen = sizeof(challenge);
	U8 unlockCode[A71CH_MODULE_UNLOCK_CHALLENGE_LEN];
	int hcRet;

	sw = A71_GetUnlockChallenge(challenge, &challengeLen);
	if (!isOk(sw)){
		return -1;
	}

	/* Decrypt challenge */
	hcRet = HOST_AES_ECB_DECRYPT(unlockCode, challenge,
						unlockKey, sizeof(unlockKey));
	if (hcRet != HOST_CRYPTO_OK) {
		return -1;
	}

	sw = A71_UnlockModule(unlockCode, 16);
	return isOk(sw)?0:-1;
}
#endif

int LIBA71CH_open()
{
	unsigned short sw;
	unsigned char atr[64];
	unsigned short atrSize = sizeof(atr);
	SmCommState_t commState;

	sw = SM_Connect(&commState, atr, &atrSize);
	if (!isOk(sw)) {
		return -1;
	}
	return 0;
}

int LIBA71CH_resetModule()
{
	unsigned short sw;
	sw = A71_DbgReset();
	if (!isOk(sw)) {
		return -1;
	}
	return 0;
}

void LIBA71CH_finalize()
{
	SM_Close(SMCOM_CLOSE_MODE_STD);
}

static void sizeToUc(const unsigned short size, unsigned char* uc)
{
	uc[0] = size & 0xff;
	uc[1] = (size >>  8) & 0xff;
	uc[2] = 0;
	uc[3] = 0;
}

static unsigned short ucToSize(const unsigned char* uc)
{
	return	uc[0] | (uc[1] << 8) ;
}

static unsigned int setGpData(const char* str, const unsigned short dataSize,
						const unsigned short offset)
{
	unsigned char data[GP_UNIT_BYTE];
	unsigned short remainSize;
	unsigned short currentSize;
	unsigned short sw;
	unsigned short currentOffset;

	remainSize = dataSize;
	currentSize = 0;
	currentOffset = offset;

	while(remainSize) {
		currentSize = (remainSize < GP_UNIT_BYTE)?remainSize
							 :GP_UNIT_BYTE;
		sw = A71_SetGpData(currentOffset, data, currentSize);
		if (!isOk(sw)) {
			return 0;
		}
		currentOffset += currentSize;
		remainSize -= currentSize;
	}

	if (currentSize % GP_UNIT_BYTE) {
		currentOffset += (GP_UNIT_BYTE - (currentSize % GP_UNIT_BYTE));
	}
	return currentOffset;
}

static unsigned int gpData(char* str,
			   const unsigned short dataSize,
			   const unsigned short offset)
{
	unsigned char* data;
	unsigned short remainSize;
	unsigned short currentSize;
	unsigned short sw;
	unsigned short currentOffset;

	data = (unsigned char*)str;
	remainSize = dataSize;
	currentSize = 0;
	currentOffset = offset;

	while(remainSize) {
		currentSize = (remainSize < GP_UNIT_BYTE)?remainSize
							 :GP_UNIT_BYTE;
		sw = A71_GetGpData(currentOffset, data, currentSize);
		if (!isOk(sw)) {
			return 0;
		}

		data += currentSize;
		currentOffset += currentSize;
		remainSize -= currentSize;
		
	}

	if (currentSize % GP_UNIT_BYTE) {
		currentOffset += (GP_UNIT_BYTE - (currentSize % GP_UNIT_BYTE));
	}
	return currentOffset;
}

/**
 * @brief	get Key and Cert
 * @param	key: key string
 * @param	cert: certificate string
 * @return	0:OK, -1:NG
 * @note	get on GP data
 * @note	GP data slot 0,
 *			0- 3byte: key length
 *			4- 7byte: certificate length
 *		after GP data slot 1, put key -> cert.
 */
int LIBA71CH_getKeyAndCert(char* key, char* cert)
{
	unsigned short sw;
	unsigned short keySize;
	unsigned short certSize;
	unsigned char data[GP_UNIT_BYTE];
	unsigned short offset;

	if (!key || !cert) {
		return -1;
	}

	/* get length from GP Data slot0 */
	sw = A71_GetGpData(0, data, GP_UNIT_BYTE);
	if (!isOk(sw)) {
		return -1;
	}
	keySize = ucToSize(&data[0]);
	certSize = ucToSize(&data[4]);

	if (!keySize) {
		return -1;
	}

	if (!certSize) {
		return -1;
	}

	offset = GP_UNIT_BYTE;
	/* get key */
	offset = gpData(key, keySize, offset);
	if (!offset) {
		return -1;
	}

	/* get certification */
	offset = gpData(cert, certSize, offset);
	if (!offset) {
		return -1;
	}

	return 0;
}

/**
 * @brief	set Key and Cert
 * @param	key: key data
 * @param	cert: certificate data
 * @return	0:OK, -1:NG
 * @note	set on GP data
 * @note	GP data slot 0,
 *			0- 3byte: key length
 *			4- 7byte: certificate length
 *		after GP data slot 1, put key -> cert.
 */
int LIBA71CH_setKeyAndCert(const char* key, const char* cert)
{
	unsigned short sw;
	unsigned short keySize;
	unsigned short certSize;
	unsigned char keySizeUc[4];
	unsigned char certSizeUc[4];
	unsigned char data[GP_UNIT_BYTE];
	unsigned int offset;

	if (!key || !cert) {
		return -1;
	}

	/* get key file size */
	keySize = strlen(key);
	if(!keySize) {
		return -1;
	}
	sizeToUc(keySize, keySizeUc);

	/* get certificate file size */
	certSize = strlen(cert);
	if(!certSize) {
		return -1;
	}
	sizeToUc(certSize, certSizeUc);

	/* set length to GP Data slot0 */
	memset(data, 0, GP_UNIT_BYTE);
	memcpy(data, keySizeUc, 4);
	memcpy(&data[4], certSizeUc, 4);
	sw = A71_SetGpData(0, data, GP_UNIT_BYTE);
	if (!isOk(sw)) {
		return -1;
	}

	offset = GP_UNIT_BYTE;

	/* set key */
	offset = setGpData(key, keySize, offset);
	if (!offset) {
		return -1;
	}

	/* set certificate */
	offset = setGpData(cert, certSize, offset);
	if (!offset) {
		return -1;
	}

	return 0;
}

/**
 * @brief	erase Key and Certtificate
 * @return	0:OK, -1:NG
 * @note	delete all GP data
 */
int LIBA71CH_eraseKeyAndCert()
{
	unsigned short sw;
	unsigned char data[GP_UNIT_BYTE];
	unsigned short i;

	memset(data, 0, GP_UNIT_BYTE);
	for(i=0; i<GP_UNIT_NUM; i++) {
		sw = A71_SetGpData(i*GP_UNIT_BYTE, data, GP_UNIT_BYTE);
		if (!isOk(sw)) {
		}
	}
	return 0;
}

