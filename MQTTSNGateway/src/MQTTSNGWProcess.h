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
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/

#ifndef MQTTSNGWPROCESS_H_
#define MQTTSNGWPROCESS_H_

#include <exception>
#include <string>
#include <signal.h>
#include "MQTTSNGWDefines.h"
#include "Threading.h"

using namespace std;

namespace MQTTSNGW
{

/*=================================
 *    Parameters
 ==================================*/
#define MQTTSNGW_MAX_TASK           10  // number of Tasks
#define PROCESS_LOG_BUFFER_SIZE  16384  // Ring buffer size for Logs
#define MQTTSNGW_PARAM_MAX         128  // Max length of config records.

/*=================================
 *    Macros
 ==================================*/
#define WRITELOG theProcess->putLog
#define CHK_SIGINT (theProcess->checkSignal() == SIGINT)
#define UNUSED(x) ((void)(x))
#define EXCEPTION(...)   Exception(__VA_ARGS__, __FILE__, __func__, __LINE__)

/*=================================
 Class Process
 ==================================*/
class Process
{
public:
    Process();
    virtual ~Process();
    virtual void initialize(int argc, char** argv);
    virtual void run(void);
    void putLog(const char* format, ...);
    void resetRingBuffer(void);
    int getArgc(void);
    char** getArgv(void);
    int getParam(const char* parameter, char* value);
    const char* getLog(void);
    int checkSignal(void);
    const string* getConfigDirName(void);
    const string* getConfigFileName(void);
private:
    int _argc;
    char** _argv;
    string _configDir;
    string _configFile;
    RingBuffer* _rb;
    NamedSemaphore* _rbsem;
    Mutex _mt;
    int _log;
    char _rbdata[PROCESS_LOG_BUFFER_SIZE + 1];
};

/*=====================================
 Class MultiTaskProcess
 ====================================*/
class MultiTaskProcess: public Process
{
public:
    MultiTaskProcess(void);
    ~MultiTaskProcess();
    void initialize(int argc, char** argv);
    int getParam(const char* parameter, char* value);
    void run(void);
    void waitStop(void);
    void threadStopped(void);
    void attach(Thread* thread);
    void abort(void);

private:
    Thread* _threadList[MQTTSNGW_MAX_TASK];
    Mutex _mutex;
    int _threadCount;
    int _stopCount;
};

/*=====================================
 Class Exception
 =====================================*/
class Exception: public exception
{
public:
    Exception(const char* message, const int errNo);
    Exception(const char* message, const int errNo, const char* file, const char* func, int line);
    virtual ~Exception() throw ();
    const char* getFileName();
    const char* getFunctionName();
    const int getLineNo();
    const int getErrNo();
    virtual const char* what() const throw ();
    void writeMessage();

private:
    const char* getFileName(const char* file);
    int _errNo;
    const char* _message;
    const char* _fileName;
    const char* _functionName;
    int _line;
};

/*=====================================
 Class QueElement
 ====================================*/
template<class T>
class QueElement
{
    template<class U> friend class Que;
public:
    QueElement(T* t)
    {
        _element = t;
        _next = nullptr;
        _prev = nullptr;
    }

    ~QueElement()
    {
    }

private:
    T* _element;
    QueElement<T>* _next;
    QueElement<T>* _prev;
};

/*=====================================
 Class Que
 ====================================*/
template<class T>
class Que
{
public:
    Que()
    {
        _head = nullptr;
        _tail = nullptr;
        _cnt = 0;
        _maxSize = 0;
    }

    ~Que()
    {
        QueElement<T>* elm = _head;
        while (elm)
        {
            QueElement<T>* next = elm->_next;
            delete elm->_element;
            delete elm;
            elm = next;
        }
    }

    void pop(void)
    {
        if (_head)
        {
            QueElement<T>* head = _head;
            if (_head == _tail)
            {
                _head = _tail = nullptr;
            }
            else
            {
                _head = head->_next;
                head->_prev = nullptr;
            }
            delete head;
            _cnt--;
        }
    }

    T* front(void)
    {
        {
            if (_head)
            {
                return _head->_element;
            }
            else
            {
                return 0;
            }
        }
    }

