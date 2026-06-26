/*******************************************************************************
 * Copyright (c) 2026 Ian Craggs
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    https://www.eclipse.org/legal/epl-2.0/
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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

/*
 * Python 3 C extension module: mqttsn2
 *
 * Provides a Python interface to the MQTT-SN 2.0 serialization and
 * deserialization functions from:
 *   MQTTSNConnectClient.c   — CONNECT, CONNACK, DISCONNECT, PINGREQ, PINGRESP
 *   MQTTSNSubscribeClient.c — SUBSCRIBE, SUBACK
 *   MQTTSNSerializePublish.c   — PUBLISH, PUBACK, PUBREC, PUBREL, PUBCOMP
 *   MQTTSNDeserializePublish.c — same packet types
 *
 * Design:
 *   Serialize functions take Python arguments → return bytes
 *   Deserialize functions take bytes          → return dict
 *
 * Topic types (exposed as module constants):
 *   TOPIC_TYPE_SESSION    = 0   (session-scoped alias)
 *   TOPIC_TYPE_PREDEFINED = 1   (statically defined alias)
 *   TOPIC_TYPE_NAME       = 3   (full topic name string)
 *   TOPIC_TYPE_FILTER     = 3   (topic filter string; same wire value as NAME)
 *
 * For subscribe/publish, the topic argument may be:
 *   int   — used as a 2-byte alias (SESSION or PREDEFINED type)
 *   str   — encoded as UTF-8 and used as a name/filter string
 *   bytes — used as a name/filter string without encoding
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "MQTTSNPacket.h"
#include "MQTTSNConnect.h"
#include "MQTTSNSubscribe.h"
#include "MQTTSNPublish.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Maximum MQTT-SN packet size */
#define MQTTSN2_MAXPACKET 65535

/* Module-level exception class */
static PyObject* mqttsn2_error;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/*
 * Extracts a binary buffer from a Python str or bytes object.
 * If the input is str, encodes it as UTF-8 into a new bytes object which is
 * placed in *encoded_out (caller must Py_XDECREF it when done).
 * Returns 0 on success, -1 on error (Python exception already set).
 */
static int get_buffer(PyObject* obj, const char** buf_out, Py_ssize_t* len_out,
                      PyObject** encoded_out)
{
    *encoded_out = NULL;

    if (PyBytes_Check(obj))
    {
        *buf_out = PyBytes_AS_STRING(obj);
        *len_out = PyBytes_GET_SIZE(obj);
        return 0;
    }
    if (PyUnicode_Check(obj))
    {
        *encoded_out = PyUnicode_AsUTF8String(obj);
        if (!*encoded_out) return -1;
        *buf_out = PyBytes_AS_STRING(*encoded_out);
        *len_out = PyBytes_GET_SIZE(*encoded_out);
        return 0;
    }
    PyErr_SetString(PyExc_TypeError, "expected str or bytes");
    return -1;
}

/*
 * Parses a Python topic argument (int, str, or bytes) into an MQTTSN_topic.
 * topic_type must be one of MQTTSN_TOPIC_TYPE_SESSION,
 *   MQTTSN_TOPIC_TYPE_PREDEFINED, MQTTSN_TOPIC_TYPE_FILTER, or
 *   MQTTSN_TOPIC_TYPE_NAME.
 * *encoded_out is set to a new bytes object for str input and must be
 *   Py_XDECREF'd by the caller.
 * Returns 0 on success, -1 on error.
 */
static int parse_topic(PyObject* topic_obj, int topic_type,
                       MQTTSN_topic* t, PyObject** encoded_out)
{
    *encoded_out = NULL;
    t->type = (MQTTSN_topicTypes)topic_type;

    if (topic_type == MQTTSN_TOPIC_TYPE_SESSION ||
        topic_type == MQTTSN_TOPIC_TYPE_PREDEFINED)
    {
        if (!PyLong_Check(topic_obj))
        {
            PyErr_SetString(PyExc_TypeError,
                "topic must be an integer alias for SESSION/PREDEFINED types");
            return -1;
        }
        unsigned long alias = PyLong_AsUnsignedLong(topic_obj);
        if (PyErr_Occurred()) return -1;
        if (alias > 0xFFFF)
        {
            PyErr_SetString(PyExc_ValueError, "topic alias must fit in 16 bits");
            return -1;
        }
        t->alt.alias = (uint16_t)alias;
    }
    else /* FILTER or NAME */
    {
        const char* buf;
        Py_ssize_t  len;
        if (get_buffer(topic_obj, &buf, &len, encoded_out) < 0) return -1;
        t->alt.string.data  = (char*)buf;
        t->alt.string.len   = (uint16_t)len;
        t->alt.string.islen8 = false;
    }
    return 0;
}

