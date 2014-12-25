 /*
 *  Copyright (c) 2010 - 2011 Espressif System
 *
 */

#ifndef _TIMER_REGISTER_H_ 
#define _TIMER_REGISTER_H_ 

#define PERIPHS_TIMER_BASEDDR       0x60000600
 
#define FRC1_LOAD_ADDRESS           (PERIPHS_TIMER_BASEDDR + 0x0)
#define TIMER_FRC1_LOAD_VALUE           0x007FFFFF
#define TIMER_FRC1_LOAD_VALUE_S         0
#define FRC1_LOAD_DATA_MSB              22
#define FRC1_LOAD_DATA_LSB              0
#define FRC1_LOAD_DATA_MASK             0x007fffff

#define FRC1_COUNT_ADDRESS          (PERIPHS_TIMER_BASEDDR + 0x4)
#define TIMER_FRC1_COUNT                0x007FFFFF
#define TIMER_FRC1_COUNT_S              0
#define FRC1_COUNT_DATA_MSB             22
#define FRC1_COUNT_DATA_LSB             0
#define FRC1_COUNT_DATA_MASK            0x007fffff

#define FRC1_CTRL_ADDRESS           (PERIPHS_TIMER_BASEDDR + 0x8)
#define TIMER_FRC1_INT                  (BIT(8))
#define TIMER_FRC1_CTRL                 0x000000FF
#define TIMER_FRC1_CTRL_S               0
#define FRC1_CTRL_DATA_MSB              7
#define FRC1_CTRL_DATA_LSB              0
#define FRC1_CTRL_DATA_MASK             0x000000ff

#define FRC1_INT_ADDRESS            (PERIPHS_TIMER_BASEDDR + 0xC)
#define TIMER_FRC1_INT_CLR_MASK         (BIT(0))
#define FRC1_INT_CLR_MSB                0
#define FRC1_INT_CLR_LSB                0
#define FRC1_INT_CLR_MASK               0x00000001

#define FRC2_LOAD_ADDRESS           (PERIPHS_TIMER_BASEDDR + 0x20)
#define TIMER_FRC2_LOAD_VALUE           0xFFFFFFFF
#define TIMER_FRC2_LOAD_VALUE_S         0
#define FRC2_LOAD_DATA_MSB              31
#define FRC2_LOAD_DATA_LSB              0
#define FRC2_LOAD_DATA_MASK             0xffffffff

#define FRC2_COUNT_ADDRESS          (PERIPHS_TIMER_BASEDDR + 0x24)
#define TIMER_FRC2_COUNT                0xFFFFFFFF
#define TIMER_FRC2_COUNT_S              0
#define FRC2_COUNT_DATA_MSB             31
#define FRC2_COUNT_DATA_LSB             0
#define FRC2_COUNT_DATA_MASK            0xffffffff

#define FRC2_CTRL_ADDRESS           (PERIPHS_TIMER_BASEDDR + 0x28)
#define TIMER_FRC2_INT                  (BIT(8))
#define TIMER_FRC2_CTRL                 0x000000FF
#define TIMER_FRC2_CTRL_S               0
#define FRC2_CTRL_DATA_MSB              7
#define FRC2_CTRL_DATA_LSB              0
#define FRC2_CTRL_DATA_MASK             0x000000ff

#define FRC2_INT_ADDRESS            (PERIPHS_TIMER_BASEDDR + 0x2C)
#define TIMER_FRC2_INT_CLR_MASK         (BIT(0))
#define FRC2_INT_CLR_MSB                0
#define FRC2_INT_CLR_LSB                0
#define FRC2_INT_CLR_MASK               0x00000001

#define FRC2_ALARM_ADDRESS          (PERIPHS_TIMER_BASEDDR + 0x30)
#define TIMER_FRC2_ALARM                0xFFFFFFFF
#define TIMER_FRC2_ALARM_S              0
#define FRC2_ALARM_DATA_MSB             31
#define FRC2_ALARM_DATA_LSB             0
#define FRC2_ALARM_DATA_MASK            0xffffffff

#endif