    int post(T* t)
    {
        if (t && (_maxSize == 0 || _cnt < _maxSize))
        {
            QueElement<T>* elm = new QueElement<T>(t);
            if (_head)
            {
                if (_tail == _head)
                {
                    elm->_prev = _tail;
                    _tail = elm;
                    _head->_next = elm;
                }
                else
                {
                    _tail->_next = elm;
                    elm->_prev = _tail;
                    _tail = elm;
                }
            }
            else
            {
                _head = elm;
                _tail = elm;
            }
            _cnt++;
            return _cnt;
        }
        return 0;
    }

    int size(void)
    {
        return _cnt;
    }

    void setMaxSize(int maxSize)
    {
        _maxSize = maxSize;
    }

private:
    int _cnt;
    int _maxSize;
    QueElement<T>* _head;
    QueElement<T>* _tail;
};

/*=====================================
 Class Tree23
 ====================================*/
#define TREE23_INSERT_ACTIVE  (-2)
#define TREE23_DELETE_ACTIVE  (-1)
#define TREE23_BI_NODE        (2)
#define TREE23_TRI_NODE       (3)

template<typename K, typename V>
class Tree23Elm
{
    template<typename T, typename U> friend class Tree23;
public:
    Tree23Elm()
    {
        _key = 0;
        _val = 0;
    }

    Tree23Elm(K* key, V* val)
    {
        _key = key;
        _val = val;
    }

    ~Tree23Elm()
    {

    }

    int compare(Tree23Elm<K, V>* elm)
    {
        return _key->compare(elm->_key);
    }
private:
    K* _key;
    V* _val;
};

template<typename K, typename V>
class Tree23Node
{
    template<typename S, typename W> friend class Tree23;
public:
    Tree23Node(const int type)
    {
        _type = type;
        _telm0 = _telm1 = NULL;
        _left = _midle = _right = NULL;
    }

    Tree23Node(const int type, Tree23Node<K, V>* midle)
    {
        _type = type;
        _telm0 = _telm1 = NULL;
        _left = _right = NULL;
        _midle = midle;
    }

    Tree23Node(const int type, Tree23Elm<K, V>* telm)
    {
        _type = type;
        _telm0 = telm;
        _telm1 = NULL;
        _left = _midle = _right = NULL;
    }

    Tree23Node(const int type, Tree23Elm<K, V>* telm, Tree23Node<K, V>* left,
            Tree23Node<K, V>* right)
    {
        _type = type;
        _telm0 = telm;
        _telm1 = NULL;
        _left = left;
        _midle = NULL;
        _right = right;
    }

    Tree23Node(const int type, Tree23Elm<K, V>* telm0, Tree23Elm<K, V>* telm1,
            Tree23Node<K, V>* left, Tree23Node<K, V>* midle,
            Tree23Node<K, V>* right)
    {
        _type = type;
        _telm0 = telm0;
        _telm1 = telm1;
        _left = left;
        _midle = midle;
        _right = right;
    }

    ~Tree23Node()
    {

    }

private:
    int _type;
    Tree23Elm<K, V>* _telm0;
    Tree23Elm<K, V>* _telm1;
    Tree23Node<K, V>* _left;
    Tree23Node<K, V>* _midle;
    Tree23Node<K, V>* _right;
};

template<typename K, typename V>
class Tree23
{
public:
    Tree23()
    {
        _root = NULL;
    }

    ~Tree23()
    {
        if (_root)
        {
            delete _root;
        }
    }

    void add(K* key, V* val)
    {
        _root = add(_root, new Tree23Elm<K, V>(key, val));
        _root->_type = abs(_root->_type);
    }

