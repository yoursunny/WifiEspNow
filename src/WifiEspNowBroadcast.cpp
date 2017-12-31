#ifdef ESP8266
#include "WifiEspNowBroadcast.h"

#include <ESP8266WiFi.h>
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
  WifiEspNowPeerInfo oldPeers[MAX_PEERS];
  int nOldPeers = std::min(WifiEspNow.listPeers(oldPeers, MAX_PEERS), MAX_PEERS);
  const uint8_t PEER_FOUND = 0xFF; // assigned to .channel to indicate peer is matched

  for (bss_info* it = reinterpret_cast<bss_info*>(result); it; it = STAILQ_NEXT(it, next)) {
    for (int i = 0; i < nOldPeers; ++i) {
      if (memcmp(it->bssid, oldPeers[i].mac, 6) != 0) {
        continue;
      }
      oldPeers[i].channel = PEER_FOUND;
      break;
    }
  }

  for (int i = 0; i < nOldPeers; ++i) {
    if (oldPeers[i].channel != PEER_FOUND) {
      WifiEspNow.removePeer(oldPeers[i].mac);
    }
  }

  for (bss_info* it = reinterpret_cast<bss_info*>(result); it; it = STAILQ_NEXT(it, next)) {
    WifiEspNow.addPeer(it->bssid, it->channel);
  }
}
#endif
