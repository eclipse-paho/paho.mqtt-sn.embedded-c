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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdarg.h>
#include <signal.h>
#include <Timer.h>
#include <exception>
#include <getopt.h>
#include <unistd.h>
#include "MQTTSNGWProcess.h"
#include "Threading.h"

using namespace std;
using namespace MQTTSNGW;

char* currentDateTime(void);

/*=====================================
 Global Variables & Functions
 ======================================*/
Process* MQTTSNGW::theProcess = nullptr;
MultiTaskProcess* MQTTSNGW::theMultiTaskProcess = nullptr;

/*
 *  Save the type of signal
 */
volatile int theSignaled = 0;

static void signalHandler(int sig)
{
    theSignaled = sig;
}

/*=====================================
 Class Process
 ====================================*/
Process::Process()
{
    _argc = 0;
    _argv = 0;
    _configDir = CONFIG_DIRECTORY;
    _configFile = CONFIG_FILE;
    _log = 0;
    _rbsem = NULL;
    _rb = NULL;
}

Process::~Process()
{
    if (_rb)
    {
        delete _rb;
    }
    if (_rbsem)
    {
        delete _rbsem;
    }
}

void Process::run()
{

}

void Process::initialize(int argc, char** argv)
{
    char param[MQTTSNGW_PARAM_MAX];
    _argc = argc;
    _argv = argv;
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);

    int opt;
    while ((opt = getopt(_argc, _argv, "f:")) != -1)
    {
        if (opt == 'f')
        {
            string config = string(optarg);
            size_t pos = 0;
            if ((pos = config.find_last_of("/")) == string::npos)
            {
                _configFile = optarg;
            }
            else
            {
                _configFile = config.substr(pos + 1, config.size() - pos - 1);
                _configDir = config.substr(0, pos + 1);
            }
        }
    }
    _rbsem = new NamedSemaphore(MQTTSNGW_RB_SEMAPHOR_NAME, 0);
    _rb = new RingBuffer(_configDir.c_str());

    if (getParam("ShearedMemory", param) == 0)
    {
        if (!strcasecmp(param, "YES"))
        {
            _log = 1;
        }
        else
        {
            _log = 0;
        }
    }
}

void Process::putLog(const char* format, ...)
{
    _mt.lock();
    va_list arg;
    va_start(arg, format);
    vsnprintf(_rbdata, sizeof(_rbdata), format, arg);
    va_end(arg);
    if (strlen(_rbdata))
    {
        if (_log > 0)
        {
            _rb->put(_rbdata);
            _rbsem->post();
        }
        else
        {
            printf("%s", _rbdata);
        }
    }
    _mt.unlock();
}

int Process::getArgc()
{
    return _argc;
}

char** Process::getArgv()
{
    return _argv;
}

int Process::getParam(const char* parameter, char* value)
{
    char str[MQTTSNGW_PARAM_MAX];
    char param[MQTTSNGW_PARAM_MAX];
    memset(str, 0, sizeof(str));
    memset(param, 0, sizeof(param));
    FILE *fp;

    int i = 0, j = 0;
    string configPath = _configDir + _configFile;

    if ((fp = fopen(configPath.c_str(), "r")) == NULL)
    {
        throw Exception("Config file not found:\n\nUsage: Command -f path/config_file_name\n", 0);
    }

    int paramlen = strlen(parameter);

    while (true)
    {
        int pos = 0;
        int len = 0;
        if (fgets(str, MQTTSNGW_PARAM_MAX - 1, fp) == NULL)
        {
            fclose(fp);
            return -3;
        }
        if (str[0] == '#' || str[0] == '\n')
        {
            continue;
        }

        len = strlen(str);
        for (pos = 0; i < len; pos++)
        {
            if (str[pos] == '=')
            {
                break;
            }
        }

        if (pos == paramlen)
        {
            if (strncmp(str, parameter, paramlen) == 0)
            {
                strcpy(param, str + pos + 1);
                param[len - pos - 2] = '\0';


                for (i = strlen(param) - 1; i >= 0 && isspace(param[i]); i--)
                    ;
                param[i + 1] = '\0';
                for (i = 0; isspace(param[i]); i++)
                    ;
                if (i > 0)
                {
                    j = 0;
                    while (param[i])
                        param[j++] = param[i++];
                    param[j] = '\0';
                }
                strcpy(value, param);
                fclose(fp);
                return 0;
            }
        }
    }
    fclose(fp);
    return -2;
}