    Tree23Node<K, V>* add(Tree23Node<K, V>* n, Tree23Elm<K, V>* elm)
    {
        if (n == 0)
        {
            return new Tree23Node<K, V>(TREE23_INSERT_ACTIVE, elm);
        }

        int cmp0 = elm->compare(n->_telm0);
        int cmp1 = 0;
        switch (n->_type)
        {
        case 2:
            if (cmp0 < 0)
            {
                n->_left = add(n->_left, elm);
                return addLeft2(n);
            }
            else if (cmp0 == 0)
            {
                n->_telm0 = elm;
                return n;
            }
            else
            {
                n->_right = add(n->_right, elm);
                return addRight2(n);
            }
            break;
        case 3:
            cmp1 = elm->compare(n->_telm1);
            if (cmp0 < 0)
            {
                n->_left = add(n->_left, elm);
                return addLeft3(n);
            }
            else if (cmp0 == 0)
            {
                n->_telm0 = elm;
                return n;
            }
            else if (cmp1 < 0)
            {
                n->_midle = add(n->_midle, elm);
                return addMidle3(n);
            }
            else if (cmp1 == 0)
            {
                n->_telm1 = elm;
                return n;
            }
            else
            {
                n->_right = add(n->_right, elm);
                return addRight3(n);
            }
            break;
        default:
            break;
        }
        return 0;
    }

    void remove(K* k)
    {
        _root = remove(_root, k);
        if (_root != NULL && _root->_type == TREE23_DELETE_ACTIVE)
        {
            _root = _root->_midle;
        }
    }

    Tree23Node<K, V>* remove(Tree23Node<K, V>* node, K* k)
    {
        if (node == NULL)
        {
            return NULL;
        }
        int cmp0 = k->compare(node->_telm0->_key);
        int cmp1 = 0;
        switch (node->_type)
        {
        case 2:
            if (cmp0 < 0)
            {
                node->_left = remove(node->_left, k);
                return removeLeft2(node);
            }
            else if (cmp0 == 0)
            {
                if (node->_left == NULL)
                {
                    return new Tree23Node<K, V>(TREE23_DELETE_ACTIVE);
                }
                Tree23Elm<K, V>* maxLeft = new Tree23Elm<K, V>();
                node->_left = removeMax(node->_left, maxLeft);
                node->_telm0 = maxLeft;
                return removeLeft2(node);
            }
            else
            {
                node->_right = remove(node->_right, k);
                return removeRight2(node);
            }
        case 3:
            cmp1 = k->compare(node->_telm1->_key);
            if (cmp0 < 0)
            {
                node->_left = remove(node->_left, k);
                return removeLeft3(node);
            }
            else if (cmp0 == 0)
            {
                if (node->_left == NULL)
                {
                    return new Tree23Node<K, V>(TREE23_BI_NODE, node->_telm1);
                }
                Tree23Elm<K, V>* maxLeft = new Tree23Elm<K, V>();
                node->_left = removeMax(node->_left, maxLeft);
                node->_telm0 = maxLeft;
                return removeLeft3(node);
            }
            else if (cmp1 < 0)
            {
                node->_midle = remove(node->_midle, k);
                return removeMidle3(node);
            }
            else if (cmp1 == 0)
            {
                if (node->_midle == NULL)
                {
                    return new Tree23Node<K, V>(TREE23_BI_NODE, node->_telm0);
                }
                Tree23Elm<K, V>* maxMidle = new Tree23Elm<K, V>();
                node->_midle = removeMax(node->_midle, maxMidle);
                node->_telm1 = maxMidle;
                return removeMidle3(node);
            }
            else
            {
                node->_right = remove(node->_right, k);
                return removeRight3(node);
            }
        default:
            break;
        }
        return NULL;
    }

    bool find(K* key)
    {
        Tree23Node<K, V>* node = _root;
        while (node != NULL)
        {
            int cmp0 = key->compare(node->_telm0->_key);
            int cmp1 = 0;
            switch (node->_type)
            {
            case 2:
                if (cmp0 < 0)
                    node = node->_left;
                else if (cmp0 == 0)
                {
                    return true;
                }
                else
                {
                    node = node->_right;
                }
                break;
            case 3:
                cmp1 = key->compare(node->_telm1->_key);
                if (cmp0 < 0)
                {
                    node = node->_left;
                }
                else if (cmp0 == 0)
                {
                    return true;
                }
                else if (cmp1 < 0)
                {
                    node = node->_midle;
                }
                else if (cmp1 == 0)
                {
                    return true;
                }
                else
                {
                    node = node->_right;
                }
                break;
            default:
                break;
            }
        }
        return false;
    }

