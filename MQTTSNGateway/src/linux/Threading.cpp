/**************************************************************************************
 * Copyright (c) 2016, Tomoaki Yamaguchi
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/

#include "MQTTSNGWProcess.h"
#include "Threading.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

using namespace std;
using namespace MQTTSNGW;

#ifdef __APPLE__

int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout)
{
	while (true)
	{
		// try to lock the semaphore
		int result = sem_trywait(sem);
		if (result != -1 || errno != EAGAIN)
			return result;

		// spin lock
		sched_yield();

		// check if timeout reached
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		if (ts.tv_sec > abs_timeout->tv_sec
			|| (ts.tv_sec == abs_timeout->tv_sec && abs_timeout->tv_nsec >= ts.tv_nsec))
		{
			return ETIMEDOUT;
		}
	}
}

#endif

/*=====================================
 Class Mutex
 =====================================*/

Mutex::Mutex(void)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutex_init(&_mutex, &attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	_shmid = 0;
	_pmutex = 0;
}

Mutex::Mutex(const char* fileName)
{
	pthread_mutexattr_t attr;

	key_t key = ftok(fileName, 1);

	if ((_shmid = shmget(key, sizeof(pthread_mutex_t), IPC_CREAT | 0666)) < 0)
	{
		throw Exception("Mutex can't create a shared memory.", -1);
	}
	_pmutex = (pthread_mutex_t*) shmat(_shmid, NULL, 0);
	if (_pmutex == (void*) -1)
	{
		throw Exception("Mutex can't attach shared memory.", -1);
	}

	pthread_mutexattr_init(&attr);

#ifdef __CYGWIN__
	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
	{
		throw Exception("Mutex can't set the recursive flag", -1);
	}
#endif

	if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0)
		{
#ifdef __CYGWIN__
		printf("WARNING: Mutex can't set the process-shared flag\n");
#else
		throw Exception("Mutex can't set the process-shared flag", -1);
#endif
	}
	if (pthread_mutex_init(_pmutex, &attr) != 0)
	{
		throw Exception("Mutex can't initialize.", -1);
	}
}

Mutex::~Mutex(void)
{
	if (_pmutex)
	{
		pthread_mutex_lock(_pmutex);
		pthread_mutex_unlock(_pmutex);
		pthread_mutex_destroy(_pmutex);
	}
	else
	{
		pthread_mutex_lock(&_mutex);
		pthread_mutex_unlock(&_mutex);
		pthread_mutex_destroy(&_mutex);
	}
	if (_shmid)
	{
		shmctl(_shmid, IPC_RMID, NULL);
	}
}

void Mutex::lock(void)
{
	int rc = 0;
	if (_pmutex)
	{
		rc = pthread_mutex_lock(_pmutex);
	}
	else
	{
		rc = pthread_mutex_lock(&_mutex);
	}

	if (rc)
	{
		throw Exception("Mutex lock error", errno);
	}
}

void Mutex::unlock(void)
{
	int rc = 0;
	if (_pmutex)
	{
		rc = pthread_mutex_unlock(_pmutex);
	}
	else
	{
		rc = pthread_mutex_unlock(&_mutex);
	}

	if (rc)
	{
		throw Exception("Mutex lock error", errno);
	}
}

/*=====================================
 Class Semaphore
 =====================================*/

Semaphore::Semaphore(unsigned int val)
{
#ifdef __APPLE__
	_sem = dispatch_semaphore_create(val);
#else
	sem_init(&_sem, 0, val);
#endif
}

Semaphore::~Semaphore()
{
#ifdef __APPLE__
	dispatch_release(_sem);
#else
	sem_destroy(&_sem);
#endif
}

void Semaphore::post(void)
{
#ifdef __APPLE__
	dispatch_semaphore_signal(_sem);
#else
	sem_post(&_sem);
#endif
}

void Semaphore::wait(void)
{
#ifdef __APPLE__
	dispatch_semaphore_wait(_sem, DISPATCH_TIME_FOREVER);
#else
	sem_wait(&_sem);
#endif
}

