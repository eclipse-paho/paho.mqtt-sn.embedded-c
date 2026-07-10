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
#include "TestProcess.h"
#include "TestTask.h"

using namespace MQTTSNGW;

TestProcess* test = new TestProcess();
//TestTask* task = new TestTask(test);

int main(int argc, char** argv)
{
	test->initialize(argc, argv);
	test->run();
	delete test;
	printf("\nPass all tests. \n");
	return 0;
}