    V* getVal(K* key)
    {
        Tree23Node<K, V>* node = _root;
        while (node != NULL)
        {
            int cmp0 = key->compare(node->_telm0->_key);
            int cmp1 = 0;
            switch (node->_type)
            {
            case 2:
                if (cmp0 < 0)
                {
                    node = node->_left;
                }
                else if (cmp0 == 0)
                {
                    return node->_telm0->_val;
                }
                else
                {
                    node = node->_right;
                }
                break;
            case 3:
                cmp1 = key->compare(node->_telm1->_key);
                if (cmp0 < 0)
                {
                    node = node->_left;
                }
                else if (cmp0 == 0)
                {
                    return node->_telm0->_val;
                }
                else if (cmp1 < 0)
                {
                    node = node->_midle;
                }
                else if (cmp1 == 0)
                {
                    return node->_telm1->_val;
                }
                else
                {
                    node = node->_right;
                }
                break;
            default:
                break;
            }
        }
        return NULL;
    }

private:
    Tree23Node<K, V>* addLeft2(Tree23Node<K, V>* node)
    {
        Tree23Node<K, V>* n = node->_left;
        if (n != NULL && n->_type == TREE23_INSERT_ACTIVE)
        {
            return new Tree23Node<K, V>(TREE23_TRI_NODE, n->_telm0,
                    node->_telm0, n->_left, n->_right, node->_right);
        }
        return node;
    }

    Tree23Node<K, V>* addLeft3(Tree23Node<K, V>* node)
    {
        Tree23Node<K, V>* n = node->_left;
        if (n != NULL && n->_type == TREE23_INSERT_ACTIVE)
        {
            n->_type = TREE23_BI_NODE;
            Tree23Node<K, V>* nn = new Tree23Node<K, V>(TREE23_BI_NODE,
                    node->_telm1, node->_midle, node->_right);
            return new Tree23Node<K, V>(TREE23_INSERT_ACTIVE, node->_telm0, n,
                    nn);
        }
        return node;
    }

    Tree23Node<K, V>* addRight2(Tree23Node<K, V>* node)
    {
        Tree23Node<K, V>* n = node->_right;
        if (n != NULL && n->_type == TREE23_INSERT_ACTIVE)
        {
            return new Tree23Node<K, V>(TREE23_TRI_NODE, node->_telm0,
                    n->_telm0, node->_left, n->_left, n->_right);
        }
        return node;
    }

    Tree23Node<K, V>* addRight3(Tree23Node<K, V>* node)
    {
        Tree23Node<K, V>* n = node->_right;
        if (n != NULL && n->_type == TREE23_INSERT_ACTIVE)
        {
            n->_type = TREE23_BI_NODE;
            Tree23Node<K, V>* nn = new Tree23Node<K, V>(TREE23_BI_NODE,
                    node->_telm0, node->_left, node->_midle);
            return new Tree23Node<K, V>(TREE23_INSERT_ACTIVE, node->_telm1, nn,
                    n);
        }
        return node;
    }

    Tree23Node<K, V>* addMidle3(Tree23Node<K, V>* node)
    {
        Tree23Node<K, V>* n = node->_midle;
        if (n != NULL && n->_type == TREE23_INSERT_ACTIVE)
        {
            n->_left = new Tree23Node<K, V>(TREE23_BI_NODE, node->_telm0,
                    node->_left, n->_left);
            n->_right = new Tree23Node<K, V>(TREE23_BI_NODE, node->_telm1,
                    n->_right, node->_right);
            return n;
        }
        return node;
    }

    Tree23Node<K, V>* removeMax(Tree23Node<K, V>* node, Tree23Elm<K, V>* elm)
    {
        if (node->_right == NULL)
        {
            switch (node->_type)
            {
            case 2:
                elm->_key = node->_telm0->_key;
                elm->_val = node->_telm0->_val;
                return new Tree23Node<K, V>(TREE23_DELETE_ACTIVE);
            case 3:
                elm->_key = node->_telm1->_key;
                elm->_val = node->_telm1->_val;
                return new Tree23Node<K, V>(TREE23_BI_NODE, node->_telm0);
            default:
                break;
            }
        }
        else
        {
            node->_right = removeMax(node->_right, elm);
            switch (node->_type)
            {
            case 2:
                return removeRight2(node);
            case 3:
                return removeRight3(node);
            default:
                break;
            }
        }
        return NULL;
    }

