/*
  ThingsBoard.h - Library API for sending data to the ThingsBoard
  Based on PubSub MQTT library.
  Created by Olender M. Oct 2018.
  Released into the public domain.
*/
#ifndef ThingsBoard_h
#define ThingsBoard_h

#include "mbed.h"
#include <string>
#include "http_request.h"
#include "https_request.h"
#include "ArduinoJson.h"
#include "ArduinoJson/Polyfills/type_traits.hpp"

#ifndef TB_PAYLOAD_SIZE
#define TB_PAYLOAD_SIZE 64
#endif

#ifndef TB_FIELDS_AMT
#define TB_FIELDS_AMT 8
#endif

#define OK_SUCCESS              200     // OK / Success

//#define PRINT_HTTP

// Telemetry record class, allows to store different data using common interface.
class Telemetry {
//  template <size_t PayloadSize = Default_Payload,
//            size_t MaxFieldsAmt = Default_Fields_Amt,
//            typename Logger = ThingsBoardDefaultLogger>
//  friend class ThingsBoardSized;

//  template <size_t PayloadSize = Default_Payload,
//            size_t MaxFieldsAmt = Default_Fields_Amt,
//            typename Logger = ThingsBoardDefaultLogger>
  friend class ThingsBoardHttpSized;
  friend class ThingsBoardHttpsSized;

public:
  inline Telemetry()
    :m_type(TYPE_NONE), m_key(NULL), m_value() { }

  // Constructs telemetry record from integer value.
  // EnableIf trick is required to overcome ambiguous float/integer conversion
  template<
      typename T,
      typename = ARDUINOJSON_NAMESPACE::enable_if<ARDUINOJSON_NAMESPACE::is_integral<T>::value>
  >
  inline Telemetry(const char *key, T val)
  :m_type(TYPE_INT), m_key(key), m_value()   { m_value.integer = val; }

  // Constructs telemetry record from boolean value.
  inline Telemetry(const char *key, bool val)
  :m_type(TYPE_BOOL), m_key(key), m_value()  { m_value.boolean = val; }

  // Constructs telemetry record from float value.
  inline Telemetry(const char *key, float val)
  :m_type(TYPE_REAL), m_key(key), m_value()  { m_value.real = val; }

  // Constructs telemetry record from string value.
  inline Telemetry(const char *key, const char *val)
  :m_type(TYPE_STR), m_key(key), m_value()   { m_value.str = val; }
  
  inline void setValue(int val)         { m_type = TYPE_INT;  m_value.integer = val; }
  inline void setValue(uint32_t val)    { m_type = TYPE_UINT; m_value.uinteger = val; }
  inline void setValue(bool val)        { m_type = TYPE_BOOL; m_value.boolean = val; }
  inline void setValue(float val)       { m_type = TYPE_REAL; m_value.real = val; }
  inline void setValue(const char *val) { m_type = TYPE_STR;  m_value.str = val; }

private:
  // Data container
  union data {
    const char  *str;
    bool        boolean;
    int         integer;
    uint32_t    uinteger;
    float       real;
  };

  // Data type inside a container
  enum dataType {
    TYPE_NONE,
    TYPE_BOOL,
    TYPE_UINT,
    TYPE_INT,
    TYPE_REAL,
    TYPE_STR,
  };

  dataType     m_type;  // Data type flag
  const char   *m_key;  // Data key
  data         m_value; // Data value

  // Serializes key-value pair in a generic way.
  bool serializeKeyval(JsonVariant &jsonObj) const {
    if (m_key) {
      switch (m_type) {
        case TYPE_BOOL:
          jsonObj[m_key] = m_value.boolean;
        break;
        case TYPE_INT:
          jsonObj[m_key] = m_value.integer;
        break;
        case TYPE_UINT:
          jsonObj[m_key] = m_value.uinteger;
        break;
        case TYPE_REAL:
          jsonObj[m_key] = m_value.real;
        break;
        case TYPE_STR:
          jsonObj[m_key] = m_value.str;
        break;
        default:
        break;
      }
    } else {
      switch (m_type) {
        case TYPE_BOOL:
          return jsonObj.set(m_value.boolean);
        break;
        case TYPE_INT:
          return jsonObj.set(m_value.integer);
        break;
        case TYPE_UINT:
          return jsonObj.set(m_value.uinteger);
        break;
        case TYPE_REAL:
          return jsonObj.set(m_value.real);
        break;
        case TYPE_STR:
          return jsonObj.set(m_value.str);
        break;
        default:
        break;
      }
    }
    return true;
  }
};

