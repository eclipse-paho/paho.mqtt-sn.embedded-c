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
#ifndef MQTTSNGATEWAY_SRC_TESTS_TESTTREE23_H_
#define MQTTSNGATEWAY_SRC_TESTS_TESTTREE23_H_

#include "MQTTSNGWProcess.h"

namespace MQTTSNGW
{

class Integer
{
public:
	int _val;
	Integer(){_val = 0;}
	Integer(int val){_val = val;}
};

class Key
{
public:
	string _key;
	Key(){};
	Key(string key){_key = key;}
	int compare(Key* obj){
		if ( _key == obj->_key )
		{
			return 0;
		}
		else if ( _key < obj->_key )
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
};



class TestTree23
{
public:
	TestTree23();
	~TestTree23();
	void add(Key* key, Integer* val){_tree23.add(key, val);}
	Tree23Node<Key, Integer>* add(Tree23Node<Key, Integer>* n, Tree23Elm<Key, Integer>* elm){return _tree23.add(n, elm);}
	void remove(Key* k){_tree23.remove(k);}
	Tree23Node<Key, Integer>* remove(Tree23Node<Key, Integer>* node, Key* k){return _tree23.remove(node, k);}
	bool find(Key* key){return _tree23.find(key);}
	Integer* getVal(Key* key){return _tree23.getVal(key);}
	void test(void);
private:
	Tree23<Key, Integer> _tree23;
};

}

#endif /* MQTTSNGATEWAY_SRC_TESTS_TESTTREE23_H_ */
