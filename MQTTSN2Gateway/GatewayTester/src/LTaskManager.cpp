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
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 *    Ian Craggs - Updating to MQTT-SN 2.0
 **************************************************************************************/

#include <stdio.h>
#include <string.h>

#include "LMqttsnClientApp.h"
#include "LSubscribeManager.h"
#include "LTimer.h"
#include "LMqttsnClient.h"
#include "LTaskManager.h"
#include "LScreen.h"

using namespace std;
using namespace linuxAsyncClient;

extern LMqttsnClient* theClient;
extern LScreen* theScreen;
extern bool theClientMode;
/*=====================================
 TaskManager
 ======================================*/
LTaskManager::LTaskManager(void)
{
    _tasks = 0;
    _tests = 0;
    _index = 0;
}

LTaskManager::~LTaskManager(void)
{

}

void LTaskManager::add(TaskList* task)
{
    _tasks = task;
}

void LTaskManager::add(TestList* test)
{
    _tests = test;
}

void LTaskManager::run(void)
{
    int i = 0;
    char c = 0;
    bool cancelFlg = false;
    TestList test = { 0 };
    TaskList task = { 0 };

    if (!theClientMode)
    {
        theClient->getGwProxy()->getMessage();

        for (i = 0; _tests[i].testTask > test.testTask; i++)
        {
            PROMPT("Execute \"%s\" ? ( y/n ) : ", _tests[i].testLabel);
            while (true)
            {
                if (CHECKKEYIN(&c))
                {
                    if (toupper(c) == 'N')
                    {

                        DISPLAY("\033[0m\033[0;32m\n**** %s is canceled ****\033[0m\033[0;37m\n\n", _tests[i].testLabel);
                        theScreen->prompt("");
                        cancelFlg = true;
                        break;
                    }
                    else if (toupper(c) == 'Y')
                    {
                        DISPLAY("\033[0m\033[0;32m\n\n**** %s start ****\033[0m\033[0;37m\n", _tests[i].testLabel);
                        theScreen->prompt("");
                        (_tests[i].testTask)();
                        cancelFlg = false;
                        break;
                    }
                }
                else
                {
                    theClient->getGwProxy()->getMessage();
                }
            }

            while (true)
            {
                do
                {
                    theClient->getGwProxy()->getMessage();
                }
                while (theClient->getPublishManager()->isMaxFlight() || !theClient->getSubscribeManager()->isDone()
                        || !theClient->getRegisterManager()->isDone());

                if (theClient->getPublishManager()->isDone())
                {
                    break;
                }
            }
            if (!cancelFlg)
            {
                DISPLAY("\033[0m\033[0;32m\n**** %s complete ****\033[0m\033[0;37m\n\n", _tests[i].testLabel);
            }
        }
        DISPLAY("\033[0m\033[0;32m\n\n#########  All tests complete!  ###########\033[0m\033[0;37m\n\n");
    }
    else
    {
        while (true)
        {
            theClient->getGwProxy()->getMessage();
            for (_index = 0; _tasks[_index].callback > task.callback; _index++)
            {
                if ((_tasks[_index].prevTime + _tasks[_index].interval <= time(NULL)) && _tasks[_index].count == 0)
                {
                    _tasks[_index].prevTime = time(NULL);
                    (_tasks[_index].callback)();
                }
            }

            do
            {
                theClient->getGwProxy()->getMessage();
            }
            while (theClient->getPublishManager()->isMaxFlight() || !theClient->getSubscribeManager()->isDone()
                    || !theClient->getRegisterManager()->isDone());

            if (theClient->getPublishManager()->isDone())
            {
                break;
            }
        }
    }
}

uint8_t LTaskManager::getIndex(void)
{
    return _index;
}

void LTaskManager::done(uint8_t index)
{
    if (_tasks)
    {
        if (_tasks[index].count > 0)
        {
            _tasks[index].count--;
        }
    }
    if (_tests)
    {
        if (_tests[index].count > 0)
        {
            _tests[index].count--;
        }
    }
}

void LTaskManager::suspend(uint8_t index)
{
    if (_tasks)
    {
        _tasks[index].count++;
    }
    if (_tests)
    {
        _tests[index].count++;
    }
}