//// Convenient aliases

using Attribute = Telemetry;

class ThingsBoardLogger
{
public:
  void log(const char *msg) {}
};

class ThingsBoardDefaultLogger : public ThingsBoardLogger
{
public:
  void log(const char *msg) {
  printf("[TB] %s\n", msg);
}
};

// ThingsBoard HTTP client class
//template <size_t PayloadSize, size_t MaxFieldsAmt, typename Logger>
class ThingsBoardHttpSized
{
public:
  // Initializes ThingsBoardHttpSized class with network client.
  inline ThingsBoardHttpSized() { }

  // Destroys ThingsBoardHttpSized class with network client.
  inline ~ThingsBoardHttpSized() { }
  
  inline void begin(TCPSocket *socket, const char *access_token, const char *host, int port, ThingsBoardLogger *logger = new ThingsBoardDefaultLogger()) {
    m_Socket = socket;
    m_Token = access_token;
    m_Host = host;
    m_Port = port;
    m_Logger = logger;
  }


  //----------------------------------------------------------------------------
  // Telemetry API

  // Sends integer telemetry data to the ThingsBoard, returns true on success.
  inline bool sendTelemetryInt(const char *key, int value) {
    return sendKeyval(key, value);
  }

  // Sends boolean telemetry data to the ThingsBoard, returns true on success.
  inline bool sendTelemetryBool(const char *key, bool value) {
    return sendKeyval(key, value);
  }

  // Sends float telemetry data to the ThingsBoard, returns true on success.
  inline bool sendTelemetryFloat(const char *key, float value) {
    return sendKeyval(key, value);
  }

  // Sends string telemetry data to the ThingsBoard, returns true on success.
  inline bool sendTelemetryString(const char *key, const char *value) {
    return sendKeyval(key, value);
  }

  // Sends aggregated telemetry to the ThingsBoard.
  inline bool sendTelemetry(const Telemetry *data, size_t data_count) {
    return sendDataArray(data, data_count);
  }

  //----------------------------------------------------------------------------
  // Attribute API

  // Sends integer attribute with given name and value.
  inline bool sendAttributeInt(const char *attrName, int value) {
    return sendKeyval(attrName, value, false);
  }

  // Sends boolean attribute with given name and value.
  inline bool sendAttributeBool(const char *attrName, bool value) {
    return sendKeyval(attrName, value, false);
  }

  // Sends float attribute with given name and value.
  inline bool sendAttributeFloat(const char *attrName, float value) {
    return sendKeyval(attrName, value, false);
  }

  // Sends string attribute with given name and value.
  inline bool sendAttributeString(const char *attrName, const char *value) {
    return sendKeyval(attrName, value, false);
  }

  // Sends aggregated attributes to the ThingsBoard.
  inline bool sendAttributes(const Attribute *data, size_t data_count) {
    return sendDataArray(data, data_count, false);
  }

private:
  // Sends custom JSON telemetry or attribute string to the ThingsBoard, using HTTP.
  inline bool sendTelemetryOrAttributeJson(const char *json, bool telemetry) {
    if (!json || !m_Token) {
      return  false;
    }

    string path = string("http://") + string(m_Host) + string(":") + std::to_string(m_Port) + string("/api/v1/") + m_Token + (telemetry?"/telemetry":"/attributes");
 
    HttpRequest* request = new HttpRequest(this->m_Socket, HTTP_POST, path.c_str());
    //request->set_header("User-Agent", TS_USER_AGENT);
//    request->set_header("Connection", "close");
    request->set_header("Content-Type", "application/json");
    //request->set_body(json);

    #ifdef PRINT_HTTP
      printf("POST \"%s\"\nbody \"%s\"", path.c_str(), json);
    #endif

    HttpResponse* response = request->send(json, strlen(json));

    #ifdef PRINT_HTTP
    printf("\n----- HTTP POST response -----\n");
    printf("Status: %d - %s\n", response->get_status_code(), response->get_status_message().c_str());

    printf("Headers:\n");
    for (size_t ix = 0; ix < response->get_headers_length(); ix++) {
      printf("\t%s: %s\n", response->get_headers_fields()[ix]->c_str(), response->get_headers_values()[ix]->c_str());
    }
    printf("\nBody (%d bytes):\n\n%s\n", response->get_body_length(), response->get_body_as_string().c_str());
    #endif

    int status = response->get_status_code();
    delete request;

    return (status == OK_SUCCESS);
  }

