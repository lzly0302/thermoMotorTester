//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#include ".\mde_mRtuSlave.h"
#include "..\..\pbc\pbc_dataConvert\pbc_dataConvert.h"
#include "..\..\pbc\pbc_crc16\pbc_crc16.h"
#include "..\..\pbc\pbc_sysTick\pbc_sysTick.h"
#include ".\depend\bsp_mRtuSlave.h"
//-----------------------------------------------------------------------------
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//modbus 从机模块,负责报文的接收和应答控制
//支持命令: 0x03 0x06 0x10 0x17
//支持多个实例
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
typedef enum
{
    mRunS_idle             = 0x00,
    mRunS_receive_wait,
    mRunS_receive_first,
    mRunS_receive_data,
    mRunS_receive_end,
    mRunS_transmit_str,
    mRunS_transmit_35T,
    mRunS_transmit_data,
    mRunS_transmit_stop,
    mRunS_transmit_end,

}modbus_runStatus_def;
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//modbus操作结构体
//-----------------------------------------------------------------------------
typedef struct
{
    mRtu_parameter_def  mRtu_parameter;
    mRtu_status_def     mRtu_status;
    modbus_runStatus_def  moo_runStutus;
    sdt_int8u  receive_buff[256];
    sdt_int8u  rev_index;
    sdt_int8u  transmit_buff[256];
    sdt_int16u transmit_regAddrStr;
    sdt_int8u  transmit_length;
    sdt_int8u  transmit_index;
    timerClock_def timer_revTimeOut;

    sdt_int16u  readReg_addr;
    sdt_int8u   readReg_length;
    sdt_int16u  writeReg_addr;
    sdt_int8u   writeReg_length;

    void (*phy_into_receive)(void);
    sdt_bool (*pull_receive_byte)(sdt_int8u* out_rByte);
    sdt_bool (*pull_busFree)(sdt_int8u t_char_dis);
    void (*restart_busFree_timer)(void);

    void (*phy_into_transmit_status)(void);
    sdt_bool (*push_transmit_byte)(sdt_int8u in_tByte);
    sdt_bool (*pull_transmit_complete)(void);


}modbus_oper_def;
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//name:potocol command
//out: true -- 协议解释完毕，false -- 协议解释失败
//-----------------------------------------------------------------------------
static sdt_bool modbus_receive_protocol(modbus_oper_def* mix_oper)
{
    sdt_int8u rd_length;

    rd_length = mix_oper->rev_index;
    if(rd_length < 6)
    {
        return(sdt_false);
    }
    else
    {
        if((mix_oper->mRtu_parameter.mRtu_address == mix_oper->receive_buff[0]) || (0xFE == mix_oper->receive_buff[0]))  //address
        {
            sdt_int8u crc_value[2];
            Crc16CalculateOfByte(&mix_oper->receive_buff[0],(rd_length-2),&crc_value[0]);
            if((crc_value[0] == mix_oper->receive_buff[rd_length-2]) && (crc_value[1] == mix_oper->receive_buff[rd_length-1]))//crc
            {
                if(0x03 == mix_oper->receive_buff[1])
                {
                    mix_oper->readReg_addr = pbc_arrayToInt16u_bigEndian(&mix_oper->receive_buff[2]);
                    mix_oper->readReg_length = pbc_arrayToInt16u_bigEndian(&mix_oper->receive_buff[4]);

                    mix_oper->transmit_buff[0] = mix_oper->mRtu_parameter.mRtu_address;
                    mix_oper->transmit_buff[1] = 0x03;
                    mix_oper->transmit_buff[2] = mix_oper->readReg_length * 2;//byte count
                    mix_oper->transmit_length = 3;

                    mix_oper->mRtu_status = mRtuS_read;
                }
                else if(0x06 == mix_oper->receive_buff[1])
                {
                    mix_oper->writeReg_addr = pbc_arrayToInt16u_bigEndian(&mix_oper->receive_buff[2]);
                    mix_oper->writeReg_length = 1;
                    mix_oper->transmit_buff[0] = mix_oper->mRtu_parameter.mRtu_address;
                    mix_oper->transmit_buff[1] = 0x06;
                    mix_oper->transmit_buff[2] = mix_oper->receive_buff[2];
                    mix_oper->transmit_buff[3] = mix_oper->receive_buff[3];
                    mix_oper->transmit_buff[4] = mix_oper->receive_buff[4];
                    mix_oper->transmit_buff[5] = mix_oper->receive_buff[5];
                    mix_oper->transmit_length = 6;

                    mix_oper->mRtu_status = mRtuS_write;
                }
                else if(0x10 == mix_oper->receive_buff[1])
                {
                    mix_oper->writeReg_addr = pbc_arrayToInt16u_bigEndian(&mix_oper->receive_buff[2]);
                    mix_oper->writeReg_length = pbc_arrayToInt16u_bigEndian(&mix_oper->receive_buff[4]);
                    mix_oper->transmit_buff[0] = mix_oper->mRtu_parameter.mRtu_address;
                    mix_oper->transmit_buff[1] = 0x10;
                    mix_oper->transmit_buff[2] = mix_oper->receive_buff[2];
                    mix_oper->transmit_buff[3] = mix_oper->receive_buff[3];
                    mix_oper->transmit_buff[4] = mix_oper->receive_buff[4];
                    mix_oper->transmit_buff[5] = mix_oper->receive_buff[5];
                    mix_oper->transmit_length = 6;

                    mix_oper->mRtu_status = mRtuS_write;
                }
                else if(0x17 == mix_oper->receive_buff[1])
                {
                    mix_oper->readReg_addr = pbc_arrayToInt16u_bigEndian(&mix_oper->receive_buff[2]);
                    mix_oper->readReg_length = pbc_arrayToInt16u_bigEndian(&mix_oper->receive_buff[4]);
                    mix_oper->writeReg_addr = pbc_arrayToInt16u_bigEndian(&mix_oper->receive_buff[6]);
                    mix_oper->writeReg_length = pbc_arrayToInt16u_bigEndian(&mix_oper->receive_buff[8]);
                    mix_oper->transmit_buff[0] = mix_oper->mRtu_parameter.mRtu_address;
                    mix_oper->transmit_buff[1] = 0x17;
                    mix_oper->transmit_buff[2] = mix_oper->readReg_length * 2;
                    mix_oper->transmit_length = 3;

                    mix_oper->mRtu_status = mRtuS_rwBoth;
                }
                else
                {
                    return(sdt_false);
                }
                return(sdt_true);
            }
        }        
    }
    return(sdt_false);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//name: 
//-----------------------------------------------------------------------------
static void modbus_operation_task(modbus_oper_def* mix_oper)
{
    pbc_timerMillRun_task(&mix_oper->timer_revTimeOut);
    switch(mix_oper->moo_runStutus)
    {
        case mRunS_idle:
        {
            mix_oper->moo_runStutus = mRunS_receive_wait;
            break;
        }
        case mRunS_receive_wait:
        {
            sdt_int8u receive_byte;
            if(mix_oper->pull_receive_byte(&receive_byte))
            {
                mix_oper->receive_buff[0] = receive_byte;
                mix_oper->rev_index = 1;
                mix_oper->moo_runStutus = mRunS_receive_data;
            }
            break;
        }
        case mRunS_receive_data:
        {
            while(1)
            {
                sdt_int8u receive_byte;
                if(mix_oper->pull_receive_byte(&receive_byte))
                {
                    mix_oper->receive_buff[mix_oper->rev_index] = receive_byte;
                    mix_oper->rev_index ++;
                    mix_oper->restart_busFree_timer();
                }
                else
                {
                    if(mix_oper->pull_busFree(25))  //标准3.5T,检测2.5T视为报文完成
                    {
                        if(modbus_receive_protocol(mix_oper))
                        {
                            pbc_reload_timerClock(&mix_oper->timer_revTimeOut,1500);
                            mix_oper->moo_runStutus = mRunS_receive_end;
                        }
                        else
                        {
                            mix_oper->moo_runStutus = mRunS_receive_wait;
                        }
                    }
                    break; 
                }
            }
            break;
        }
        case mRunS_receive_end:
        {
            if(pbc_pull_timerIsCompleted(&mix_oper->timer_revTimeOut))
            {
                mix_oper->moo_runStutus = mRunS_receive_wait;
            }
            break;
        }
        case mRunS_transmit_str:
        {
            mix_oper->phy_into_transmit_status();
            mix_oper->restart_busFree_timer();
            mix_oper->moo_runStutus = mRunS_transmit_35T;
            break;
        }
        case mRunS_transmit_35T:
        {
            if(mix_oper->pull_busFree(50))  //5T
            {
                mix_oper->moo_runStutus = mRunS_transmit_data;
                mix_oper->transmit_index = 0;
            } 
            break;
        }
        case mRunS_transmit_data:
        {
            if(mix_oper->transmit_length != 0)
            {
                sdt_bool push_succeed;
                while(1)
                {
                    push_succeed = mix_oper->push_transmit_byte(mix_oper->transmit_buff[mix_oper->transmit_index]);
                    if(push_succeed)
                    {
                        mix_oper->transmit_index ++;
                        mix_oper->transmit_length --;
                        if(0 == mix_oper->transmit_length)
                        {
                            mix_oper->moo_runStutus = mRunS_transmit_stop;
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            break;
        }
        case mRunS_transmit_stop:
        {
            if(mix_oper->pull_transmit_complete())
            {
                mix_oper->restart_busFree_timer();
                mix_oper->moo_runStutus = mRunS_transmit_end;
            }
            break;
        }
        case mRunS_transmit_end:
        {
            if(mix_oper->pull_busFree(50)) 
            {
                mix_oper->phy_into_receive();
                mix_oper->moo_runStutus = mRunS_receive_wait;
            }
            break;
        }
        default:
        {
            mix_oper->moo_runStutus = mRunS_idle;
            break;
        }
    }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//name:写入一个寄存器的数据到缓存
//-----------------------------------------------------------------------------
static sdt_bool make_readReg_buff(modbus_oper_def* mix_oper,sdt_int16u in_reg_addr,sdt_int16u in_regDetails)
{
    if(in_reg_addr < (mix_oper->readReg_addr))
    {
        return(sdt_false);
    }
    else
    {
        sdt_int8u buff_index;
        if(0x03 == mix_oper->transmit_buff[1])
        {
            buff_index = 3;
        }
        else if (0x17 == mix_oper->transmit_buff[1])
        {
            buff_index = 3;
        }
        else
        {
            while(1);
        }
        
        sdt_int16u dis_r = in_reg_addr - (mix_oper->readReg_addr);
        while(dis_r)
        {
            buff_index = buff_index + 2;
            dis_r = dis_r-1;
        }
        mix_oper->transmit_length += 2;
        mix_oper->transmit_buff[buff_index] = in_regDetails >> 8;
        mix_oper->transmit_buff[buff_index+1] = in_regDetails;
        return(sdt_true);
    }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//name:获取写寄存的的内容
//-----------------------------------------------------------------------------
static sdt_bool get_writeReg_buff(modbus_oper_def* mix_oper,sdt_int16u in_reg_addr,sdt_int16u* out_regDetails)
{
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static void enable_answer_message(modbus_oper_def* mix_oper)
{
    if(mix_oper->transmit_length >253)
    {
        mix_oper->moo_runStutus = mRunS_idle; //overflow
    }
    else
    {
        Crc16CalculateOfByte(&mix_oper->transmit_buff[0],(sdt_int16u)mix_oper->transmit_length,&mix_oper->transmit_buff[mix_oper->transmit_length]);//crc
        mix_oper->transmit_length += 2;
        mix_oper->moo_runStutus = mRunS_transmit_str;       
    }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
//+++++++++++++++++++++++++++++++solid+++++++++++++++++++++++++++++++++++++++++
//-----------------------------------------------------------------------------
static modbus_oper_def modbus_oper_one;
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static void modbus_solid_cfg(void)
{
    bsp_uart4_cfg();
    modbus_oper_one.pull_receive_byte = bsp_pull_oneByte_uart4_rxd;
    modbus_oper_one.push_transmit_byte = bsp_push_oneByte_uart4_txd;
    modbus_oper_one.pull_busFree = bsp_uart4_busFree;
    modbus_oper_one.restart_busFree_timer = bsp_restart_tim3;
    modbus_oper_one.phy_into_receive = bps_uart4_into_receive;
    modbus_oper_one.phy_into_transmit_status = bps_uart4_into_transmit;
    modbus_oper_one.pull_transmit_complete =bsp_pull_uart4_txd_cmp;
    
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++interface+++++++++++++++++++++++++++++++++++
//name:mRtu任务
//-----------------------------------------------------------------------------
void mde_mRtu_task(void)
{
    static sdt_bool cfged = sdt_false;

    if(cfged)
    {
        modbus_operation_task(&modbus_oper_one);
    }
    else
    {
        cfged = sdt_true;
        modbus_solid_cfg();
    }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//name:设置modbus参数
//-----------------------------------------------------------------------------
void set_mRtu_parameter(sdt_int8u in_solidNum,mRtu_parameter_def in_parameter)
{
    if(0 == in_solidNum)
    {

    }
    else
    {
        while(1);
    }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//name:获取modbus register 当前状态
//in: *out_reg_addr 地址指针，*out_length 长度指针
//out: mRtuS_none 无,mRtuS_read 读数据,mRtuS_write 写数据
//-----------------------------------------------------------------------------
mRtu_status_def pull_mRtu_register(sdt_int8u in_solidNum,sdt_int16u* out_reg_addr,sdt_int16u* out_length)
{
    if(0 == in_solidNum)
    {
        mRtu_status_def rd_status;

        rd_status = modbus_oper_one.mRtu_status;
        modbus_oper_one.mRtu_status = mRtuS_none;
        if((mRtuS_read == rd_status) || (mRtuS_rwBoth == rd_status))
        {
            *out_reg_addr = modbus_oper_one.readReg_addr;
            *out_length = modbus_oper_one.readReg_length;
        }
        else if(mRtuS_write == rd_status)
        {
            *out_reg_addr = modbus_oper_one.writeReg_addr;
            *out_length = modbus_oper_one.writeReg_length;
        }
        return(rd_status);
    }
    else
    {
        while(1);
    }
}
//-----------------------------------------------------------------------------
//name:获取写入的寄存器地址
//-----------------------------------------------------------------------------
void pull_mRtu_register_wb(sdt_int8u in_solidNum,sdt_int16u* out_reg_addr,sdt_int16u* out_length)
{
    if(0 == in_solidNum)
    {
        *out_reg_addr = modbus_oper_one.writeReg_addr;
        *out_length = modbus_oper_one.writeReg_length;
    }
    else
    {
        while(1);
    }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//name:modbus读取命令
//fun:把modbus需要读取的数据，推入modbus模块
//in: in_reg_addr 寄存器地址，in_RegDetails 寄存器内容
//out: sdt_true 推入成功,sdt_false 推入失败
//-----------------------------------------------------------------------------
sdt_bool push_mRtu_readReg(sdt_int8u in_solidNum,sdt_int16u in_reg_addr,sdt_int16u in_RegDetails)
{
    sdt_bool complete = sdt_false;

    if(0 == in_solidNum)
    {
        if(make_readReg_buff(&modbus_oper_one,in_reg_addr,in_RegDetails))
        {
            complete = sdt_true;
        }
        else
        {
            while(1);
        } 
    }
    else
    {
        while(1);
    }

    return(complete);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//name:modbus写入命令
//fun:从模块中获取需要写入的寄存器内容
//in: in_reg_addr 寄存器地址，in_length 长度，*out_pRregDetails 寄存器内容指针
//out: sdt_true 获取成功,sdt_false 获取失败
//-----------------------------------------------------------------------------
sdt_bool pull_mRtu_writeReg(sdt_int8u in_solidNum,sdt_int16u in_reg_addr,sdt_int16u* out_pRregDetails)
{
    sdt_bool complete = sdt_false;

    if(0 == in_solidNum)
    {

    }
    else
    {
        while(1);
    }

    return(complete);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//name:应答事件
//fun:modbus总线开始应答
//-----------------------------------------------------------------------------
void mRtu_answer_event(sdt_int8u in_solidNum)
{
    if(0 == in_solidNum)
    {
        enable_answer_message(&modbus_oper_one);
    }
    else
    {
        while(1);
    }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//-----------------------------------------------------------------------------
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++