/*
 * Serializes packet data using the supplied function and returns a Python bytes
 * object.  The function is expected to write into buf[0..MQTTSN2_MAXPACKET-1]
 * and return the number of bytes written, or <= 0 on error.
 */
static PyObject* make_bytes(int32_t wire_len, uint8_t* buf)
{
    if (wire_len <= 0)
    {
        PyErr_SetString(mqttsn2_error, "serialization failed");
        return NULL;
    }
    return PyBytes_FromStringAndSize((char*)buf, (Py_ssize_t)wire_len);
}


/* =========================================================================
 * CONNECT / CONNACK / DISCONNECT / PINGREQ / PINGRESP
 * ========================================================================= */

PyDoc_STRVAR(serialize_connect_doc,
"serialize_connect(client_id, *, keep_alive=60, max_packet_size=65535,\n"
"                  clean_start=False, packet_id=0,\n"
"                  session_expiry_interval=None, default_awake_messages=None,\n"
"                  will=None, auth=None) -> bytes\n"
"\n"
"Serialize a MQTT-SN 2.0 CONNECT packet.\n"
"\n"
"client_id               str or bytes (may be empty)\n"
"will                    dict with keys:\n"
"  topic_type            int (TOPIC_TYPE_SESSION / PREDEFINED / NAME)\n"
"  topic                 int (alias) or str/bytes (name)\n"
"  qos                   int 0/1/2\n"
"  retain                bool\n"
"  payload               bytes\n"
"auth                    dict with keys:\n"
"  method                str or bytes\n"
"  data                  bytes\n");