  // Sends array of attributes or telemetry to ThingsBoard
  bool sendDataArray(const Telemetry *data, size_t data_count, bool telemetry = true) {
    if (TB_FIELDS_AMT < data_count) {
      m_Logger->log("too much JSON fields passed");
      return false;
    }
    char payload[TB_PAYLOAD_SIZE];
    {
      StaticJsonDocument<JSON_OBJECT_SIZE(TB_FIELDS_AMT)> jsonBuffer;
      JsonVariant object = jsonBuffer.template to<JsonVariant>();

      for (size_t i = 0; i < data_count; ++i) {
        if (data[i].serializeKeyval(object) == false) {
          m_Logger->log("unable to serialize data");
          return false;
        }
      }

      if (measureJson(jsonBuffer) > TB_PAYLOAD_SIZE - 1) {
        m_Logger->log("too small buffer for JSON data");
        return false;
      }
      serializeJson(object, payload, sizeof(payload));
    }

    return sendTelemetryOrAttributeJson(payload, telemetry);
}

  // Sends single key-value in a generic way.
  template<typename T>
  bool sendKeyval(const char *key, T value, bool telemetry = true) {
    Telemetry t(key, value);

    char payload[TB_PAYLOAD_SIZE];
    {
      Telemetry t(key, value);
      StaticJsonDocument<JSON_OBJECT_SIZE(1)> jsonBuffer;
      JsonVariant object = jsonBuffer.template to<JsonVariant>();
      if (t.serializeKeyval(object) == false) {
        m_Logger->log("unable to serialize data");
        return false;
      }

      if (measureJson(jsonBuffer) > TB_PAYLOAD_SIZE - 1) {
        m_Logger->log("too small buffer for JSON data");
        return false;
      }
      serializeJson(object, payload, sizeof(payload));
    }
    return sendTelemetryOrAttributeJson(payload, telemetry);
  }

  TCPSocket *m_Socket;
  const char *m_Token;
  const char *m_Host;
  int m_Port;
  ThingsBoardLogger *m_Logger;
};

// ThingsBoard HTTPS client class
//template <size_t PayloadSize, size_t MaxFieldsAmt, typename Logger>
class ThingsBoardHttpsSized
{
public:
  // Initializes ThingsBoardHttpSized class with network client.
  inline ThingsBoardHttpsSized() { }

  // Destroys ThingsBoardHttpSized class with network client.
  inline ~ThingsBoardHttpsSized() { }
  
  inline void begin(TLSSocket *socket, const char *access_token, const char *host, int port, ThingsBoardLogger *logger = new ThingsBoardDefaultLogger()) {
    m_Socket = socket;
    m_Token = access_token;
    m_Host = host;
    m_Port = port;
    m_Logger = logger;
  }
  
  inline void begin(const char *access_token, const char *host, int port, ThingsBoardLogger *logger = new ThingsBoardDefaultLogger()) {
    m_Socket = nullptr;
    m_Token = access_token;
    m_Host = host;
    m_Port = port;
    m_Logger = logger;
  }
  
  inline void setSocket(TLSSocket *socket) {
    m_Socket = socket;
  }


  //----------------------------------------------------------------------------
  // Telemetry API

  // Sends integer telemetry data to the ThingsBoard, returns true on success.
  inline bool sendTelemetryInt(const char *key, int value) {
    return sendKeyval(key, value);
  }

  // Sends boolean telemetry data to the ThingsBoard, returns true on success.
  inline bool sendTelemetryBool(const char *key, bool value) {
    return sendKeyval(key, value);
  }

  // Sends float telemetry data to the ThingsBoard, returns true on success.
  inline bool sendTelemetryFloat(const char *key, float value) {
    return sendKeyval(key, value);
  }

  // Sends string telemetry data to the ThingsBoard, returns true on success.
  inline bool sendTelemetryString(const char *key, const char *value) {
    return sendKeyval(key, value);
  }

  // Sends aggregated telemetry to the ThingsBoard.
  inline bool sendTelemetry(const Telemetry *data, size_t data_count) {
    return sendDataArray(data, data_count);
  }

  //----------------------------------------------------------------------------
  // Attribute API

  // Sends integer attribute with given name and value.
  inline bool sendAttributeInt(const char *attrName, int value) {
    return sendKeyval(attrName, value, false);
  }

