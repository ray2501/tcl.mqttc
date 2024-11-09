/*
 * For C++ compilers, use extern "C"
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <tcl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"

/*
 * Only the _Init function is exported.
 */

extern DLLEXPORT int	Mqttc_Init(Tcl_Interp * interp);

/*
 * end block for C++
 */

#ifdef __cplusplus
}
#endif


/*
 * This struct is to record MonetDB database info,
 */
struct MQTTCDATA {
    MQTTClient   client;
    int          version;
    Tcl_Interp   *interp;
    char         *clientId;
    int          timeout;
};

typedef struct MQTTCDATA MQTTCDATA;


static void DbDeleteCmd(void *db) {
  MQTTCDATA *pDb = (MQTTCDATA *)db;

  if(pDb) {
      if(pDb->version == MQTTVERSION_5) {
          MQTTClient_disconnect5(pDb->client, pDb->timeout, MQTTREASONCODE_SUCCESS, NULL);
      } else {
          MQTTClient_disconnect(pDb->client, pDb->timeout);
      }

      MQTTClient_destroy(&(pDb->client));

      Tcl_Free((char*)pDb);
  }

  pDb = 0;
}

static int MgttObjCmd(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
  MQTTCDATA *pMqtt = (MQTTCDATA *) cd;
  int choice;
  int rc = TCL_OK;

  static const char *MQTT_strs[] = {
    "isConnected",
    "publishMessage",
    "subscribe",
    "unsubscribe",
    "receive",
    "close",
    0
  };

  enum MQTT_enum {
    MQTT_ISCONNECTED,
    MQTT_PUBLISHMESSAGE,
    MQTT_SUBSCRIBE,
    MQTT_UNSUBSCRIBE,
    MQTT_RECEIVE,
    MQTT_CLOSE,
  };

  if( objc < 2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "SUBCOMMAND ...");
    return TCL_ERROR;
  }

  if( Tcl_GetIndexFromObj(interp, objv[1], MQTT_strs, "option", 0, &choice) ){
    return TCL_ERROR;
  }

  switch( (enum MQTT_enum)choice ){

    case MQTT_ISCONNECTED: {
      int rc;

      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 2, objv, 0);
        return TCL_ERROR;
      }

      rc = MQTTClient_isConnected(pMqtt->client);
      if(rc == 0) {
          Tcl_SetObjResult(interp, Tcl_NewBooleanObj(0));
      } else {
          Tcl_SetObjResult(interp, Tcl_NewBooleanObj(1));
      }

      break;
    }

    case MQTT_PUBLISHMESSAGE: {
      char *topic = NULL;
      char *payload = NULL;
      int qos = 1;
      int retained = 0;
      MQTTClient_message pubmsg = MQTTClient_message_initializer;
      MQTTClient_deliveryToken token;
      int rc;

      if( objc != 6 ){
        Tcl_WrongNumArgs(interp, 2, objv,
          "topic payload QoS retained "
        );

        return TCL_ERROR;
      }

      topic = Tcl_GetStringFromObj(objv[2], 0);
      payload= Tcl_GetStringFromObj(objv[3], 0);

      if(Tcl_GetIntFromObj(interp, objv[4], &qos) != TCL_OK) {
          return TCL_ERROR;
      }

      if(qos < 0 || qos > 2) {
          Tcl_AppendResult(interp, "qos must be 0, 1 or 2", (char*)0);
          return TCL_ERROR;
      }

      if(Tcl_GetBooleanFromObj(interp, objv[5], &retained) != TCL_OK) {
          return TCL_ERROR;
      }

      pubmsg.payload = payload;
      pubmsg.payloadlen = strlen(payload);
      pubmsg.qos = qos;
      pubmsg.retained = retained;

      if(pMqtt->version == MQTTVERSION_5) {
          MQTTResponse response = MQTTResponse_initializer;
          response = MQTTClient_publishMessage5(pMqtt->client, topic, &pubmsg, &token);
          rc = response.reasonCode;
          MQTTResponse_free(response);
      } else {
          MQTTClient_publishMessage(pMqtt->client, topic, &pubmsg, &token);
      }
      rc = MQTTClient_waitForCompletion(pMqtt->client, token, pMqtt->timeout);
      if(rc == MQTTCLIENT_SUCCESS) {
          // Return the token value
          Tcl_SetObjResult(interp, Tcl_NewIntObj(token));
      } else {
          Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
      }

      break;
    }

    case MQTT_SUBSCRIBE: {
      char *topic = NULL;
      int qos = 1;
      int rc;

      if( objc !=4) {
        Tcl_WrongNumArgs(interp, 2, objv, "topic QoS ");

        return TCL_ERROR;
      }

      topic = Tcl_GetStringFromObj(objv[2], 0);
      if(Tcl_GetIntFromObj(interp, objv[3], &qos) != TCL_OK) {
          return TCL_ERROR;
      }

      if(qos < 0 || qos > 2) {
          Tcl_AppendResult(interp, "qos must be 0, 1 or 2", (char*)0);
          return TCL_ERROR;
      }

      if(pMqtt->version == MQTTVERSION_5) {
          MQTTResponse response = MQTTResponse_initializer;
          response = MQTTClient_subscribe5(pMqtt->client, topic, qos, NULL, NULL);
          rc = response.reasonCode;
          MQTTResponse_free(response);
      } else {
          rc = MQTTClient_subscribe(pMqtt->client, topic, qos);
      }

      if(rc == MQTTCLIENT_SUCCESS) {
          Tcl_SetObjResult(interp, Tcl_NewBooleanObj(1));
      } else {
          Tcl_SetObjResult(interp, Tcl_NewBooleanObj(0));
      }

      break;
    }

    case MQTT_UNSUBSCRIBE: {
      char *topic = NULL;
      int rc;

      if( objc != 3 ){
        Tcl_WrongNumArgs(interp, 2, objv, "topic ");

        return TCL_ERROR;
      }

      topic = Tcl_GetStringFromObj(objv[2], 0);

      if(pMqtt->version == MQTTVERSION_5) {
          MQTTResponse response = MQTTResponse_initializer;
          response = MQTTClient_unsubscribe5(pMqtt->client, topic, NULL);
          rc = response.reasonCode;
          MQTTResponse_free(response);
      } else {
          rc = MQTTClient_unsubscribe(pMqtt->client, topic);
      }

      if(rc == MQTTCLIENT_SUCCESS) {
          Tcl_SetObjResult(interp, Tcl_NewBooleanObj(1));
      } else {
          Tcl_SetObjResult(interp, Tcl_NewBooleanObj(0));
      }

      break;
    }

    case MQTT_RECEIVE: {
      char *topicName = NULL;
      int topicLen;
      MQTTClient_message* message = NULL;
      int rc;
      Tcl_Obj *pResultStr;

      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 2, objv, 0);
        return TCL_ERROR;
      }

      pResultStr = Tcl_NewListObj(0, NULL);
      rc = MQTTClient_receive(pMqtt->client, &topicName, &topicLen, &message, pMqtt->timeout);

      // Is it OK?
      if(rc != MQTTCLIENT_SUCCESS) {
          return TCL_ERROR;
      }

      if (message) {
           Tcl_ListObjAppendElement(interp, pResultStr, 
                     Tcl_NewStringObj(topicName, -1));
           Tcl_ListObjAppendElement(interp, pResultStr, 
                     Tcl_NewStringObj(message->payload, message->payloadlen));
           Tcl_ListObjAppendElement(interp, pResultStr, 
                     Tcl_NewBooleanObj(message->dup));

           MQTTClient_freeMessage(&message);
           MQTTClient_free(topicName);
      }

      Tcl_SetObjResult(interp, pResultStr);

      break;
    }

    case MQTT_CLOSE: {
      if( objc != 2){
        Tcl_WrongNumArgs(interp, 2, objv, 0);
        return TCL_ERROR;
      }

      Tcl_DeleteCommand(interp, Tcl_GetStringFromObj(objv[0], 0));

      break;
    }

  } /* End of the SWITCH statement */

  return rc;
}