const char* Process::getLog()
{
    int len = 0;
    _mt.lock();
    while ((len = _rb->get(_rbdata, PROCESS_LOG_BUFFER_SIZE)) == 0)
    {
        _rbsem->timedwait(1000);
        if (checkSignal() == SIGINT)
        {
            break;
        }
    }
    *(_rbdata + len) = 0;
    _mt.unlock();
    return _rbdata;
}

void Process::resetRingBuffer()
{
    _rb->reset();
}

int Process::checkSignal(void)
{
    return theSignaled;
}

const string* Process::getConfigDirName(void)
{
    return &_configDir;
}

const string* Process::getConfigFileName(void)
{
    return &_configFile;
}

/*=====================================
 Class MultiTaskProcess
 ====================================*/
MultiTaskProcess::MultiTaskProcess()
{
    theMultiTaskProcess = this;
    _threadCount = 0;
    _stopCount = 0;
}

MultiTaskProcess::~MultiTaskProcess()
{
    for (int i = 0; i < _threadCount; i++)
    {
        _threadList[i]->stop();
    }
}

void MultiTaskProcess::initialize(int argc, char** argv)
{
    Process::initialize(argc, argv);
    for (int i = 0; i < _threadCount; i++)
    {
        _threadList[i]->initialize(argc, argv);
    }

}

void MultiTaskProcess::run(void)
{
    for (int i = 0; i < _threadCount; i++)
    {
        _threadList[i]->start();
    }

    while (true)
    {
        if (theProcess->checkSignal() == SIGINT)
        {
            return;
        }
        sleep(1);
    }
}

void MultiTaskProcess::waitStop(void)
{
    while (_stopCount < _threadCount)
    {
        sleep(1);
    }
}

void MultiTaskProcess::threadStopped(void)
{
    _mutex.lock();
    _stopCount++;
    _mutex.unlock();

}

void MultiTaskProcess::abort(void)
{
    signalHandler(SIGINT);
}

void MultiTaskProcess::attach(Thread* thread)
{
    _mutex.lock();
    if (_threadCount < MQTTSNGW_MAX_TASK)
    {
        _threadList[_threadCount] = thread;
        _threadCount++;
    }
    else
    {
        _mutex.unlock();
        throw Exception("The maximum number of threads has been exceeded.", -1);
    }
    _mutex.unlock();
}

int MultiTaskProcess::getParam(const char* parameter, char* value)
{
    _mutex.lock();
    int rc = Process::getParam(parameter, value);
    _mutex.unlock();
    return rc;
}

/*=====================================
 Class Exception
 ======================================*/
Exception::Exception(const char* message, const int errNo)
{
    _message = message;
    _errNo = errNo;
    _fileName = nullptr;
    _functionName = nullptr;
    _line = 0;
}
Exception::Exception(const char* message, const int errNo, const char* file, const char* function, const int line)
{
    _message = message;
    _errNo = errNo;
    _fileName = getFileName(file);
    ;
    _functionName = function;
    _line = line;
}

Exception::~Exception() throw ()
{

}

const char* Exception::what() const throw ()
{
    return _message;
}

const char* Exception::getFileName()
{
    return _fileName;
}

const char* Exception::getFunctionName()
{
    return _functionName;
}

const int Exception::getLineNo()
{
    return _line;
}

const int Exception::getErrNo()
{
    return _errNo;
}

void Exception::writeMessage()
{
    if (_fileName == nullptr)
    {
        if (_errNo == 0)
        {
            WRITELOG("%s%s %s%s\n", currentDateTime(), RED_HDR, _message, CLR_HDR);
        }
        else
        {
            WRITELOG("%s%s %s.\n                    errno=%d : %s%s\n", currentDateTime(), RED_HDR, _message, _errNo,
                    strerror(_errNo), CLR_HDR);
        }
    }
    else
    {
        if (_errNo == 0)
        {
            WRITELOG("%s%s %s.  %s line %-4d %s()%s\n", currentDateTime(), RED_HDR, _message, _fileName, _line, _functionName,
            CLR_HDR);
        }
        else
        {
            WRITELOG("%s%s %s.  %s line %-4d %s()\n                    errno=%d : %s%s\n", currentDateTime(), RED_HDR, _message,
                    _fileName, _line, _functionName, _errNo, strerror(_errNo), CLR_HDR);
        }
    }
}

const char* Exception::getFileName(const char* file)
{
    for (int len = strlen(file); len > 0; len--)
    {
        if (*(file + len) == '/')
        {
            return file + len + 1;
        }
    }
    return file;
}

