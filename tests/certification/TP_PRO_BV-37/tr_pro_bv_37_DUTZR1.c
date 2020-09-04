/***************************************************************************
*                                                                          *
* INSERT COPYRIGHT HERE!                                                   *
*                                                                          *
****************************************************************************
PURPOSE: TP/PRO/BV-37 Frequency Agility � Receipt of unsolicited
Mgmt_NWK_Update_notify, router side.
*/

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_test_profile.h"

/* For NS build first ieee addr byte should be unique */
#ifdef ZB_NS_BUILD
zb_ieee_addr_t g_ieee_addr_1 = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_2 = {0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
#else
zb_ieee_addr_t g_ieee_addr_1 = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_2 = {0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
#endif

#define TEST_PACKET_LENGTH 60
#define TEST_PACKET_COUNT 21
#define TEST_PACKET_DELAY 100 /* ms */


MAIN()
{
  ARGV_UNUSED;

#ifndef KEIL
  if ( argc < 3 )
  {
    printf("%s <read pipe path> <write pipe path>\n", argv[0]);
    return 0;
  }
#endif

  /* Init device, load IB values from nvram or set it to default */
#ifndef ZB8051
  ZB_INIT("zdo_zr1_", argv[1], argv[2]);
#else
  ZB_INIT("zdo_zr1_", "2", "2");
#endif

  /* set ieee addr */
  ZB_IEEE_ADDR_COPY(ZB_PIB_EXTENDED_ADDRESS(), &g_ieee_addr_1);

  /* join as a router */
  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;

#ifdef ZB_SECURITY
  /* turn off security */
  ZB_NIB_SECURITY_LEVEL() = 0;
#endif

  ZB_NWK().max_children = 0;
  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


void packets_sent_cb(zb_uint8_t param) ZB_CALLBACK
{
  //zb_buf_t *buf = ZB_BUF_FROM_PARAM(param);

  ZVUNUSED(param);

  TRACE_MSG(TRACE_APS3, "packets_sent_cb", (FMT__0));

}

void start_packet_send(zb_uint8_t param) ZB_CALLBACK
{
  zb_tp_transmit_counted_packets_param_t *params;
  zb_buf_t *asdu;

  ZVUNUSED(param);

  TRACE_MSG(TRACE_APS3, "start_packet_send", (FMT__0));

  asdu = zb_get_out_buf();
  if (!asdu)
  {
    TRACE_MSG(TRACE_ERROR, "out buf alloc failed!", (FMT__0));
  }
  else
  {
    params = ZB_GET_BUF_PARAM(asdu, zb_tp_transmit_counted_packets_param_t);
    params->len = TEST_PACKET_LENGTH;
    params->packets_number = TEST_PACKET_COUNT;
    params->idle_time = TEST_PACKET_DELAY;
    params->dst_addr = zb_address_short_by_ieee(g_ieee_addr_2);

    TRACE_MSG(TRACE_APS3, "dst addr %d", (FMT__D, params->dst_addr));
    zb_tp_transmit_counted_packets_req(ZB_REF_FROM_BUF(asdu), packets_sent_cb);
  }
}

void zb_zdo_startup_complete(zb_uint8_t param) ZB_CALLBACK
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  if (buf->u.hdr.status == 0)
  {
    TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
    ZB_MAC_SET_CHANNEL_ERROR_TEST();
    zb_schedule_alarm(start_packet_send, 0, ZB_TIME_ONE_SECOND * 30);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, (int)buf->u.hdr.status));
    zb_free_buf(buf);
  }
}
