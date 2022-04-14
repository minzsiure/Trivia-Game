#include <mpu6050_esp32.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

TFT_eSPI tft = TFT_eSPI();
int loop_timer;
//network setup
const uint16_t RESPONSE_TIMEOUT = 6000;
const uint16_t IN_BUFFER_SIZE = 3500; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
const uint16_t JSON_BODY_SIZE = 3000;
char request[IN_BUFFER_SIZE];
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
char json_body[JSON_BODY_SIZE];
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response

//button setup
const uint8_t BUTTON_GAME = 38;
const uint8_t BUTTON_TRUE = 39;
const uint8_t BUTTON_FALSE = 45;

//state machine setup
const char USER[] = "eva1"; //change name if u'd like to
const uint8_t START = 0;
const uint8_t WAIT_1 = 1;
const uint8_t QUESTION = 2;
const uint8_t CHECK_ANS = 3;
const uint8_t WAIT_2 = 4;
const uint8_t WAIT_3 = 5;
const uint8_t WAIT_4 = 6; //question -> start after pressing b38
const uint8_t WAIT_QUESTION = 7;
int indicator = 0;
int qs_indicator = 0;
uint8_t game_state = START;

//some game tracking vars
int score = 0; //for tracking current score
int correct_num = 0; //for #qs answered correct
int wrong_num = 0;

char curr_ans[100]; //initiate a holder for current answer
char trivia_ans[100];
char question[1000];

//button checking states
uint8_t prev_b1 = 1;
uint8_t prev_b2 = 1;
uint8_t prev_b3 = 1;


//wifi
uint32_t timer;
WiFiClientSecure client; //global WiFiClient Secure object
WiFiClient client2; //global WiFiClient Secure object
const char NETWORK[] = "MIT";
const char PASSWORD[] = "";
uint8_t scanning = 0;//set to 1 if you'd like to scan for wifi networks (see below):
uint8_t channel = 1; //network channel on 2.4 GHz
byte bssid[] = {0x04, 0x95, 0xE6, 0xAE, 0xDB, 0x41}; //6 byte MAC address of AP you're targeting.


int toggle_time;//used to remember last time LED switched value
int led_state;
const int FLASH_PERIOD = 200;

void setup() {
  Serial.begin(115200); //start serial at 115200 baud
  //SET UP SCREEN:
  tft.init();  //init screen
  tft.setRotation(2); //adjust rotation
  tft.setTextSize(1); //default font size, change if you want
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_PINK, TFT_BLACK); //set color of font to hot pink foreground, black background

  while (!Serial); //wait for serial to start
  pinMode(BUTTON_GAME, INPUT_PULLUP);
  pinMode(BUTTON_TRUE, INPUT_PULLUP);
  pinMode(BUTTON_FALSE, INPUT_PULLUP);

  //if using regular connection use line below:
  // NETWORK
  if (scanning) {
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
      Serial.println("no networks found");
    } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
        Serial.printf("%d: %s, Ch:%d (%ddBm) %s ", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
        uint8_t* cc = WiFi.BSSID(i);
        for (int k = 0; k < 6; k++) {
          Serial.print(*cc, HEX);
          if (k != 5) Serial.print(":");
          cc++;
        }
        Serial.println("");
      }
    }
  }
  delay(100); //wait a bit (100 ms)
  WiFi.begin(NETWORK, PASSWORD);
  //if using channel/mac specification for crowded bands use the following:
  //WiFi.begin(network, password, channel, bssid);
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(NETWORK);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str() , WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  timer = millis();
}

void loop() {
  uint8_t button1 = digitalRead(BUTTON_GAME);
  uint8_t button2 = digitalRead(BUTTON_TRUE);
  uint8_t button3 = digitalRead(BUTTON_FALSE);

  game_state_machine(button1, button2, button3);
}