  // Sends boolean attribute with given name and value.
  inline bool sendAttributeBool(const char *attrName, bool value) {
    return sendKeyval(attrName, value, false);
  }

  // Sends float attribute with given name and value.
  inline bool sendAttributeFloat(const char *attrName, float value) {
    return sendKeyval(attrName, value, false);
  }

  // Sends string attribute with given name and value.
  inline bool sendAttributeString(const char *attrName, const char *value) {
    return sendKeyval(attrName, value, false);
  }

  // Sends aggregated attributes to the ThingsBoard.
  inline bool sendAttributes(const Attribute *data, size_t data_count) {
    return sendDataArray(data, data_count, false);
  }

private:
  // Sends custom JSON telemetry or attribute string to the ThingsBoard, using HTTP.
  inline bool sendTelemetryOrAttributeJson(const char *json, bool telemetry) {
    if(!json || !m_Token)
      return  false;
    
    if(m_Socket == nullptr)
      return false;

    string path = string("https://") + string(m_Host) + string(":") + std::to_string(m_Port) + string("/api/v1/") + m_Token + (telemetry?"/telemetry":"/attributes");
 
    HttpsRequest* request = new HttpsRequest(this->m_Socket, HTTP_POST, path.c_str());
    request->set_header("Content-Type", "application/json");

    #ifdef PRINT_HTTP
      printf("POST \"%s\"\nbody \"%s\"", path.c_str(), json);
    #endif

    HttpResponse* response = request->send(json, strlen(json));

    #ifdef PRINT_HTTP
    printf("\n----- HTTP POST response -----\n");
    printf("Status: %d - %s\n", response->get_status_code(), response->get_status_message().c_str());

    printf("Headers:\n");
    for (size_t ix = 0; ix < response->get_headers_length(); ix++) {
      printf("\t%s: %s\n", response->get_headers_fields()[ix]->c_str(), response->get_headers_values()[ix]->c_str());
    }
    printf("\nBody (%d bytes):\n\n%s\n", response->get_body_length(), response->get_body_as_string().c_str());
    #endif

    int status = response->get_status_code();
    delete request;

    return (status == OK_SUCCESS);
  }

  // Sends array of attributes or telemetry to ThingsBoard
  bool sendDataArray(const Telemetry *data, size_t data_count, bool telemetry = true) {
    if (TB_FIELDS_AMT < data_count) {
      m_Logger->log("too much JSON fields passed");
      return false;
    }
    char payload[TB_PAYLOAD_SIZE];
    {
      StaticJsonDocument<JSON_OBJECT_SIZE(TB_FIELDS_AMT)> jsonBuffer;
      JsonVariant object = jsonBuffer.template to<JsonVariant>();

      for (size_t i = 0; i < data_count; ++i) {
        if (data[i].serializeKeyval(object) == false) {
          m_Logger->log("unable to serialize data");
          return false;
        }
      }

      if (measureJson(jsonBuffer) > TB_PAYLOAD_SIZE - 1) {
        m_Logger->log("too small buffer for JSON data");
        return false;
      }
      serializeJson(object, payload, sizeof(payload));
    }

    return sendTelemetryOrAttributeJson(payload, telemetry);
}

  // Sends single key-value in a generic way.
  template<typename T>
  bool sendKeyval(const char *key, T value, bool telemetry = true) {
    Telemetry t(key, value);

    char payload[TB_PAYLOAD_SIZE];
    {
      Telemetry t(key, value);
      StaticJsonDocument<JSON_OBJECT_SIZE(1)> jsonBuffer;
      JsonVariant object = jsonBuffer.template to<JsonVariant>();
      if (t.serializeKeyval(object) == false) {
        m_Logger->log("unable to serialize data");
        return false;
      }

      if (measureJson(jsonBuffer) > TB_PAYLOAD_SIZE - 1) {
        m_Logger->log("too small buffer for JSON data");
        return false;
      }
      serializeJson(object, payload, sizeof(payload));
    }
    return sendTelemetryOrAttributeJson(payload, telemetry);
  }

  TLSSocket *m_Socket;
  const char *m_Token;
  const char *m_Host;
  const char* m_ssl_ca_pem;
  int m_Port;
  ThingsBoardLogger *m_Logger;
};

using ThingsBoardHttp = ThingsBoardHttpSized;
using ThingsBoardHttps = ThingsBoardHttpsSized;

//using ThingsBoard = ThingsBoardSized<>;

#endif // ThingsBoard_h
