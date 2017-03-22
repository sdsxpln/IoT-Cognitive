
#include "NTPClient.h"
#include "SpeechInterface.h"
#include "SASToken.h"
#include "picojson.h"

#if _debug
#define DBG(x, ...)  printf("[SPEECHINTERFACE: DBG] %s \t[%s,%d]\r\n", x, ##__VA_ARGS__, __FILE__, __LINE__); 
#define WARN(x, ...) printf("[SPEECHINTERFACE: WARN] %s \t[%s,%d]\r\n", x, ##__VA_ARGS__, __FILE__, __LINE__); 
#define ERR(x, ...)  printf("[SPEECHINTERFACE: ERR] %s \t[%s,%d]\r\n", x, ##__VA_ARGS__, __FILE__, __LINE__); 
#endif

#define GUID_SIZE 36
#define SPEECH_RECOGNITION_API_REQUEST_URL  ""                                                                  \
                                            "https://speech.platform.bing.com/recognize?"                       \
                                            "scenarios=smd&appid=D4D52672-91D7-4C74-8AD8-42B1D98141A5"          \
                                            "&locale=en-us&device.os=bot"                                       \
                                            "&form=BCSSTT&version=3.0&format=json&instanceid=%s&requestid=%s"

#define GUID_GENERATOR_HTTP_REQUEST_URL  "http://www.fileformat.info/tool/guid.htm?count=1&format=text&hyphen=true"
#define TOKEN_REQUEST_URL "https://api.cognitive.microsoft.com/sts/v1.0/issueToken"

const char CERT[] = 
"-----BEGIN CERTIFICATE-----\r\nMIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\r\n"
"RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\r\nVQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\r\n"
"DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\r\nZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\r\n"
"VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\r\nmD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\r\n"
"IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\r\nmpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\r\n"
"XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\r\ndc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\r\n"
"jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\r\nBE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\r\n"
"DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\r\n9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\r\n"
"jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\r\nEpn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\r\n"
"ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\r\nR9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\r\n-----END CERTIFICATE-----\r\n";

SpeechInterface::SpeechInterface(NetworkInterface * networkInterface, const char * subscriptionKey, const char * deviceId, bool debug)
{
    _wifi = networkInterface;
    _requestUri = (char *)malloc(300);
    _cognitiveSubKey = (char *)malloc(33);
    _deviceId = (char *)malloc(37);

    memcpy(_cognitiveSubKey, subscriptionKey, 33);
    memcpy(_deviceId, deviceId, 37);
    
    _response = NULL;
    _debug = debug;

    printf("subscriptionKey: %s, deviceId: %s \r\n", subscriptionKey, deviceId);
}

SpeechInterface::~SpeechInterface(void)
{  
    delete _cognitiveSubKey;
    delete _deviceId;
    delete _requestUri;

    if (_response)
    {
        delete _response;
    }
}

int SpeechInterface::generateGuidStr(char * guidStr)
{
    if (guidStr == NULL)
    {
        return -1;
    }

    HttpRequest* guidRequest = new HttpRequest(_wifi, HTTP_GET, GUID_GENERATOR_HTTP_REQUEST_URL);
    _response = guidRequest->send();
    if (!_response)
    {
        printf("Guid generator HTTP request failed.\r\n");
        return -1;
    }

    strcpy(guidStr, _response->get_body().c_str());   
    printf("Got new guid: %s \r\n", guidStr);
    
    //free(guidRequest);
    return strlen(guidStr);
}

string SpeechInterface::getJwtToken()
{    
    HttpsRequest* tokenRequest = new HttpsRequest(_wifi, CERT, HTTP_POST, TOKEN_REQUEST_URL);
    tokenRequest->set_header("Ocp-Apim-Subscription-Key", _cognitiveSubKey);
    _response = tokenRequest->send();
    if (!_response)
    {
        printf("HttpRequest failed (error code %d)\n", tokenRequest->get_error());
        return NULL;
    }

    string token = _response->get_body();
    printf("Got JWT token: %s\r\n", token.c_str());
    
    delete tokenRequest;  
    return token;
}

SpeechResponse* SpeechInterface::recognizeSpeech(char * audioFileBinary, int length)
{
    // Generate a new guid for cognitive service API request
    char * guid = (char *)malloc(33);
    generateGuidStr(guid);

    // Generate a JWT token for cognitove service authentication
    string jwtToken = getJwtToken();
    
    // Preapre Speech Recognition API request URL
    sprintf(_requestUri, SPEECH_RECOGNITION_API_REQUEST_URL, _deviceId, guid);
    printf("recognizeSpeech request URL: %s\r\n", _requestUri);

    HttpsRequest* speechRequest = new HttpsRequest(_wifi, CERT, HTTP_POST, _requestUri);
    speechRequest->set_header("Authorization", "Bearer " + jwtToken);
    speechRequest->set_header("Content-Type", "plain/text");
    
    _response = speechRequest->send(audioFileBinary, length);
    if (!_response)
    {
        printf("Speech API request failed.");
        return NULL;
    }
    
    string body = _response->get_body();
    printf("congnitive result: %s\r\n", body.c_str());

    SpeechResponse* speechResponse;
    char * bodyStr = (char*)body.c_str();

    // Parse Json result to SpeechResponse object
    picojson::value json;
    string err = picojson::parse(json, bodyStr, bodyStr + strlen(bodyStr));
    if (err != "")
    {
        printf("Parse json response error: %s\r\n", err.c_str());
        return NULL;
    }

    speechResponse->status = (char *)json.get("header").get("status").get<string>().c_str();
    if (strcmp(speechResponse->status, "error") == 0)
    {
        printf("Unable to recognize the speech.\r\n");
    }

    picojson::array results = json.get("results").get<picojson::array>();
    picojson::array::iterator iter = results.begin();  
    speechResponse->text = (char *)(*iter).get("name").get<string>().c_str();
    char * confidenceStr = (char *)(*iter).get("confidence").get<string>().c_str();
    speechResponse->confidence = atof(confidenceStr);

    free(guid);
    delete speechRequest;
    
    return speechResponse;
}

int SpeechInterface::convertTextToSpeech(char * text, int length, char * audioFileBinary, int audioLen)
{
    return 0;
}

int SpeechInterface::setupRealTime(void)
{
    NTPClient ntp(*_wifi);
    int result = 0;
    do {
        result = ntp.setTime("0.pool.ntp.org");
    } while (result != 0);
    return result;
}

int SpeechInterface::sentToIotHub(char * file, int length)
{
    SASToken iothubtoken;
    do {
        setupRealTime();
    } while(strlen(iothubtoken.getValue(time(NULL))) == 0);
    sprintf(_requestUri, "https://%s/devices/%s/messages/events?api-version=2016-11-14", IOTHUB_HOST, DEVICE_ID);
    printf("<%s>\r\n", _requestUri);
    HttpsRequest* iotRequest = new HttpsRequest(_wifi, CERT, HTTP_POST, _requestUri);
    iotRequest->set_header("Authorization", iothubtoken.getValue(time(NULL)));
    
    _response = iotRequest->send(file, length);
    string result = _response->get_body();
    printf("iot hub result <%s>\r\n", result.c_str());
    
    delete iotRequest;
    return 0;
}