    Tree23Node<K, V>* removeLeft2(Tree23Node<K, V>* node)
    {
        Tree23Node<K, V>* n = node->_left;
        if (n != NULL && n->_type == TREE23_DELETE_ACTIVE)
        {
            Tree23Node<K, V>* r = node->_right;
            Tree23Node<K, V>* midle;
            Tree23Node<K, V>* left;
            Tree23Node<K, V>* right;

            switch (r->_type)
            {
            case 2:
                midle = new Tree23Node<K, V>(TREE23_TRI_NODE, node->_telm0,
                        r->_telm0, n->_midle, r->_left, r->_right);
                return new Tree23Node<K, V>(TREE23_DELETE_ACTIVE, midle);
            case 3:
                left = new Tree23Node<K, V>(TREE23_BI_NODE, node->_telm0,
                        n->_midle, r->_left);
                right = new Tree23Node<K, V>(TREE23_BI_NODE, r->_telm1,
                        r->_midle, r->_right);
                return new Tree23Node<K, V>(TREE23_BI_NODE, r->_telm0, left,
                        right);
            default:
                break;
            }
        }
        return node;
    }

    Tree23Node<K, V>* removeRight2(Tree23Node<K, V>* node)
    {
        Tree23Node<K, V>* n = node->_right;
        if (n != NULL && n->_type == TREE23_DELETE_ACTIVE)
        {
            Tree23Node<K, V>* l = node->_left;
            Tree23Node<K, V>* midle;
            Tree23Node<K, V>* left;
            Tree23Node<K, V>* right;

            switch (l->_type)
            {
            case 2:
                midle = new Tree23Node<K, V>(TREE23_TRI_NODE, l->_telm0,
                        node->_telm0, l->_left, l->_right, n->_midle);
                return new Tree23Node<K, V>(-1, midle);
            case 3:
                right = new Tree23Node<K, V>(TREE23_BI_NODE, node->_telm0,
                        l->_right, n->_midle);
                left = new Tree23Node<K, V>(TREE23_BI_NODE, l->_telm0, l->_left,
                        l->_midle);
                return new Tree23Node<K, V>(TREE23_BI_NODE, l->_telm1, left,
                        right);
            default:
                break;
            }
        }
        return node;
    }

    Tree23Node<K, V>* removeLeft3(Tree23Node<K, V>* node)
    {
        Tree23Node<K, V>* n = node->_left;
        if (n != NULL && n->_type == TREE23_DELETE_ACTIVE)
        {
            Tree23Node<K, V>* m = node->_midle;
            Tree23Node<K, V>* r = node->_right;
            Tree23Node<K, V>* left;
            Tree23Node<K, V>* midle;

            switch (m->_type)
            {
            case 2:
                left = new Tree23Node<K, V>(TREE23_TRI_NODE, node->_telm0,
                        m->_telm0, n->_midle, m->_left, m->_right);
                return new Tree23Node<K, V>(TREE23_BI_NODE, node->_telm1, left,
                        r);
            case 3:
                left = new Tree23Node<K, V>(TREE23_BI_NODE, node->_telm0,
                        n->_midle, m->_left);
                midle = new Tree23Node<K, V>(TREE23_BI_NODE, m->_telm1,
                        m->_midle, m->_right);
                return new Tree23Node<K, V>(TREE23_TRI_NODE, m->_telm0,
                        node->_telm1, left, midle, r);
            default:
                break;
            }
        }
        return node;
    }