void Semaphore::timedwait(uint16_t millsec)
{
#ifdef __APPLE__
	dispatch_semaphore_wait(_sem, dispatch_time(DISPATCH_TIME_NOW, int64_t(millsec) * 1000000));
#else
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	int nsec = ts.tv_nsec + (millsec % 1000) * 1000000;
	ts.tv_nsec = nsec % 1000000000;
	ts.tv_sec += millsec / 1000 + nsec / 1000000000;
	sem_timedwait(&_sem, &ts);
#endif
}

/*=====================================
 Class NamedSemaphore
 =====================================*/

NamedSemaphore::NamedSemaphore(const char* name, unsigned int val)
{
	_psem = sem_open(name, O_CREAT, 0666, val);
	if (_psem == SEM_FAILED)
	{
		throw Exception("Semaphore can't be created.", -1);
	}
	_name = strdup(name);
	if (_name == NULL)
	{
		throw Exception("Semaphore can't allocate memories.", -1);
	}
}

NamedSemaphore::~NamedSemaphore()
{
	sem_close(_psem);
	sem_unlink(_name);
	free(_name);
}

void NamedSemaphore::post(void)
{
	sem_post(_psem);
}

void NamedSemaphore::wait(void)
{
	sem_wait(_psem);
}

void NamedSemaphore::timedwait(uint16_t millsec)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += millsec / 1000;
	ts.tv_nsec = (millsec % 1000) * 1000000;
	sem_timedwait(_psem, &ts);
}

/*=========================================
 Class RingBuffer
 =========================================*/
RingBuffer::RingBuffer(const char* keyDirectory)
{
	int fp = 0;
	string fileName = keyDirectory + string(MQTTSNGW_RINGBUFFER_KEY);
	fp = open(fileName.c_str(), O_CREAT, S_IRGRP);
	if ( fp > 0 )
	{
		close(fp);
	}

	fileName = keyDirectory + string(MQTTSNGW_RB_MUTEX_KEY);
	fp = open(fileName.c_str(), O_CREAT, S_IRGRP);
	if ( fp > 0 )
	{
		close(fp);
	}

	key_t key = ftok(MQTTSNGW_RINGBUFFER_KEY, 1);

	if ((_shmid = shmget(key, PROCESS_LOG_BUFFER_SIZE,
	IPC_CREAT | IPC_EXCL | 0666)) >= 0)
	{
		if ((_shmaddr = (uint16_t*) shmat(_shmid, NULL, 0)) != (void*) -1)
		{
			_length = (uint16_t*) _shmaddr;
			_start = (uint16_t*) _length + sizeof(uint16_t*);
			_end = (uint16_t*) _start + sizeof(uint16_t*);
			_buffer = (char*) _end + sizeof(uint16_t*);
			_createFlg = true;

			*_length = PROCESS_LOG_BUFFER_SIZE - sizeof(uint16_t*) * 3 - 16;
			*_start = *_end = 0;
		}
		else
		{
			throw Exception("RingBuffer can't attach shared memory.", -1);
		}
	}
	else if ((_shmid = shmget(key, PROCESS_LOG_BUFFER_SIZE, IPC_CREAT | 0666)) != -1)
	{
		if ((_shmaddr = (uint16_t*) shmat(_shmid, NULL, 0)) != (void*) -1)
		{
			_length = (uint16_t*) _shmaddr;
			_start = (uint16_t*) _length + sizeof(uint16_t*);
			_end = (uint16_t*) _start + sizeof(uint16_t*);
			_buffer = (char*) _end + sizeof(uint16_t*);
			_createFlg = false;
		}
		else
		{
			throw Exception("RingBuffer can't create a shared memory.", -1);
		}
	}
	else
	{
		throw Exception( "RingBuffer can't create a shared memory.", -1);
	}

	_pmx = new Mutex(MQTTSNGW_RB_MUTEX_KEY);
}

RingBuffer::~RingBuffer()
{
	if (_createFlg)
	{
		if (_shmid > 0)
		{
			shmctl(_shmid, IPC_RMID, NULL);
		}
	}
	else
	{
		if (_shmid > 0)
		{
			shmdt(_shmaddr);
		}
	}

	if (_pmx != NULL)
	{
		delete _pmx;
	}
}

