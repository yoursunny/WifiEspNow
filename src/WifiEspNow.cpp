#include "WifiEspNow.h"

#if defined(ESP8266)
#include <c_types.h>
#include <espnow.h>
#elif defined(ESP32)
#include <string.h>
#include <esp_now.h>
#else
#error "This library supports ESP8266 and ESP32 only."
#endif

WifiEspNowClass WifiEspNow;

WifiEspNowClass::WifiEspNowClass()
  : m_rxCb(nullptr)
  , m_rxCbArg(nullptr)
{
}

bool
WifiEspNowClass::begin()
{
  return esp_now_init() == 0 &&
#ifdef ESP8266
         esp_now_set_self_role(ESP_NOW_ROLE_COMBO) == 0 &&
#endif
         esp_now_register_recv_cb(reinterpret_cast<esp_now_recv_cb_t>(WifiEspNowClass::rx)) == 0 &&
         esp_now_register_send_cb(reinterpret_cast<esp_now_send_cb_t>(WifiEspNowClass::tx)) == 0;
}

void
WifiEspNowClass::end()
{
  esp_now_deinit();
}

bool
WifiEspNowClass::addPeer(const uint8_t mac[6], int channel, const uint8_t key[WIFIESPNOW_KEYLEN])
{
#if defined(ESP8266)
  return esp_now_add_peer(const_cast<u8*>(mac), ESP_NOW_ROLE_SLAVE, static_cast<u8>(channel),
                          const_cast<u8*>(key), key == nullptr ? 0 : WIFIESPNOW_KEYLEN) == 0;
#elif defined(ESP32)
  esp_now_peer_info_t pi;
  memset(&pi, 0, sizeof(pi));
  memcpy(pi.peer_addr, mac, ESP_NOW_ETH_ALEN);
  pi.channel = static_cast<uint8_t>(channel);
  pi.ifidx = ESP_IF_WIFI_AP;  
  return esp_now_add_peer(&pi) == 0;
#endif
}

bool
WifiEspNowClass::removePeer(const uint8_t mac[6])
{
  return esp_now_del_peer(const_cast<uint8_t*>(mac)) == 0;
}

void
WifiEspNowClass::onReceive(RxCallback cb, void* cbarg)
{
  m_rxCb = cb;
  m_rxCbArg = cbarg;
}

bool
WifiEspNowClass::send(const uint8_t mac[6], const uint8_t* buf, size_t count)
{
  if (count > WIFIESPNOW_MAXMSGLEN || count == 0) {
    return false;
  }
  WifiEspNow.m_txRes = WifiEspNowSendStatus::NONE;
  return esp_now_send(const_cast<uint8_t*>(mac), const_cast<uint8_t*>(buf), static_cast<int>(count)) == 0;
}

void
WifiEspNowClass::rx(const uint8_t* mac, const uint8_t* data, uint8_t len)
{
  if (WifiEspNow.m_rxCb != nullptr) {
    (*WifiEspNow.m_rxCb)(mac, data, len, WifiEspNow.m_rxCbArg);
  }
}

void
WifiEspNowClass::tx(const uint8_t* mac, uint8_t status)
{
  WifiEspNow.m_txRes = status == 0 ? WifiEspNowSendStatus::OK : WifiEspNowSendStatus::FAIL;
}
