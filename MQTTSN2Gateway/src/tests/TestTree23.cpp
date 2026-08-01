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
#include "TestTree23.h"
#include <stdio.h>
#include <string>
#include <cassert>

using namespace std;
using namespace MQTTSNGW;

TestTree23::TestTree23()
{

}

TestTree23::~TestTree23()
{

}

void TestTree23::test(void)
{
	int N = 100;

	Key* r1[100];
	Integer* r2[100];

	for ( int i = 0; i < N; i++)
	{
		char buff[5];
		snprintf(buff, sizeof(buff), "%d", i);
		r1[i] = new Key(string(buff));
		r2[i] = new Integer(i);
		this->add(r1[i], r2[i]);
	}

	for ( int i = 0; i < N; i++)
	{
		Integer* rc = this->getVal(r1[i]);
		//printf("key=%d  val=%d\n", i, rc->_val);
		assert(i == rc->_val);
	}

	for ( int i = 20; i < 50; i++)
	{
		this->remove(r1[i]);
		//printf("key=%d  str=%s\n", i, r1[i]->_key.c_str());
	}

	for ( int i = 0; i < 20; i++)
	{
		bool rc = this->find(r1[i]);
		assert(rc == true);
		//printf("key=%d  find=%d\n", i, rc);
		Integer* val = this->getVal(r1[i]);
		//printf("key=%d  val=%d\n", i, val->_val);
		assert(val->_val == i);
	}
	for (  int i = 20; i < 50; i++ )
	{
		bool rc = this->find(r1[i]);
		assert(rc == false);
		//printf("key=%d  find=%d\n", i, rc);
		Integer* val = this->getVal(r1[i]);
		assert(val == 0);
	}
	for ( int i = 50; i < N; i++)
	{
		bool rc = this->find(r1[i]);
		assert(rc == true);
		//printf("key=%d  find=%d\n", i, rc);
		Integer* val = this->getVal(r1[i]);
		//printf("key=%d  val=%d\n", i, val->_val);
		assert(val->_val == i);
	}
	printf("[ OK ]\n");
}

