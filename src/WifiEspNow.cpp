#include "WifiEspNow.h"

#include <c_types.h>
#include <espnow.h>

WifiEspNowClass WifiEspNow;

WifiEspNowClass::WifiEspNowClass()
  : m_rxCb(nullptr)
  , m_rxCbArg(nullptr)
{
}

bool
WifiEspNowClass::begin(WifiEspNowRole role)
{
  return esp_now_init() == 0 &&
         esp_now_register_recv_cb(WifiEspNowClass::rx) == 0 &&
         esp_now_register_send_cb(WifiEspNowClass::tx) == 0 &&
         esp_now_set_self_role(static_cast<u8>(role)) == 0;
}

void
WifiEspNowClass::end()
{
  esp_now_deinit();
}

bool
WifiEspNowClass::addPeer(const uint8_t mac[6], WifiEspNowRole role, int channel,
                         const uint8_t key[WIFIESPNOW_KEYLEN])
{
  return esp_now_add_peer(const_cast<u8*>(mac), static_cast<u8>(role), static_cast<u8>(channel),
                          const_cast<u8*>(key), key == nullptr ? 0 : WIFIESPNOW_KEYLEN) == 0;
}

bool
WifiEspNowClass::removePeer(const uint8_t mac[6])
{
  return esp_now_del_peer(const_cast<u8*>(mac)) == 0;
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
  return esp_now_send(const_cast<u8*>(mac), const_cast<u8*>(buf), static_cast<int>(count)) == 0;
}

void
WifiEspNowClass::rx(u8* mac, u8* data, u8 len)
{
  if (WifiEspNow.m_rxCb != nullptr) {
    (*WifiEspNow.m_rxCb)(mac, data, len, WifiEspNow.m_rxCbArg);
  }
}

void
WifiEspNowClass::tx(u8* mac, u8 status)
{
  WifiEspNow.m_txRes = status == 0 ? WifiEspNowSendStatus::OK : WifiEspNowSendStatus::FAIL;
}
