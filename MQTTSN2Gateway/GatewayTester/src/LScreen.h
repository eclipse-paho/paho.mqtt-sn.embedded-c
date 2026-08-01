/**************************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi, Ian Craggs
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * AI Disclosure: This file was partly AI-generated. The AI-generated
 * portions are made available under CC0-1.0 and not subject to the
 * project's licence. The human contributor has reviewed and verified
 * that the code is correct.
 *
 * SPDX-License-Identifier: EPL-2.0 and CC0-1.0
 *
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation 
 *    Ian Craggs - Updating to MQTT-SN 2.0
 **************************************************************************************/
#ifndef LSCREEN_H_
#define LSCREEN_H_
#include <stdarg.h>
#include <string>
using namespace std;

namespace linuxAsyncClient {
#define SCREEN_BUFF_SIZE 1024
/*========================================
 Class Screen
 =======================================*/
class LScreen{
public:
	LScreen();
	~LScreen();
	void clear(void);
	void display(const char* format, ...);
	void prompt(const char* format, ...);
	bool checkKeyIn(char* val);

private:
	void reprompt(void);
	void getSize(void);
	void vdisplay(const char* format, va_list args);
	void vprompt(const char* format, va_list args);
	char _buffer[SCREEN_BUFF_SIZE];
	int _hight;
	int _width;
	string _prompt;
};

}  /* end of namespace */

#endif /* LSCREEN_H_ */
