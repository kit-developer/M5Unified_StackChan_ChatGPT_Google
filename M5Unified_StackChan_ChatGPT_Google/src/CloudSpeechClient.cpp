#include "CloudSpeechClient.h"
#include "network_param.h"
#include <base64.h>
#include <ArduinoJson.h>

CloudSpeechClient::CloudSpeechClient(Authentication authentication) {
  this->authentication = authentication;
  client.setCACert(root_ca);
  client.setTimeout( 10000 ); 
  if (!client.connect(server_stt, 443)) Serial.println("Connection failed!");
}

CloudSpeechClient::~CloudSpeechClient() {
  client.stop();
}

void CloudSpeechClient::PrintHttpBody2(Audio* audio) {
  String enc = base64::encode(audio->paddedHeader, sizeof(audio->paddedHeader));
  enc.replace("\n", "");  // delete last "\n"
  client.print(enc);      // HttpBody2
  Serial.println("audio1");
  char* wavData = (char*)audio->wavData;
  Serial.println("audio2");
  for (int j = 0; j < audio->record_number; j++) {
    enc = base64::encode((byte*)&wavData[j*audio->record_length*2], audio->record_length*2);
    enc.replace("\n", "");// delete last "\n"
    client.print(enc);    //Serial.print(enc); // HttpBody2
    delay(10);
  } 
//  Serial.printf("PrintHttpBody2=%d",len);
}

String CloudSpeechClient::Transcribe(Audio* audio) {
  String HttpBody1 = "{\
                        \"config\":\
                        {\
                          \"encoding\":\"LINEAR16\",\
                          \"sampleRateHertz\":16000,\
                          \"languageCode\":\""+LANG_CODE+"\"\
                        },\
                        \"audio\":\
                        {\
                          \"content\":\"";
  String HttpBody3 = "\"}}\r\n\r\n";
  int httpBody2Length = (audio->wavDataSize + sizeof(audio->paddedHeader))*4/3;  // 4/3 is from base64 encoding
  String ContentLength = String(HttpBody1.length() + httpBody2Length + HttpBody3.length());
//  Serial.printf("HttpBody1=%d httpBody2Length=%d HttpBody3=%d \n",HttpBody1.length(),httpBody2Length,HttpBody3.length());
  
  Serial.println("ヘッダ準備");
  String HttpHeader;
  if (authentication == USE_APIKEY) 
    // HttpHeader = String("POST speech/recognition/conversation/cognitiveservices/v1?language=ja-JP&format=detailed HTTP/1.1\r\n\
    // Accept: application/json;text/xml\r\n\
    // Content-Type: audio/wav; codecs=audio/pcm; samplerate=16000\r\n\
    // Ocp-Apim-Subscription-Key: ") + api_key + String("\r\n\
    // Host: westus.stt.speech.microsoft.com\r\n\
    // Transfer-Encoding: chunked\r\n\
    // Expect: 100-continue\r\n\
    // Content-Length: ")
    // + ContentLength + String("\r\n\r\n");
    HttpHeader = String("POST speech/recognition/conversation/cognitiveservices/v1?language=ja-JP&format=detailed HTTP/1.1\r\nAccept: application/json;text/xml\r\nContent-Type: audio/wav; codecs=audio/pcm; samplerate=16000\r\nOcp-Apim-Subscription-Key: ") + AZURE_API_KEY
    + String("\r\nHost: japaneast.stt.speech.microsoft.com\r\nTransfer-Encoding: chunked\r\nExpect: 100-continue\r\nContent-Length: ")
    + ContentLength + String("\r\n\r\n");
    // HttpHeader = String("POST /v1/speech:recognize?key=") + api_key
    // + String(" HTTP/1.1\r\nHost: speech.googleapis.com\r\nContent-Type: application/json\r\nContent-Length: ")
    // + ContentLength + String("\r\n\r\n");
  // else if (authentication == USE_ACCESSTOKEN)
  //   HttpHeader = String("POST /v1/speech:recognize HTTP/1.1\r\n\
  //   Host: speech.googleapis.com\r\n\
  //   Content-Type: application/json\r\n\
  //   Authorization: Bearer ")
  //   + AccessToken + String("\r\nContent-Length: ") + ContentLength + String("\r\n\r\n");
  
  Serial.println("HttpHeader");
  client.print(HttpHeader); //Serial.print(HttpHeader);
  Serial.println("HttpBody1");
  client.print(HttpBody1); //Serial.print(HttpBody1);
  Serial.println("audio");
  PrintHttpBody2(audio);
  Serial.println("HttpBody3");
  client.print(HttpBody3); //Serial.print(HttpBody3);
  
  Serial.println("client.available");
  while (!client.available());
  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return String("");
  }
  Serial.println("client.read");
  if(client.available())client.read();
  if(client.available())client.read();
  if(client.available())client.read();

  Serial.println("jsonBuffer準備");

  // Parse JSON object
  StaticJsonDocument<500> jsonBuffer;
  DeserializationError error = deserializeJson(jsonBuffer, client);
//root.prettyPrintTo(Serial); //Serial.println("");
  String result = "";
  if (error) {
    Serial.println("Parsing failed!");
    return result;
  } else {
    String  json_string;
    serializeJsonPretty(jsonBuffer, json_string);
    Serial.println("====================");
    Serial.println(json_string);
    Serial.println("====================");
//  root.prettyPrintTo(Serial);
    const char* text = jsonBuffer["results"][0]["alternatives"][0]["transcript"];
    Serial.print("\n認識結果：");
    if(text) {
      result = String (text);
      Serial.println((char *)text);
    }
    else {
      Serial.println("NG");
    }
  }
  return result;
}