static PyObject* py_serialize_connect(PyObject* self, PyObject* args,
                                      PyObject* kwargs)
{
    static char* kwlist[] = {
        "client_id",
        "keep_alive", "max_packet_size", "clean_start", "packet_id",
        "session_expiry_interval", "default_awake_messages",
        "will", "auth",
        NULL
    };

    PyObject *client_id_obj = NULL;
    int       keep_alive    = 60;
    int       max_pkt_size  = 65535;
    int       clean_start   = 0;
    int       packet_id     = 0;
    PyObject *sei_obj       = Py_None; /* session_expiry_interval */
    PyObject *dam_obj       = Py_None; /* default_awake_messages */
    PyObject *will_obj      = Py_None;
    PyObject *auth_obj      = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
            "O|iiiiOOOO", kwlist,
            &client_id_obj,
            &keep_alive, &max_pkt_size, &clean_start, &packet_id,
            &sei_obj, &dam_obj,
            &will_obj, &auth_obj))
        return NULL;

    /* --- Owned temporaries that must be freed before any return --- */
    PyObject *cid_enc   = NULL;
    PyObject *will_enc  = NULL; /* will topic name, if str */
    PyObject *auth_menc = NULL; /* auth method, if str */

    MQTTSNPacket_connectData options = MQTTSNPacket_connectData_initializer;
    uint8_t buf[MQTTSN2_MAXPACKET];
    PyObject* result = NULL;

    /* client_id */
    {
        const char* buf_ptr; Py_ssize_t buf_len;
        if (get_buffer(client_id_obj, &buf_ptr, &buf_len, &cid_enc) < 0)
            goto done;
        options.clientID.data = (char*)buf_ptr;
        options.clientID.len  = (uint16_t)buf_len;
    }

    options.keepAlive   = (uint16_t)keep_alive;
    options.maxPacketSize = (uint16_t)max_pkt_size;
    options.packetId      = (uint16_t)packet_id;
    options.flags.bits.cleanStart = (clean_start != 0);

    /* optional: sessionExpiryInterval */
    if (sei_obj != Py_None)
    {
        options.flags.bits.sessionExpiryInterval = 1;
        options.sessionExpiryInterval = (uint32_t)PyLong_AsUnsignedLong(sei_obj);
        if (PyErr_Occurred()) goto done;
    }

    /* optional: defaultAwakeMessages */
    if (dam_obj != Py_None)
    {
        options.flags.bits.defaultAwakeMessages = 1;
        options.defaultAwakeMessages = (uint8_t)PyLong_AsLong(dam_obj);
        if (PyErr_Occurred()) goto done;
    }

    /* optional: will */
    if (will_obj != Py_None)
    {
        if (!PyDict_Check(will_obj))
        {
            PyErr_SetString(PyExc_TypeError, "'will' must be a dict");
            goto done;
        }

        PyObject* tt = PyDict_GetItemString(will_obj, "topic_type");
        PyObject* tp = PyDict_GetItemString(will_obj, "topic");
        PyObject* qo = PyDict_GetItemString(will_obj, "qos");
        PyObject* rt = PyDict_GetItemString(will_obj, "retain");
        PyObject* pl = PyDict_GetItemString(will_obj, "payload");

        if (!tt || !tp)
        {
            PyErr_SetString(PyExc_KeyError,
                "will dict requires 'topic_type' and 'topic'");
            goto done;
        }

        int wtt = (int)PyLong_AsLong(tt);
        if (PyErr_Occurred()) goto done;

        MQTTSN_topic will_topic;
        if (parse_topic(tp, wtt, &will_topic, &will_enc) < 0) goto done;

        options.will.topic = will_topic;
        options.willFlags.bits.topicType = (uint8_t)wtt;
        options.willFlags.bits.QoS       = qo ? (uint8_t)PyLong_AsLong(qo) : 0;
        options.willFlags.bits.retain    = rt ? (PyObject_IsTrue(rt) != 0) : false;
        if (PyErr_Occurred()) goto done;

        if (pl && pl != Py_None)
        {
            if (!PyBytes_Check(pl))
            {
                PyErr_SetString(PyExc_TypeError, "will 'payload' must be bytes");
                goto done;
            }
            options.will.payload.data = (unsigned char*)PyBytes_AS_STRING(pl);
            options.will.payload.len  = (uint16_t)PyBytes_GET_SIZE(pl);
        }

        options.flags.bits.will = 1;
    }

    /* optional: auth */
    if (auth_obj != Py_None)
    {
        if (!PyDict_Check(auth_obj))
        {
            PyErr_SetString(PyExc_TypeError, "'auth' must be a dict");
            goto done;
        }

        PyObject* mobj = PyDict_GetItemString(auth_obj, "method");
        PyObject* dobj = PyDict_GetItemString(auth_obj, "data");

        if (!mobj)
        {
            PyErr_SetString(PyExc_KeyError, "auth dict requires 'method'");
            goto done;
        }

        const char* mptr; Py_ssize_t mlen;
        if (get_buffer(mobj, &mptr, &mlen, &auth_menc) < 0) goto done;
        options.auth.method.data   = (char*)mptr;
        options.auth.method.len    = (uint16_t)mlen;
        options.auth.method.islen8 = true;

        if (dobj && dobj != Py_None)
        {
            if (!PyBytes_Check(dobj))
            {
                PyErr_SetString(PyExc_TypeError, "auth 'data' must be bytes");
                goto done;
            }
            options.auth.data.data = (unsigned char*)PyBytes_AS_STRING(dobj);
            options.auth.data.len  = (uint16_t)PyBytes_GET_SIZE(dobj);
        }

        options.flags.bits.auth = 1;
    }

    result = make_bytes(
        MQTTSNSerialize_connect(buf, (int32_t)sizeof(buf), &options),
        buf);

done:
    Py_XDECREF(cid_enc);
    Py_XDECREF(will_enc);
    Py_XDECREF(auth_menc);
    return result;
}


PyDoc_STRVAR(deserialize_connack_doc,
"deserialize_connack(buffer) -> dict\n"
"\n"
"Deserialize a MQTT-SN 2.0 CONNACK packet.\n"
"\n"
"Returns a dict with keys:\n"
"  packet_id, reason_code, session_present,\n"
"  session_expiry_interval (int or None),\n"
"  server_keep_alive (int or None),\n"
"  auth_method (bytes or None), auth_data (bytes or None),\n"
"  assigned_client_id (bytes or None)");