    Tree23Node<K, V>* removeMidle3(Tree23Node<K, V>* node)
    {
        Tree23Node<K, V>* n = node->_midle;
        if (n != NULL && n->_type == TREE23_DELETE_ACTIVE)
        {
            Tree23Node<K, V>* l = node->_left;
            Tree23Node<K, V>* r = node->_right;
            Tree23Node<K, V>* midle;
            Tree23Node<K, V>* right;
            switch (r->_type)
            {
            case 2:
                right = new Tree23Node<K, V>(TREE23_TRI_NODE, node->_telm1,
                        r->_telm0, n->_midle, r->_left, r->_right);
                return new Tree23Node<K, V>(TREE23_BI_NODE, node->_telm0, l,
                        right);
            case 3:
                midle = new Tree23Node<K, V>(TREE23_BI_NODE, node->_telm1,
                        n->_midle, r->_left);
                right = new Tree23Node<K, V>(TREE23_BI_NODE, r->_telm1,
                        r->_midle, r->_right);
                return new Tree23Node<K, V>(TREE23_TRI_NODE, node->_telm0,
                        r->_telm0, l, midle, right);
            default:
                break;
            }
        }
        return node;
    }

    Tree23Node<K, V>* removeRight3(Tree23Node<K, V>* node)
    {
        Tree23Node<K, V>* n = node->_right;
        if (n != NULL && n->_type == TREE23_DELETE_ACTIVE)
        {
            Tree23Node<K, V>* l = node->_left;
            Tree23Node<K, V>* m = node->_midle;
            Tree23Node<K, V>* midle;
            Tree23Node<K, V>* right;
            switch (m->_type)
            {
            case 2:
                right = new Tree23Node<K, V>(TREE23_TRI_NODE, m->_telm0,
                        node->_telm1, m->_left, m->_right, n->_midle);
                return new Tree23Node<K, V>(TREE23_BI_NODE, node->_telm0, l,
                        right);
            case 3:
                right = new Tree23Node<K, V>(TREE23_BI_NODE, node->_telm1,
                        m->_right, n->_midle);
                midle = new Tree23Node<K, V>(TREE23_BI_NODE, m->_telm0,
                        m->_left, m->_midle);
                return new Tree23Node<K, V>(TREE23_TRI_NODE, node->_telm0,
                        m->_telm1, l, midle, right);
            default:
                break;
            }
        }
        return node;
    }

    Tree23Node<K, V>* _root;
};

/*=====================================
 Class List
 =====================================*/
template<typename T>
class ListElm
{
    template<typename U> friend class List;
public:
    ListElm()
    {
        _elm = nullptr;
        _prev = _next = nullptr;
    }
    ListElm(T* elm)
    {
        _elm = elm;
        _prev = _next = nullptr;
    }
    T* getContent(void)
    {
        return _elm;
    }
    ~ListElm()
    {
    }

private:
    ListElm<T>* getNext(void)
    {
        return _next;
    }
    T* _elm;
    ListElm<T>* _prev;
    ListElm<T>* _next;
};

template<typename T>
class List
{
public:
    List()
    {
        _head = _tail = nullptr;
        _size = 0;
    }
    ~List()
    {
        clear();
    }

    int add(T* t)
    {
        ListElm<T>* elm = new ListElm<T>(t);
        if (elm == nullptr)
        {
            return 0;
        }
        if (_head == nullptr)
        {
            _head = elm;
            _tail = elm;
        }
        else
        {
            elm->_prev = _tail;
            _tail->_next = elm;
            _tail = elm;
        }
        _size++;
        return 1;
    }

    void erase(ListElm<T>* elm)
    {
        if (_head == elm)
        {
            _head = elm->_next;
            _size--;
            delete elm;
        }
        else if (_tail == elm)
        {
            _tail = elm->_prev;
            elm->_prev->_next = nullptr;
            _size--;
            delete elm;
        }
        else
        {
            elm->_prev->_next = elm->_next;
            elm->_next->_prev = elm->_prev;
            _size--;
            delete elm;
        }
    }
    void clear(void)
    {
        ListElm<T>* p = _head;
        while (p)
        {
            ListElm<T>* q = p->_next;
            delete p;
            p = q;
        }
        _head = nullptr;
        _tail = nullptr;
        _size = 0;
    }

    ListElm<T>* getElm(void)
    {
        return _head;
    }

    ListElm<T>* getNext(ListElm<T>* elm)
    {
        return elm->getNext();
    }

    int getSize(void)
    {
        return _size;
    }

private:
    ListElm<T>* _head;
    ListElm<T>* _tail;
    int _size;
};

extern Process* theProcess;
extern MultiTaskProcess* theMultiTaskProcess;

}
#endif /* MQTTSNGWPROCESS_H_ */
