//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef bsp_mRtuSlave_H
#define bsp_mRtuSlave_H
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//-----------------------------------------------------------------------------
void bsp_uart4_cfg(void);
sdt_bool bsp_pull_oneByte_uart4_rxd(sdt_int8u* out_byte_details);
sdt_bool bsp_push_oneByte_uart4_txd(sdt_int8u in_byte_details);
sdt_bool bsp_uart4_busFree(sdt_int8u t_char_dis);
void bsp_restart_tim3(void);
sdt_bool bsp_pull_uart4_txd_cmp(void);
void bps_uart4_into_receive(void);
void bps_uart4_into_transmit(void);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#endif 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++