static PyObject* py_deserialize_connack(PyObject* self, PyObject* args)
{
    Py_buffer view;
    if (!PyArg_ParseTuple(args, "y*", &view)) return NULL;

    MQTTSNPacket_connackData data;
    memset(&data, 0, sizeof(data));

    int32_t rc = MQTTSNDeserialize_connack(&data,
                     (uint8_t*)view.buf, (int32_t)view.len);
    PyBuffer_Release(&view);

    if (rc != 1)
    {
        PyErr_SetString(mqttsn2_error, "CONNACK deserialization failed");
        return NULL;
    }

    PyObject* d = PyDict_New();
    if (!d) return NULL;

#define SET_INT(k, v)  PyDict_SetItemString(d, k, PyLong_FromLong(v))
#define SET_BOOL(k, v) PyDict_SetItemString(d, k, PyBool_FromLong(v))
#define SET_NONE(k)    PyDict_SetItemString(d, k, Py_None)
#define SET_BYTES(k, ptr, len) \
    PyDict_SetItemString(d, k, PyBytes_FromStringAndSize((char*)(ptr), (len)))

    SET_INT ("packet_id",        data.packetId);
    SET_INT ("reason_code",      data.reasonCode);
    SET_BOOL("session_present",  data.flags.bits.sessionPresent);

    if (data.flags.bits.sessionExpiryInterval)
        SET_INT("session_expiry_interval", (long)data.sessionExpiryInterval);
    else
        SET_NONE("session_expiry_interval");

    if (data.flags.bits.serverKeepAlive)
        SET_INT("server_keep_alive", data.serverKeepAlive);
    else
        SET_NONE("server_keep_alive");

    if (data.flags.bits.auth && data.auth.method.len > 0)
        SET_BYTES("auth_method", data.auth.method.data, data.auth.method.len);
    else
        SET_NONE("auth_method");

    if (data.flags.bits.auth && data.auth.data.len > 0)
        SET_BYTES("auth_data", data.auth.data.data, data.auth.data.len);
    else
        SET_NONE("auth_data");

    if (data.assignedClientID.len > 0)
        SET_BYTES("assigned_client_id",
                  data.assignedClientID.data, data.assignedClientID.len);
    else
        SET_NONE("assigned_client_id");

#undef SET_INT
#undef SET_BOOL
#undef SET_NONE
#undef SET_BYTES
    return d;
}


PyDoc_STRVAR(serialize_disconnect_doc,
"serialize_disconnect(reason_code=0) -> bytes\n"
"\n"
"Serialize a MQTT-SN 2.0 DISCONNECT packet.\n"
"reason_code  0x00 = normal disconnection (reason code omitted from wire)");

static PyObject* py_serialize_disconnect(PyObject* self, PyObject* args)
{
    int reason_code = 0;
    if (!PyArg_ParseTuple(args, "|i", &reason_code)) return NULL;

    uint8_t buf[8];
    return make_bytes(
        MQTTSNSerialize_disconnect(buf, (int32_t)sizeof(buf),
                                   (uint8_t)reason_code),
        buf);
}


PyDoc_STRVAR(serialize_pingreq_doc,
"serialize_pingreq(packet_id) -> bytes\n"
"\n"
"Serialize a MQTT-SN 2.0 PINGREQ packet.\n"
"\n"
"packet_id  int (0-65535): used to match the corresponding PINGRESP.\n"
"           The caller should use a fresh value for each PINGREQ.\n"
"\n"
"Wire format: length(1) + type(1) + packet_id(2) = always 4 bytes.\n"
"The v1.2 Client Identifier field is gone in v2.0 (Section 3.11.2).");

static PyObject* py_serialize_pingreq(PyObject* self, PyObject* args)
{
    int packet_id;
    if (!PyArg_ParseTuple(args, "i", &packet_id)) return NULL;

    if (packet_id < 0 || packet_id > 0xFFFF)
    {
        PyErr_SetString(PyExc_ValueError, "packet_id must be 0-65535");
        return NULL;
    }

    uint8_t buf[8];   /* fixed 4-byte packet; small stack buffer is fine */
    return make_bytes(
        MQTTSNSerialize_pingreq(buf, (int32_t)sizeof(buf), (uint16_t)packet_id),
        buf);
}


PyDoc_STRVAR(deserialize_pingresp_doc,
"deserialize_pingresp(buffer) -> dict\n"
"\n"
"Deserialize a MQTT-SN 2.0 PINGRESP packet.\n"
"\n"
"Returns a dict with keys:\n"
"  packet_id           int  — must equal the PINGREQ packet_id\n"
"  messages_remaining  int  — Application Messages Remaining (0-255),\n"
"                             or -1 if the field was absent from the wire\n"
"\n"
"Raises mqttsn2.MQTTSNError if the buffer is not a valid PINGRESP.");

static PyObject* py_deserialize_pingresp(PyObject* self, PyObject* args)
{
    Py_buffer view;
    if (!PyArg_ParseTuple(args, "y*", &view)) return NULL;

    uint16_t packetid          = 0;
    int      messages_remaining = 0;

    int32_t rc = MQTTSNDeserialize_pingresp(
                     &packetid, &messages_remaining,
                     (uint8_t*)view.buf, (int32_t)view.len);
    PyBuffer_Release(&view);

    if (rc != 1)
    {
        PyErr_SetString(mqttsn2_error, "PINGRESP deserialization failed");
        return NULL;
    }

    return Py_BuildValue("{s:i, s:i}",
        "packet_id",          (int)packetid,
        "messages_remaining", messages_remaining);
}