void game_state_machine(uint8_t button1, uint8_t button2, uint8_t button3) {
  switch (game_state) {
    case START: {
        Serial.println("start");
        if(indicator == 0){
          Serial.println("sending get score request");
          //send a request to trivia_score.py on dev-2, get current score, if any
          sprintf(request_buffer, "GET http://608dev-2.net/sandbox/sc/xieyi/trivia_score.py?score=%s&user=%s HTTP/1.1\r\n", "True", USER);
          strcat(request_buffer, "Host: 608dev-2.net\r\n"); //add more to the end
          strcat(request_buffer, "\r\n"); //add blank line!
          //submit to function that performs GET.  It will return output using response_buffer char array
          do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
          Serial.println(response_buffer);
          DynamicJsonDocument doc(500);
          DeserializationError error = deserializeJson(doc, response_buffer);
          if(error){
            Serial.println("json error");
            return;
            }
          score = doc["score"];
          Serial.println(score);
          correct_num = doc["correct_num"];
          wrong_num = doc["wrong_num"];
          indicator = 1;
          }
          //display "press to start"
        
        tft.setCursor(0, 0, 1);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK); //set color of font to WHITE foreground, RED background
        tft.printf("Hi %s\nYou historical score is %i, with %i of correct answers and %i of wrong answers\n\n",USER,score, correct_num, wrong_num);
        tft.println("[Press Button 38 to Start A Game]");

        //ON PRESS button 38, go into wait_1, then QUESTION
        if (prev_b1 == 1 && button1 == 0) {
          prev_b1 = 0;
          game_state = WAIT_1;
          tft.fillScreen(TFT_BLACK); //fill background
          score = 0;
          correct_num = 0;
          wrong_num = 0;
        }
        break;
      }

    case WAIT_1: {
        if (prev_b1 == 0 && button1 == 1) {
          game_state = QUESTION;
          prev_b1 = 1;
        }

        //store the answer into trivia_ans
        break;
      }

    case QUESTION: {
        //extract question and answer from response_buffer, store each
        //display current score
        //send a request to travia.py on dev-2, get a response of question and answer
        if(qs_indicator == 0){
        sprintf(request_buffer, "GET http://608dev-2.net/sandbox/sc/xieyi/travia_request.py HTTP/1.1\r\n");
        strcat(request_buffer, "Host: 608dev-2.net\r\n"); //add more to the end
        strcat(request_buffer, "\r\n"); //add blank line!
        //submit to function that performs GET.  It will return output using response_buffer char array
        do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
        DynamicJsonDocument doc2(500);
        DeserializationError error = deserializeJson(doc2, response_buffer);
        if(error){
          Serial.println("json error");
          return;
         }
        strcpy(question, doc2["question"]);
        strcpy(trivia_ans, doc2["answer"]);
        
        char screen_display[1000];
        //display one question, and left button(39) for TRUE, right button(45) for FALSE
        sprintf(screen_display, "Correct: %i\nWrong:%i\nScore:%i\nQuestion:%s\nPress 39 for T\nPress 45 for F\nPress 38 to terminate the game", correct_num, wrong_num, score, question);

        tft.setCursor(0, 0, 1);
        tft.setTextSize(1); //default font size
        tft.setTextColor(TFT_WHITE, TFT_BLACK); //set color of font to WHITE foreground, RED background
        tft.println(screen_display);
        qs_indicator = 1;
        }

        //if press 39, go to wait_2
        if (prev_b2 == 1 && button2 == 0) {
          Serial.println("chose true");
          prev_b2 = 0;
          game_state = WAIT_2;
          tft.fillScreen(TFT_BLACK); //fill background
        }

        //if press 45, go to wait_3
        if (prev_b3 == 1 && button3 == 0) {
          Serial.println("chose false");
          prev_b3 = 0;
          game_state = WAIT_3;
          tft.fillScreen(TFT_BLACK); //fill background
        }

        //if press 38, go to wait_4
        if (prev_b1 == 1 && button1 == 0) {
          prev_b1 = 0;
          game_state = WAIT_4;
          tft.fillScreen(TFT_BLACK); //fill background
        }

        break;
      }

    case WAIT_2: {
        //on press 39(true), on release go into check_ans
        //update curr_ans = True
        Serial.println("wait state true");
        strcpy(curr_ans, "True");
        if (prev_b2 == 0 && button2 == 1) {
          game_state = CHECK_ANS;
          prev_b2 = 1;
        }

        break;
      }

    case WAIT_3: {
        //on press 45(false), on release go into check_ans
        //update curr_ans = False
        Serial.println("wait state false");
        strcpy(curr_ans, "False");
        if (prev_b3 == 0 && button3 == 1) {
          game_state = CHECK_ANS;
          prev_b3 = 1;
        }
        break;
      }

    case CHECK_ANS: {
        //check if curr_ans == trivia_ans
        Serial.println("checking");
        //if so, score++, correct_num++
        if (strcmp(curr_ans, trivia_ans) == 0) {
          score++;
          correct_num++;
          tft.setTextColor(TFT_WHITE, TFT_GREEN); //set color of font to WHITE foreground, RED background
          tft.setCursor(0, 0, 2);
          tft.setTextSize(2);
          tft.fillScreen(TFT_GREEN); //fill background
          tft.println("CORRECT");
          loop_timer = millis();
          while (millis()-loop_timer<400);
          loop_timer = millis();
        } else {
          //else, score--, wrong_num++
          score--;
          wrong_num++;
          tft.setTextColor(TFT_WHITE, TFT_RED); //set color of font to WHITE foreground, RED background
          tft.setCursor(0, 0, 2);
          tft.setTextSize(2);
          tft.fillScreen(TFT_RED); //fill background
          tft.println("INCORRECT");
          loop_timer = millis();
          while (millis()-loop_timer<400);
          loop_timer = millis();
        }
      
        //either way, go back to QUESTION once update the score
        memset(curr_ans, 0, sizeof curr_ans);
        game_state = WAIT_QUESTION;
        tft.fillScreen(TFT_BLACK); //fill background
        break;
      }

    case WAIT_QUESTION: {
        qs_indicator = 0;
        game_state = QUESTION;
        break;
      }

    case WAIT_4:
      //TODO: POST user_name, score, correct_num, wrong_num to server
      Serial.println("pressed POST");
      //post it
      char body[500]; //for body
      //{"user":"jodalyst","message":"cats and dogsss"}
      sprintf(body, "user=%s&score=%i&correct_num=%i&wrong_num=%i", USER, score,correct_num,wrong_num); //generate body, posting to User, 1 step
      int body_len = strlen(body); //calculate body length (for header reporting)
      sprintf(request_buffer, "POST https://608dev-2.net/sandbox/sc/xieyi/trivia_score.py HTTP/1.1\r\n");
      strcat(request_buffer, "Host: 608dev-2.net\r\n");
      strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
      sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
      strcat(request_buffer, "\r\n"); //new line from header to body
      strcat(request_buffer, body); //body
      strcat(request_buffer, "\r\n"); //new line
      Serial.println(request_buffer);
      do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
      Serial.println(response_buffer); //viewable in Serial Terminal

      //clear score
      score = 0;
      correct_num = 0;
      wrong_num = 0;
      //go back to START
      if (prev_b1 == 0 && button1 == 1) {
        game_state = START;
        prev_b1 = 1;
        indicator = 0;
      }
      break;
  }

}