static int MQTTC_MAIN(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
  MQTTCDATA *p;
  const char *zArg;
  char *serverURI = NULL;
  char *clientId = NULL;
  int persistence_type = MQTTCLIENT_PERSISTENCE_ERROR;
  int timeout = 1000;
  int cleansession = 1;
  int cleanstart = 1;
  int keepalive = 20;
  char *version = NULL;
  char *username = NULL;
  char *password = NULL;
  int sslenable = 0;
  char *trustStore = NULL;
  char *keyStore = NULL;
  char *privateKey = NULL;
  char *privateKeyPassword = NULL;
  int enableServerCertAuth = 0;
  MQTTClient_createOptions createOpts = MQTTClient_createOptions_initializer;
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
  MQTTProperties connect_props = MQTTProperties_initializer;
  MQTTProperty property;
  int interval = -1;
  int i, rc;
  int length;

  if( objc < 5 ||  (objc&1) != 1){
    Tcl_WrongNumArgs(interp, 1, objv,
      "HANDLE serverURI clientId persistence_type ?-timeout timeout? "
      "?-cleansession boolean? ?-cleanstart boolean? ?-keepalive keepalive? "
      "?-username username? ?-password password? ?-sslenable boolean? "
      "?-trustStore truststore? ?-keyStore keystore? "
      "?-privateKey privatekey? ?-privateKeyPassword password? "
      "?-enableServerCertAuth boolean? ?-session-expiry-interval value? "
      "?-version version? "
    );
    return TCL_ERROR;
  }

  serverURI = Tcl_GetStringFromObj(objv[2], 0);
  clientId = Tcl_GetStringFromObj(objv[3], 0);

  if(Tcl_GetIntFromObj(interp, objv[4], &persistence_type) != TCL_OK) {
      return TCL_ERROR;
  }

  if(persistence_type < 0 || persistence_type > 1) {
      Tcl_AppendResult(interp, "persistence_type must be 0 or 1", (char*)0);
      return TCL_ERROR;
  }

  for(i=5; i+1<objc; i+=2){
    zArg = Tcl_GetStringFromObj(objv[i], 0);

    if( strcmp(zArg, "-timeout")==0 ){
        if(Tcl_GetIntFromObj(interp, objv[i + 1], &timeout) != TCL_OK) {
            return TCL_ERROR;
        }

        if(timeout <= 0) {
            Tcl_AppendResult(interp, "timeout must be > 0", (char*)0);
            return TCL_ERROR;
        }
    } else if( strcmp(zArg, "-keepalive")==0 ){
        if(Tcl_GetIntFromObj(interp, objv[i + 1], &keepalive) != TCL_OK) {
            return TCL_ERROR;
        }

        if(keepalive <= 0) {
            Tcl_AppendResult(interp, "keepalive must be > 0", (char*)0);
            return TCL_ERROR;
        }
    } else if( strcmp(zArg, "-cleansession")==0 ){
        if(Tcl_GetBooleanFromObj(interp, objv[i + 1], &cleansession) != TCL_OK) {
            return TCL_ERROR;
        }
    } else if( strcmp(zArg, "-cleanstart")==0 ){
        if(Tcl_GetBooleanFromObj(interp, objv[i + 1], &cleanstart) != TCL_OK) {
            return TCL_ERROR;
        }
    } else if( strcmp(zArg, "-username")==0 ){
        username = Tcl_GetStringFromObj(objv[i + 1], 0);
    } else if( strcmp(zArg, "-password")==0 ){
        password = Tcl_GetStringFromObj(objv[i + 1], 0);
    } else if( strcmp(zArg, "-sslenable")==0 ){
        if( Tcl_GetBooleanFromObj(interp, objv[i+1], &sslenable) ) return TCL_ERROR;
    } else if( strcmp(zArg, "-trustStore")==0 ){
        trustStore = Tcl_GetStringFromObj(objv[i + 1], 0);
    } else if( strcmp(zArg, "-keyStore")==0 ){
        keyStore = Tcl_GetStringFromObj(objv[i + 1], 0);
    } else if( strcmp(zArg, "-privateKey")==0 ){
        privateKey = Tcl_GetStringFromObj(objv[i + 1], 0);
    } else if( strcmp(zArg, "-privateKeyPassword")==0 ){
        privateKeyPassword = Tcl_GetStringFromObj(objv[i + 1], 0);
    } else if( strcmp(zArg, "-enableServerCertAuth")==0 ){
        if( Tcl_GetBooleanFromObj(interp, objv[i+1], &enableServerCertAuth) ) return TCL_ERROR;
    } else if( strcmp(zArg, "-session-expiry-interval")==0 ) {
        if(Tcl_GetIntFromObj(interp, objv[i + 1], &interval) != TCL_OK) {
            return TCL_ERROR;
        }

        if(interval < 0) {
            Tcl_AppendResult(interp, "interval must be >= 0", (char*)0);
            return TCL_ERROR;
        }
    } else if( strcmp(zArg, "-version")==0 ){
        version = Tcl_GetStringFromObj(objv[i + 1], 0);
    } else {
      Tcl_AppendResult(interp, "unknown option: ", zArg, (char*)0);
      return TCL_ERROR;
    }
  }

  // Don't let user give ssl URL but sslenable is false.
  length = strlen(serverURI);
  if((length > 3 && strncmp(serverURI, "ssl", 3)==0) && sslenable==0)
  {
    Tcl_SetResult(interp, (char *)"-sslenable needed setup to true", TCL_STATIC);
    return TCL_ERROR;
  }

  if((length > 3 && strncmp(serverURI, "tcp", 3)==0) && sslenable==1)
  {
    Tcl_SetResult(interp, (char *)"-sslenable needed setup to false", TCL_STATIC);
    return TCL_ERROR;
  }

  // Check version setting
  if(version != NULL) {
      if(strcmp(version, "3.1")==0) {
          createOpts.MQTTVersion = MQTTVERSION_3_1;
      } else if(strcmp(version, "3.1.1")==0) {
          createOpts.MQTTVersion = MQTTVERSION_3_1_1;
      } else if(strcmp(version, "5")==0) {
          createOpts.MQTTVersion = MQTTVERSION_5;
      } else {
          createOpts.MQTTVersion = MQTTVERSION_DEFAULT;
      }
  }

  p = (MQTTCDATA *)Tcl_Alloc( sizeof(*p) );
  if( p==0 ){
    Tcl_SetResult(interp, (char *)"malloc failed", TCL_STATIC);
    return TCL_ERROR;
  }

  memset(p, 0, sizeof(*p));

  rc = MQTTClient_createWithOptions(&(p->client), serverURI, clientId, persistence_type, 
		  NULL, &createOpts);
  if (rc != MQTTCLIENT_SUCCESS) {
      printf("return value %d\n", rc);
      Tcl_SetResult (interp, "Create MQTT client fail", NULL);

      if(p) Tcl_Free((char*) p);
      return TCL_ERROR;
  }

  if(createOpts.MQTTVersion==MQTTVERSION_5) {
      MQTTClient_connectOptions conn_opts5 = MQTTClient_connectOptions_initializer5;
      conn_opts = conn_opts5;
  }
  conn_opts.keepAliveInterval = keepalive;
  if(username) conn_opts.username = username;
  if(password) conn_opts.password = password;
  if(version) conn_opts.MQTTVersion = createOpts.MQTTVersion;

  if(sslenable) {
      if(trustStore) ssl_opts.trustStore = trustStore;
      if(keyStore) ssl_opts.keyStore = keyStore;
      if(privateKey) ssl_opts.privateKey = privateKey;
      if(privateKeyPassword) ssl_opts.privateKeyPassword = privateKeyPassword;
      ssl_opts.enableServerCertAuth = enableServerCertAuth;
      conn_opts.ssl = &ssl_opts;
  }

  if (createOpts.MQTTVersion == MQTTVERSION_5)
  {
      MQTTResponse response = MQTTResponse_initializer;
      conn_opts.cleanstart = cleanstart;

      if(interval >= 0) {
          property.identifier = MQTTPROPERTY_CODE_SESSION_EXPIRY_INTERVAL;
          property.value.integer4 = interval;
          MQTTProperties_add(&connect_props, &property);
      }

      response = MQTTClient_connect5(p->client, &conn_opts, &connect_props, NULL);
      rc = response.reasonCode;
      MQTTResponse_free(response);
  } else {
      conn_opts.cleansession = cleansession;

      rc = MQTTClient_connect(p->client, &conn_opts);
  }

  if (rc != MQTTCLIENT_SUCCESS)
  {
      printf("return value %d\n", rc);
      Tcl_SetResult (interp, "Connect MQTT server fail", NULL);

      if(p) Tcl_Free((char*) p);
      return TCL_ERROR;
  }

  p->interp = interp;
  p->version = createOpts.MQTTVersion;
  p->clientId = clientId;
  p->timeout = timeout;

  zArg = Tcl_GetStringFromObj(objv[1], 0);
  Tcl_CreateObjCommand(interp, zArg, MgttObjCmd, (char*)p, DbDeleteCmd);

  return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Mqttc_Init --
 *
 *	Initialize the new package.  The string "Mqttc" in the
 *	function name must match the PACKAGE declaration at the top of
 *	configure.ac.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side effects:
 *	The Mqttc package is created.
 *
 *----------------------------------------------------------------------
 */

int Mqttc_Init(Tcl_Interp *interp)
{
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }

    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    Tcl_CreateObjCommand(interp, "mqttc", (Tcl_ObjCmdProc *) MQTTC_MAIN,
            (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);


    return TCL_OK;
}
