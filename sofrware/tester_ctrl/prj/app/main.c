//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#include ".\app_cfg.h"
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int main (void)
{
    pbc_createTask_one(app_general_task,0);
    pbc_createTask_one(app_modbus_task,0);
//-----------------------------------------------------------------------------
    pbc_sysTickTaskProcess();
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++