/* =========================================================================
 * SUBSCRIBE / SUBACK
 * ========================================================================= */

PyDoc_STRVAR(serialize_subscribe_doc,
"serialize_subscribe(qos, retain_handling, rap, no_local, packet_id,\n"
"                    topic_type, topic) -> bytes\n"
"\n"
"Serialize a MQTT-SN 2.0 SUBSCRIBE packet.\n"
"\n"
"topic_type  TOPIC_TYPE_SESSION / PREDEFINED: topic must be an int alias\n"
"            TOPIC_TYPE_FILTER / NAME:        topic must be str or bytes");

static PyObject* py_serialize_subscribe(PyObject* self, PyObject* args)
{
    int       qos, retain_handling, rap, no_local, packet_id, topic_type;
    PyObject* topic_obj;

    if (!PyArg_ParseTuple(args, "iiiiiiO",
            &qos, &retain_handling, &rap, &no_local,
            &packet_id, &topic_type, &topic_obj))
        return NULL;

    PyObject*    enc = NULL;
    MQTTSN_topic topic;
    if (parse_topic(topic_obj, topic_type, &topic, &enc) < 0) return NULL;

    uint8_t buf[MQTTSN2_MAXPACKET];
    PyObject* result = make_bytes(
        MQTTSNSerialize_subscribe(buf, (int32_t)sizeof(buf),
            qos, (uint8_t)retain_handling, (uint8_t)rap, (uint8_t)no_local,
            (uint16_t)packet_id, &topic),
        buf);
    Py_XDECREF(enc);
    return result;
}


PyDoc_STRVAR(deserialize_suback_doc,
"deserialize_suback(buffer) -> dict\n"
"\n"
"Deserialize a MQTT-SN 2.0 SUBACK packet.\n"
"\n"
"Returns a dict with keys:\n"
"  packet_id, reason_code, topic_alias_present (bool), topic_alias");

static PyObject* py_deserialize_suback(PyObject* self, PyObject* args)
{
    Py_buffer view;
    if (!PyArg_ParseTuple(args, "y*", &view)) return NULL;

    uint16_t topic_alias         = 0;
    uint8_t  topic_alias_present = 0;
    uint16_t packet_id           = 0;
    uint8_t  reason_code         = 0;

    int32_t rc = MQTTSNDeserialize_suback(&topic_alias, &topic_alias_present,
                     &packet_id, &reason_code,
                     (uint8_t*)view.buf, (int32_t)view.len);
    PyBuffer_Release(&view);

    if (rc != 1)
    {
        PyErr_SetString(mqttsn2_error, "SUBACK deserialization failed");
        return NULL;
    }

    return Py_BuildValue("{s:i, s:i, s:N, s:i}",
        "packet_id",           (int)packet_id,
        "reason_code",         (int)reason_code,
        "topic_alias_present", PyBool_FromLong(topic_alias_present),
        "topic_alias",         (int)topic_alias);
}


/* =========================================================================
 * PUBLISH / PUBACK / PUBREC / PUBREL / PUBCOMP
 * ========================================================================= */

PyDoc_STRVAR(serialize_publish_doc,
"serialize_publish(dup, qos, retained, packet_id, topic_type, topic,\n"
"                  payload) -> bytes\n"
"\n"
"Serialize a MQTT-SN 2.0 PUBLISH packet.\n"
"\n"
"topic_type  TOPIC_TYPE_SESSION / PREDEFINED: topic must be an int alias\n"
"            TOPIC_TYPE_NAME:                 topic must be str or bytes\n"
"payload     bytes");

static PyObject* py_serialize_publish(PyObject* self, PyObject* args)
{
    int       dup, qos, retained, packet_id, topic_type;
    PyObject* topic_obj;
    PyObject* payload_obj;

    if (!PyArg_ParseTuple(args, "iiiiiOO",
            &dup, &qos, &retained, &packet_id,
            &topic_type, &topic_obj, &payload_obj))
        return NULL;

    if (!PyBytes_Check(payload_obj))
    {
        PyErr_SetString(PyExc_TypeError, "payload must be bytes");
        return NULL;
    }

    PyObject*    enc = NULL;
    MQTTSN_topic topic;
    if (parse_topic(topic_obj, topic_type, &topic, &enc) < 0) return NULL;

    uint8_t buf[MQTTSN2_MAXPACKET];
    PyObject* result = make_bytes(
        MQTTSNSerialize_publish(buf, (int32_t)sizeof(buf),
            (uint8_t)dup, qos, (uint8_t)retained, (uint16_t)packet_id,
            &topic,
            (uint8_t*)PyBytes_AS_STRING(payload_obj),
            (int32_t)PyBytes_GET_SIZE(payload_obj)),
        buf);
    Py_XDECREF(enc);
    return result;
}


