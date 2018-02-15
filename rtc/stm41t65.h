#ifndef __M41T65_H_
#define __M41T65_H_

#define M41T65_DEVICE "stm-m41t65"

#define M41T65_REG_SSEC 0
#define M41T65_REG_SEC  1
#define M41T65_REG_MIN  2
#define M41T65_REG_HOUR 3
#define M41T65_REG_WDAY 4
#define M41T65_REG_DAY  5
#define M41T65_REG_MON  6
#define M41T65_REG_YEAR 7
#define M41T65_REG_ALARM_MON    0xa
#define M41T65_REG_ALARM_DAY    0xb
#define M41T65_REG_ALARM_HOUR   0xc
#define M41T65_REG_ALARM_MIN    0xd
#define M41T65_REG_ALARM_SEC    0xe
#define M41T65_REG_FLAGS    0xf
#define M41T65_REG_SQW  0x13

#define M41T65_DATETIME_REG_SIZE  (M41T65_REG_YEAR+1)

#endif
