/**
 * @file libA71CH_api.h
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
 * This file provides the public interface of the Atmark Techno A71CH module.
 * @par History
 * 1.0   20-sep-2019 : Initial version
 *
 *****************************************************************************/
#ifndef _LIBA71CH_API_
#define _LIBA71CH_API_

#ifdef __cplusplus
extern "C" {
#endif
	int LIBA71CH_open();
	int LIBA71CH_resetModule();
	void LIBA71CH_finalize();

	int LIBA71CH_getKeyAndCert(char* key, char* cert);
	int LIBA71CH_setKeyAndCert(const char* key, const char* cert);
	int LIBA71CH_eraseKeyAndCert();
#ifdef __cplusplus
}
#endif
#endif	/* _LIBA71CH_API_ */