PyDoc_STRVAR(deserialize_publish_doc,
"deserialize_publish(buffer) -> dict\n"
"\n"
"Deserialize a MQTT-SN 2.0 PUBLISH packet.\n"
"\n"
"Returns a dict with keys:\n"
"  dup (bool), qos, retained (bool), packet_id,\n"
"  topic_type, topic_alias (int, for alias types) or\n"
"  topic_name (bytes, for NAME type), payload (bytes)");

static PyObject* py_deserialize_publish(PyObject* self, PyObject* args)
{
    Py_buffer view;
    if (!PyArg_ParseTuple(args, "y*", &view)) return NULL;

    uint8_t      dup = 0, retained = 0;
    int32_t      qos = 0;
    uint16_t     packet_id = 0;
    MQTTSN_topic topic;
    uint8_t*     payload    = NULL;
    int32_t      payloadlen = 0;

    memset(&topic, 0, sizeof(topic));

    int32_t rc = MQTTSNDeserialize_publish(&dup, &qos, &retained, &packet_id,
                     &topic, &payload, &payloadlen,
                     (uint8_t*)view.buf, (int32_t)view.len);
    PyBuffer_Release(&view);

    if (rc != 1)
    {
        PyErr_SetString(mqttsn2_error, "PUBLISH deserialization failed");
        return NULL;
    }

    PyObject* d = PyDict_New();
    if (!d) return NULL;

    PyDict_SetItemString(d, "dup",      PyBool_FromLong(dup));
    PyDict_SetItemString(d, "qos",      PyLong_FromLong(qos));
    PyDict_SetItemString(d, "retained", PyBool_FromLong(retained));
    PyDict_SetItemString(d, "packet_id",PyLong_FromLong(packet_id));
    PyDict_SetItemString(d, "topic_type",PyLong_FromLong((long)topic.type));

    if (topic.type == MQTTSN_TOPIC_TYPE_NAME)
        PyDict_SetItemString(d, "topic_name",
            PyBytes_FromStringAndSize(topic.alt.string.data,
                                      topic.alt.string.len));
    else
        PyDict_SetItemString(d, "topic_alias",
            PyLong_FromLong(topic.alt.alias));

    PyDict_SetItemString(d, "payload",
        PyBytes_FromStringAndSize((char*)payload, (Py_ssize_t)payloadlen));

    return d;
}


PyDoc_STRVAR(serialize_puback_doc,
"serialize_puback(packet_id, return_code=0) -> bytes\n\n"
"Serialize a MQTT-SN 2.0 PUBACK packet.");

static PyObject* py_serialize_puback(PyObject* self, PyObject* args)
{
    int packet_id, return_code = 0;
    if (!PyArg_ParseTuple(args, "i|i", &packet_id, &return_code)) return NULL;
    uint8_t buf[8];
    return make_bytes(
        MQTTSNSerialize_puback(buf, (int32_t)sizeof(buf),
                               (uint16_t)packet_id, (uint8_t)return_code),
        buf);
}


PyDoc_STRVAR(deserialize_puback_doc,
"deserialize_puback(buffer) -> dict\n\n"
"Deserialize a MQTT-SN 2.0 PUBACK packet.\n"
"Returns a dict with keys: packet_id, return_code");

static PyObject* py_deserialize_puback(PyObject* self, PyObject* args)
{
    Py_buffer view;
    if (!PyArg_ParseTuple(args, "y*", &view)) return NULL;

    uint16_t packet_id   = 0;
    uint8_t  return_code = 0;

    int32_t rc = MQTTSNDeserialize_puback(&packet_id, &return_code,
                     (uint8_t*)view.buf, (int32_t)view.len);
    PyBuffer_Release(&view);

    if (rc != 1)
    {
        PyErr_SetString(mqttsn2_error, "PUBACK deserialization failed");
        return NULL;
    }
    return Py_BuildValue("{s:i, s:i}",
        "packet_id",   (int)packet_id,
        "return_code", (int)return_code);
}


PyDoc_STRVAR(serialize_pubrec_doc,
"serialize_pubrec(packet_id, return_code=0) -> bytes\n\n"
"Serialize a MQTT-SN 2.0 PUBREC packet.");

