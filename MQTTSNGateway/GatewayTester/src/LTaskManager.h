/**************************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi
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
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/

#ifndef TASKMANAGER_H_
#define TASKMANAGER_H_

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "LMqttsnClientApp.h"
#include "LTimer.h"

using namespace std;

namespace linuxAsyncClient {

struct TaskList{
    void     (*callback)(void);
	time_t   interval;
    time_t    prevTime;
    uint8_t  count;
};

struct TestList {
	const char* testLabel;
	void     (*testTask)(void);
	uint8_t  count;
};

/*========================================
       Class TaskManager
 =======================================*/
class LTaskManager{
public:
    LTaskManager();
    ~LTaskManager();
    void add(TaskList* task);
    void add(TestList* test);
    void run(void);
    void done(uint8_t index);
    void suspend(uint8_t index);
    uint8_t getIndex(void);
private:
    TaskList* _tasks;
    TestList* _tests;
    uint8_t   _index;

};

} /* end of namespace */
#endif /* TASKMANAGER_H_ */

