#ifndef app_grating_H
#define app_grating_H
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void app_grating_task(void);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//name:获取增量位置
//fun:获取相对上传的位置变动值
//-----------------------------------------------------------------------------
sdt_int32s app_pull_increment_um(void);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//name:获取绝对位置
//fun:获取相对绝对零点的测量值
//out: sdt_false 没有复位零点
//-----------------------------------------------------------------------------
sdt_bool app_pull_site_um(sdt_int32s* out_site_um);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#endif