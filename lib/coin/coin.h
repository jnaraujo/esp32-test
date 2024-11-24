#include <HTTPClient.h>

#ifndef COIN_H
#define COIN_H

struct CoinData {
  float price;
  tm updatedTime;
};

CoinData getCoinData(String c1, String c2);
#endif