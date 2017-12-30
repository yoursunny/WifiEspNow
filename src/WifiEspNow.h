#ifndef WIFIESPNOW_H
#define WIFIESPNOW_H

#include <cstddef>
#include <cstdint>

/** \brief Key length.
 */
static const int WIFIESPNOW_KEYLEN = 16;

/** \brief Maximum message length.
 */
static const int WIFIESPNOW_MAXMSGLEN = 250;

/** \brief ESP-NOW role.
 */
enum class WifiEspNowRole : uint8_t {
  IDLE       = 0,
  CONTROLLER = 1,
  SLAVE      = 2,
  COMBO      = 3,
};

/** \brief Result of send operation.
 */
enum class WifiEspNowSendStatus : uint8_t {
  NONE = 0, ///< result unknonw, send in progress
  OK   = 1, ///< sent successfully
  FAIL = 2, ///< sending failed
};

class WifiEspNowClass
{
public:
  WifiEspNowClass();

  /** \brief Initialize ESP-NOW.
   *  \param role ESP-NOW role of this node.
   *  \return whether success
   */
  bool
  begin(WifiEspNowRole role = WifiEspNowRole::COMBO);

  /** \brief Stop ESP-NOW.
   */
  void
  end();

  /** \brief Add a peer.
   *  \param mac peer MAC address
   *  \param role peer role
   *  \param channel peer channel, 0 for current channel
   *  \param key encryption key, nullptr to disable encryption
   *  \return whether success
   */
  bool
  addPeer(const uint8_t mac[6], WifiEspNowRole role = WifiEspNowRole::SLAVE,
          int channel = 0, const uint8_t key[WIFIESPNOW_KEYLEN] = nullptr);

  /** \brief Remove a peer.
   *  \param mac peer MAC address
   *  \return whether success
   */
  bool
  removePeer(const uint8_t mac[6]);

  typedef void (*RxCallback)(const uint8_t mac[6], const uint8_t* buf, size_t count, void* cbarg);

  /** \brief Set receive callback.
   *  \param cb the callback
   *  \param cbarg an arbitrary argument passed to the callback
   *  \note Only one callback is allowed; this replaces any previous callback.
   */
  void
  onReceive(RxCallback cb, void* cbarg);

  /** \brief Send a message.
   *  \param mac destination MAC address, nullptr for all peers
   *  \param buf payload
   *  \param count payload size, must not exceed \p WIFIESPNOW_MAXMSGLEN
   *  \return whether success (message queued for transmission)
   */
  bool
  send(const uint8_t mac[6], const uint8_t* buf, size_t count);

  /** \brief Retrieve status of last sent message.
   *  \return whether success (unicast message received by peer, multicast message sent)
   */
  WifiEspNowSendStatus
  getSendStatus() const
  {
    return m_txRes;
  }

private:
  static void
  rx(uint8_t* mac, uint8_t* data, uint8_t len);

  static void
  tx(uint8_t* mac, uint8_t status);

private:
  RxCallback m_rxCb;
  void* m_rxCbArg;
  WifiEspNowSendStatus m_txRes;
};

extern WifiEspNowClass WifiEspNow;

#endif // WIFIESPNOW_H