void RingBuffer::put(char* data)
{
	_pmx->lock();

	uint16_t dlen = strlen(data);
	uint16_t blen = *_length - *_end;

	if (*_end > *_start)
	{
		if (dlen < blen)
		{
			strncpy(_buffer + *_end, data, dlen);
			if (*_end - *_start == 1)
			{ // Buffer is empty.
				*_start = *_end;
			}
			*_end += dlen;
		}
		else
		{
			strncpy(_buffer + *_end, data, blen);
			strncpy(_buffer, data + blen, dlen - blen);
			if (*_end - *_start == 1)
			{ // Buffer is empty.
				*_start = *_end;
				*_end = dlen - blen;
			}
			else
			{
				*_end = dlen - blen;
				*_start = *_end + 1;
			}
		}
	}
	else if (*_end == *_start)
	{
		if (dlen < blen)
		{
			strncpy(_buffer + *_end, data, dlen);
			*_end += dlen;
		}
		else
		{
			const char* errmsg = "RingBuffer Error: data is too long";
			strcpy(_buffer + *_end, errmsg);
			*_end += strlen(errmsg);
		}
	}
	else
	{    // *_end < *_start
		if (dlen < *_start - *_end)
		{
			strncpy(_buffer + *_end, data, dlen);
			*_end += dlen;
			*_start = *_end + 1;
		}
		else
		{
			if (dlen < blen)
			{
				strncpy(_buffer + *_end, data, dlen);
				*_end += dlen;
				*_start = *_end + 1;
			}
			else
			{
				strncpy(_buffer + *_end, data, blen);
				strncpy(_buffer, data + blen, dlen - blen);
				*_start = *_end;
				*_end = dlen - blen;
			}
		}
	}
	_pmx->unlock();
}

int RingBuffer::get(char* buf, int length)
{
	int len = 0;
	_pmx->lock();

	if (*_end > *_start)
	{
		if (length > *_end - *_start)
		{
			len = *_end - *_start;
			if (len == 1)
			{
				len = 0;
			}
			strncpy(buf, _buffer + *_start, len);
			*_start = *_end - 1;
		}
		else
		{
			len = length;
			strncpy(buf, _buffer + *_start, len);
			*_start = *_start + len;
		}
	}
	else if (*_end < *_start)
	{
		int blen = *_length - *_start;
		if (length > blen)
		{
			strncpy(buf, _buffer + *_start, blen);
			*_start = 0;
			if (length - (blen + *_end) > 0)
			{
				strncpy(buf + blen, _buffer, *_end);
				len = blen + *_end;
				if (*_end > 0)
				{
					*_start = *_end - 1;
				}
			}
			else
			{
				strncpy(buf + blen, _buffer, length - blen);
				len = length;
				*_start = length - blen;
			}
		}
		else
		{
			strncpy(buf, _buffer + *_start, length);
			*_start += length;
			len = length;
		}
	}
	_pmx->unlock();
	return len;
}

void RingBuffer::reset()
{
	_pmx->lock();
	if ( _start && _end )
	{
		*_start = *_end = 0;
	}
	else
	{
		throw Exception("RingBuffer can't reset. need to clear shared memory.", -1);
	}
	_pmx->unlock();
}

/*=====================================
 Class Thread
 =====================================*/
Thread::Thread()
{
	_threadID = 0;
	_taskName = nullptr;
}

Thread::~Thread()
{
}

void* Thread::_run(void* runnable)
{
	static_cast<Runnable*>(runnable)->EXECRUN();
	return 0;
}

void Thread::initialize(int argc, char** argv)
{

}

pthread_t Thread::getID()
{
	return pthread_self();
}

bool Thread::equals(pthread_t *t1, pthread_t *t2)
{
	return (pthread_equal(*t1, *t2) ? false : true);
}

int Thread::start(void)
{
    Runnable* runnable = this;
	return pthread_create(&_threadID, 0, _run, runnable);
}

void Thread::stop(void)
{
	if ( _threadID )
	{
		pthread_join(_threadID, NULL);
		_threadID = 0;
	}
}

void Thread::setTaskName(const char* name)
{
    _taskName = name;
}

const char* Thread::getTaskName(void)
{
    return _taskName;
}
