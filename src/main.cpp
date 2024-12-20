#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include <map>
#include <coin.h>
#include <memory>
#include <mutex>
#include <secrets.h>

#define OLED_ADDR 0x3C
#define BTN_PIN 19

void displayCoin();
String formatTime(tm* time);
String formatPrice(float price);
void updateCoinsTask(void *pvParameters);

Adafruit_SSD1306 display(128, 64, &Wire, -1);

std::mutex coinsMapMutex; 
std::map<String, std::unique_ptr<CoinData>> coins;

void setup() {
  // PINS

  pinMode(BTN_PIN, INPUT_PULLUP);

  /////////////////////////////////

  configTime(-3*3600, 0, "pool.ntp.org");

  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);

  display.print("Connecting to Wi-Fi");
  Serial.println("Connecting to Wi-Fi");

  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.print(".");
    display.display();
  }

  display.println("Connected.");
  Serial.println("Connected.");

  Serial.printf("IP Addr: %s\n", WiFi.localIP().toString().c_str());
  display.print("IP Addr: ");
  display.println(WiFi.localIP());
  display.display();

  xTaskCreate(
    updateCoinsTask,
    "UpdateCoinsTask",
    10000,
    NULL,
    1,
    NULL
  );
  
  displayCoin();
}

int state = 0;
bool isPressed = false;
unsigned long lastDebounceTime = 0;  // Tempo da última leitura estável do botão
unsigned long debounceDelay = 50;    // Atraso para debouncing (em milissegundos)
int lastButtonState = HIGH;          // Último estado do botão

void loop() {
  int buttonState = digitalRead(BTN_PIN);

  // Verifica se houve uma mudança de estado do botão
  if (buttonState != lastButtonState) {
    lastDebounceTime = millis();  // Atualiza o tempo de debounce
  }

  // Só processa a mudança de estado se o botão estiver estabilizado por um tempo
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Se o botão foi pressionado (LOW)
    if (buttonState == LOW && !isPressed) {
      isPressed = true;
      // Avança o estado quando o botão for pressionado
      if (state < 2) {
        state++;
      } else {
        state = 0;
      }
    }
    // Se o botão foi liberado (HIGH)
    else if (buttonState == HIGH) {
      isPressed = false;
    }
  }

  // Armazena o estado atual do botão para a próxima leitura
  lastButtonState = buttonState;

  displayCoin();
}

void updateCoinsTask(void *pvParameters) {
  coinsMapMutex.lock();
  coins["BTCUSDT"] = std::unique_ptr<CoinData>(new CoinData{.0, tm{}});
  coins["ETHUSDT"] = std::unique_ptr<CoinData>(new CoinData{.0, tm{}});
  coins["SOLUSDT"] = std::unique_ptr<CoinData>(new CoinData{.0, tm{}});
  coinsMapMutex.unlock();
  
  while (true) {
    CoinData btcUsdt = getCoinData("BTC", "USDT");
    CoinData ethUsdt = getCoinData("ETH", "USDT");
    CoinData solUsdt = getCoinData("SOL", "USDT");

    coinsMapMutex.lock();
    coins["BTCUSDT"] = std::unique_ptr<CoinData>(new CoinData(btcUsdt));
    coins["ETHUSDT"] = std::unique_ptr<CoinData>(new CoinData(ethUsdt));
    coins["SOLUSDT"] = std::unique_ptr<CoinData>(new CoinData(solUsdt));
    coinsMapMutex.unlock();
    delay(5000);
  }
}

void displayCoin() {
  display.clearDisplay();

  display.setCursor(0, 0);
  display.setTextSize(2);

  CoinData data = CoinData{price: .0, updatedTime: tm{}};
  if (state == 0) {
    display.println("Bitcoin");
    coinsMapMutex.lock();
    data = *(coins.find("BTCUSDT")->second);
    coinsMapMutex.unlock();
  } else if (state == 1) {
    display.println("Ethereum");
    coinsMapMutex.lock();
    data = *(coins.find("ETHUSDT")->second);
    coinsMapMutex.unlock();
  } else {
    display.println("Solana");
    coinsMapMutex.lock();
    data = *(coins.find("SOLUSDT")->second);
    coinsMapMutex.unlock();
  }

  display.setCursor(0, 25);
  display.setTextSize(1);
  display.printf("$");
  display.setTextSize(3);
  display.printf("%s\n", formatPrice(data.price));

  display.setCursor(0, 55);
  display.setTextSize(1);
  display.printf("Updated %s\n", formatTime(&data.updatedTime));

  display.display();
}

String formatTime(tm* time) {
  char formattedTime[6];
  snprintf(formattedTime, sizeof(formattedTime), "%02d:%02d",
           time->tm_hour,
           time->tm_min);
  return String(formattedTime);
}

String formatPrice(float price) {
  String priceStr = String((int)price);

  String result = "";
  int count = 0;
  for (int i = priceStr.length() - 1; i >= 0; i--) {
    result = priceStr[i] + result;
    count++;
    if (count % 3 == 0 && i != 0) {
      result = "." + result;
    }
  }

  return result;
}