static PyObject* py_serialize_pubrec(PyObject* self, PyObject* args)
{
    int packet_id, return_code = 0;
    if (!PyArg_ParseTuple(args, "i|i", &packet_id, &return_code)) return NULL;
    uint8_t buf[8];
    return make_bytes(
        MQTTSNSerialize_pubrec(buf, (int32_t)sizeof(buf),
                               (uint16_t)packet_id, (uint8_t)return_code),
        buf);
}


PyDoc_STRVAR(serialize_pubrel_doc,
"serialize_pubrel(packet_id, return_code=0) -> bytes\n\n"
"Serialize a MQTT-SN 2.0 PUBREL packet.");

static PyObject* py_serialize_pubrel(PyObject* self, PyObject* args)
{
    int packet_id, return_code = 0;
    if (!PyArg_ParseTuple(args, "i|i", &packet_id, &return_code)) return NULL;
    uint8_t buf[8];
    return make_bytes(
        MQTTSNSerialize_pubrel(buf, (int32_t)sizeof(buf),
                               (uint16_t)packet_id, (uint8_t)return_code),
        buf);
}


PyDoc_STRVAR(serialize_pubcomp_doc,
"serialize_pubcomp(packet_id, return_code=0) -> bytes\n\n"
"Serialize a MQTT-SN 2.0 PUBCOMP packet.");

static PyObject* py_serialize_pubcomp(PyObject* self, PyObject* args)
{
    int packet_id, return_code = 0;
    if (!PyArg_ParseTuple(args, "i|i", &packet_id, &return_code)) return NULL;
    uint8_t buf[8];
    return make_bytes(
        MQTTSNSerialize_pubcomp(buf, (int32_t)sizeof(buf),
                                (uint16_t)packet_id, (uint8_t)return_code),
        buf);
}


PyDoc_STRVAR(deserialize_ack_doc,
"deserialize_ack(buffer) -> dict\n\n"
"Deserialize a MQTT-SN 2.0 PUBREC, PUBREL, or PUBCOMP packet.\n"
"Returns a dict with keys: packet_type, packet_id, return_code");

static PyObject* py_deserialize_ack(PyObject* self, PyObject* args)
{
    Py_buffer view;
    if (!PyArg_ParseTuple(args, "y*", &view)) return NULL;

    uint8_t  packet_type = 0;
    uint16_t packet_id   = 0;
    uint8_t  return_code = 0;

    int32_t rc = MQTTSNDeserialize_ack(&packet_type, &packet_id, &return_code,
                     (uint8_t*)view.buf, (int32_t)view.len);
    PyBuffer_Release(&view);

    if (rc != 1)
    {
        PyErr_SetString(mqttsn2_error, "ACK deserialization failed");
        return NULL;
    }
    return Py_BuildValue("{s:i, s:i, s:i}",
        "packet_type", (int)packet_type,
        "packet_id",   (int)packet_id,
        "return_code", (int)return_code);
}


/* =========================================================================
 * Method table
 * ========================================================================= */

static PyMethodDef mqttsn2_methods[] = {
    /* connect */
    {"serialize_connect",   (PyCFunction)py_serialize_connect,
     METH_VARARGS | METH_KEYWORDS, serialize_connect_doc},
    {"deserialize_connack", py_deserialize_connack,
     METH_VARARGS, deserialize_connack_doc},
    {"serialize_disconnect",py_serialize_disconnect,
     METH_VARARGS, serialize_disconnect_doc},
    {"serialize_pingreq",   py_serialize_pingreq,
     METH_VARARGS, serialize_pingreq_doc},
    {"deserialize_pingresp",py_deserialize_pingresp,
     METH_VARARGS, deserialize_pingresp_doc},

    /* subscribe */
    {"serialize_subscribe",  py_serialize_subscribe,
     METH_VARARGS, serialize_subscribe_doc},
    {"deserialize_suback",   py_deserialize_suback,
     METH_VARARGS, deserialize_suback_doc},

    /* publish */
    {"serialize_publish",    py_serialize_publish,
     METH_VARARGS, serialize_publish_doc},
    {"deserialize_publish",  py_deserialize_publish,
     METH_VARARGS, deserialize_publish_doc},
    {"serialize_puback",     py_serialize_puback,
     METH_VARARGS, serialize_puback_doc},
    {"deserialize_puback",   py_deserialize_puback,
     METH_VARARGS, deserialize_puback_doc},
    {"serialize_pubrec",     py_serialize_pubrec,
     METH_VARARGS, serialize_pubrec_doc},
    {"serialize_pubrel",     py_serialize_pubrel,
     METH_VARARGS, serialize_pubrel_doc},
    {"serialize_pubcomp",    py_serialize_pubcomp,
     METH_VARARGS, serialize_pubcomp_doc},
    {"deserialize_ack",      py_deserialize_ack,
     METH_VARARGS, deserialize_ack_doc},

    {NULL, NULL, 0, NULL}
};


