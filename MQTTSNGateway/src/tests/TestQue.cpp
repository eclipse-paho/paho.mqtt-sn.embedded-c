/**************************************************************************************
 * Copyright (c) 2016, Tomoaki Yamaguchi
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
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation 
 **************************************************************************************/
#include <string.h>
#include <tests/TestProcess.h>
#include <cassert>
#include "TestQue.h"
using namespace std;
using namespace MQTTSNGW;

TestQue::TestQue()
{

}

TestQue::~TestQue()
{

}

void TestQue::test(void)
{
	int* v = 0;
	int i = 0;

	for ( i = 0; i < 10; i++ )
	{
		v = new int(i);
		this->post(v);
	}
	assert( 10 == this->size());

	for ( i = 0; i < 10; i++ )
	{
		assert(i == *this->front());
		int* p = this->front();
		if ( p )
		{
			assert(i == *p);
			this->pop();
			delete p;
		}
	}
	assert(0 == this->front());
	assert(0 == this->size());

	this->setMaxSize(5);
	for ( i = 0; i < 10; i++ )
	{
		v = new int(i);
		this->post(v);
		assert( 5 >= this->size());
	}
	for ( i = 0; i < 10; i++ )
	{
		int* p = this->front();
		if ( p )
		{
			this->pop();
			delete p;
		}
	}
	printf("[ OK ]\n");
}

int* TestQue::front(void)
{
	return _que.front();
}
void TestQue::pop(void)
{
	_que.pop();
}
int TestQue::size(void)
{
	return _que.size();
}
void TestQue::setMaxSize(int maxsize)
{
	_que.setMaxSize(maxsize);
}

void TestQue::post(int* val)
{
	_que.post(val);
}


