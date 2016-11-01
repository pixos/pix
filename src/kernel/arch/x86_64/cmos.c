/*_
 * Copyright (c) 2016 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <aos/const.h>
#include "arch.h"
#include "cmos.h"

/* Prototype declarations */
static int _is_updating(void);
static u8 _read_rtc_reg(u8);

/*
 * Check if the CMOS is updating (in progress)
 */
static int
_is_updating(void)
{
    /* Read status register A */
    outb(CMOS_ADDR, 0x0a);

    return inb(CMOS_DATA) & 0x80;
}

/*
 * Read the real-time clock register
 */
static u8
_read_rtc_reg(u8 reg)
{
    outb(CMOS_ADDR, reg);

    return inb(CMOS_DATA);
}

/*
 * Date and time to unix timestamp (seconds since January 1, 1970)
 */
static u64
_datetime_to_unixtime(u32 year, u8 month, u8 day, u8 hour, u8 min, u8 sec)
{
    u64 unixtime;
    u64 days;

    unixtime = 0;

    /* Year */
    days = 365 * (year - 1970);

    /* Leap year */
    days += ((year - 1) / 4) - 1969 / 4;
    days -= ((year - 1) / 100) - 1969 / 100;
    days += ((year - 1) / 400) - 1969 / 400;

    /* Month */
    switch ( month ) {
    case 12:
        days += 30;
    case 11:
        days += 31;
    case 10:
        days += 30;
    case 9:
        days += 31;
    case 8:
        days += 31;
    case 7:
        days += 30;
    case 6:
        days += 31;
    case 5:
        days += 30;
    case 4:
        days += 31;
    case 3:
        days += 28;
        /* Check leap years */
        if ( (0 == (year % 4) && 0 != (year % 100)) || 0 == (year % 400) ) {
            /* Leap years */
            days += 1;
        }
    case 2:
        days += 31;
    case 1:
        ;
    }

    /* Day */
    days += day - 1;

    unixtime = (((days * 24 + hour) * 60 + min)) * 60 + sec;

    return unixtime;
}


/*
 * Read date & time from CMOS RTC in Unix timestamp
 */
u64
cmos_rtc_read_datetime(u8 century_reg)
{
    u8 sec;
    u8 min;
    u8 hour;
    u8 day;
    u8 month;
    u8 year;
    u8 century;
    u8 last_sec;
    u8 last_min;
    u8 last_hour;
    u8 last_day;
    u8 last_month;
    u8 last_year;
    u8 last_century;
    u8 regb;

    /* Wait until the CMOS is ready for use */
    while ( _is_updating() );

    sec = _read_rtc_reg(0x00);
    min = _read_rtc_reg(0x02);
    hour = _read_rtc_reg(0x04);
    day = _read_rtc_reg(0x07);
    month = _read_rtc_reg(0x08);
    year = _read_rtc_reg(0x09);
    if ( century_reg ) {
        century = _read_rtc_reg(0x09);
    } else {
        century = 0;
    }

    /* Try until no overflow/carry */
    do {
        last_sec = sec;
        last_min = min;
        last_hour = hour;
        last_day = day;
        last_month = month;
        last_year = year;
        last_century = century;

        while ( _is_updating() );

        sec = _read_rtc_reg(0x00);
        min = _read_rtc_reg(0x02);
        hour = _read_rtc_reg(0x04);
        day = _read_rtc_reg(0x07);
        month = _read_rtc_reg(0x08);
        year = _read_rtc_reg(0x09);
        if ( century_reg ) {
            century = _read_rtc_reg(century_reg);
        } else {
            century = 0;
        }
    } while ( last_sec != sec || last_min != min || last_hour != hour
              || last_day != day || last_month != month || last_year != year
              || last_century != century );

    /* Read status register B */
    regb = _read_rtc_reg(0x0b);

    if ( !(regb & 0x04) ) {
        /* Convert BCD to binary values if necessary */
        sec = (sec & 0x0f) + (sec >> 4) * 10;
        min = (min & 0x0f) + (min >> 4) * 10;
        day = (day & 0x0f) + (day >> 4) * 10;
        month = (month & 0x0f) + (month >> 4) * 10;
        year = (year & 0x0f) + (year >> 4) * 10;
        if ( century_reg ) {
            century = (century & 0x0f) + (century >> 4) * 10;
        }

        if ( regb & 0x02 ) {
            /* 12-hour format to 24-hour format */
            if ( hour & 0x80 ) {
                /* PM */
                hour = hour & 0x7f;
                hour = (hour & 0x0f) + (hour >> 4) * 10;
                hour = (hour + 12) % 24;
            } else {
                hour = (hour & 0x0f) + (hour >> 4) * 10;
            }
        } else {
            hour = (hour & 0x0f) + (hour >> 4) * 10;
        }
    } else {
        if ( regb & 0x02 ) {
            /* 12-hour format to 24-hour format */
            if ( hour & 0x80 ) {
                /* PM */
                hour = ((hour & 0x7f) + 12) % 24;
            }
        }
    }

    if ( year >= MIN_YEAR % 100 ) {
        century = MIN_YEAR / 100;
    } else {
        century = MIN_YEAR / 100 + 1;
    }

    /* Return in the unix timestamp format */
    return _datetime_to_unixtime((u32)century * 100 + (u32)year,
                                 month, day, hour, min, sec);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
