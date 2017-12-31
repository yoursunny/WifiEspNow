#ifdef ESP8266
#include "WifiEspNowBroadcast.h"

#include <ESP8266WiFi.h>

#include <c_types.h>
#include <espnow.h>
#include <user_interface.h>

WifiEspNowBroadcastClass WifiEspNowBroadcast;

WifiEspNowBroadcastClass::WifiEspNowBroadcastClass()
  : m_isScanning(false)
{
}

bool
WifiEspNowBroadcastClass::begin(const char* ssid, int channel, int scanFreq)
{
  m_ssid = ssid;
  m_nextScan = 0;
  m_scanFreq = scanFreq;

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, nullptr, channel);
  WiFi.disconnect();

  WifiEspNow.begin();
}

void
WifiEspNowBroadcastClass::loop()
{
  if (millis() >= m_nextScan && !m_isScanning) {
    this->scan();
  }
}

void
WifiEspNowBroadcastClass::end()
{
  WifiEspNow.end();
  WiFi.softAPdisconnect();
  m_ssid = "";
}

bool
WifiEspNowBroadcastClass::send(const uint8_t* buf, size_t count)
{
  return WifiEspNow.send(nullptr, buf, count);
}

void
WifiEspNowBroadcastClass::scan()
{
  m_isScanning = true;
  scan_config sc { .ssid = reinterpret_cast<uint8*>(m_ssid.begin()) };
  wifi_station_scan(&sc, reinterpret_cast<scan_done_cb_t>(WifiEspNowBroadcastClass::processScan));
}

void
WifiEspNowBroadcastClass::processScan(void* result, int status)
{
  WifiEspNowBroadcast.m_isScanning = false;
  WifiEspNowBroadcast.m_nextScan = millis() + WifiEspNowBroadcast.m_scanFreq;
  if (status != 0) {
    return;
  }

  const int MAX_PEERS = 20;
  uint8_t oldPeers[6][MAX_PEERS];
  int nOldPeers = 0;
  for (u8* peer = esp_now_fetch_peer(true);
       peer != nullptr && nOldPeers < MAX_PEERS;
       peer = esp_now_fetch_peer(false)) {
    memcpy(oldPeers[nOldPeers++], peer, 6);
  }

  uint8_t* newPeer = nullptr; // every scan adds at most one new peer
  int newPeerChannel;
  for (bss_info* it = reinterpret_cast<bss_info*>(result); it; it = STAILQ_NEXT(it, next)) {
    if (it->ssid_len != WifiEspNowBroadcast.m_ssid.length() ||
        memcmp(it->ssid, WifiEspNowBroadcast.m_ssid.c_str(), it->ssid_len) != 0) {
      continue;
    }

    bool isOldPeer = false;
    for (int i = 0; i < nOldPeers; ++i) {
      if (memcmp(it->bssid, oldPeers[i], 6) == 0) {
        isOldPeer = true;
        memset(oldPeers[i], 0, 6);
        break;
      }
    }

    if (!isOldPeer) {
      newPeer = it->bssid;
      newPeerChannel = it->channel;
    }
  }

  for (int i = 0; i < nOldPeers; ++i) {
    uint8_t* peer = oldPeers[i];
    if ((peer[0] | peer[1] | peer[2] | peer[3] | peer[4] | peer[5]) != 0) {
      WifiEspNow.removePeer(peer);
    }
  }

  if (newPeer != nullptr) {
    WifiEspNow.addPeer(newPeer, newPeerChannel);
  }
}
#endif