/* =========================================================================
 * Module definition
 * ========================================================================= */

PyDoc_STRVAR(module_doc,
"mqttsn2 — Python interface to MQTT-SN 2.0 packet serialization.\n"
"\n"
"Topic type constants\n"
"--------------------\n"
"TOPIC_TYPE_SESSION    = 0  session-scoped topic alias\n"
"TOPIC_TYPE_PREDEFINED = 1  statically known topic alias\n"
"TOPIC_TYPE_NAME       = 3  full topic name string\n"
"TOPIC_TYPE_FILTER     = 3  topic filter string (same wire value as NAME)\n"
"\n"
"Functions\n"
"---------\n"
"Serialize functions take Python arguments and return bytes.\n"
"Deserialize functions take bytes and return a dict.\n"
"\n"
"  serialize_connect / deserialize_connack\n"
"  serialize_disconnect\n"
"  serialize_pingreq / deserialize_pingresp\n"
"  serialize_subscribe / deserialize_suback\n"
"  serialize_publish / deserialize_publish\n"
"  serialize_puback / deserialize_puback\n"
"  serialize_pubrec / serialize_pubrel / serialize_pubcomp\n"
"  deserialize_ack\n"
"\n"
"Exception\n"
"---------\n"
"MQTTSNError  raised when a packet cannot be serialized or deserialized.");

static struct PyModuleDef mqttsn2module = {
    PyModuleDef_HEAD_INIT,
    "mqttsn2",
    module_doc,
    -1,
    mqttsn2_methods
};

PyMODINIT_FUNC PyInit_mqttsn2(void)
{
    PyObject* m = PyModule_Create(&mqttsn2module);
    if (!m) return NULL;

    /* Exception class */
    mqttsn2_error = PyErr_NewException("mqttsn2.MQTTSNError", NULL, NULL);
    Py_INCREF(mqttsn2_error);
    PyModule_AddObject(m, "MQTTSNError", mqttsn2_error);

    /* Topic type constants */
    PyModule_AddIntConstant(m, "TOPIC_TYPE_SESSION",    MQTTSN_TOPIC_TYPE_SESSION);
    PyModule_AddIntConstant(m, "TOPIC_TYPE_PREDEFINED", MQTTSN_TOPIC_TYPE_PREDEFINED);
    PyModule_AddIntConstant(m, "TOPIC_TYPE_NAME",       MQTTSN_TOPIC_TYPE_NAME);
    PyModule_AddIntConstant(m, "TOPIC_TYPE_FILTER",     MQTTSN_TOPIC_TYPE_FILTER);

    /* Reason code constants (most commonly used subset) */
    PyModule_AddIntConstant(m, "RC_SUCCESS",              MQTT_SN_RC_SUCCESS);
    PyModule_AddIntConstant(m, "RC_GRANTED_QOS_0",        MQTT_SN_RC_GRANTED_QOS_0);
    PyModule_AddIntConstant(m, "RC_GRANTED_QOS_1",        MQTT_SN_RC_GRANTED_QOS_1);
    PyModule_AddIntConstant(m, "RC_GRANTED_QOS_2",        MQTT_SN_RC_GRANTED_QOS_2);
    PyModule_AddIntConstant(m, "RC_NO_MATCHING_SUBSCRIBERS",
                               MQTT_SN_RC_NO_MATCHING_SUBSCRIBERS);
    PyModule_AddIntConstant(m, "RC_UNSPECIFIED_ERROR",    MQTT_SN_RC_UNSPECIFIED_ERROR);
    PyModule_AddIntConstant(m, "RC_NOT_AUTHORIZED",       MQTT_SN_RC_NOT_AUTHORIZED);
    PyModule_AddIntConstant(m, "RC_TOPIC_FILTER_INVALID", MQTT_SN_RC_TOPIC_FILTER_INVALID);
    PyModule_AddIntConstant(m, "RC_PACKET_ID_IN_USE",     MQTT_SN_RC_PACKET_ID_IN_USE);
    PyModule_AddIntConstant(m, "RC_CONGESTION",           MQTT_SN_RC_CONGESTION);
    PyModule_AddIntConstant(m, "RC_UNKNOWN_TOPIC_ALIAS",  MQTT_SN_RC_UNKNOWN_TOPIC_ALIAS);

    return m;
}
