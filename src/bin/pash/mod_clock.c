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

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "pash.h"

static int
_unixtime_to_datetime(uint64_t val, struct tm *tm)
{
    int year;
    int leap;

    /* Seconds */
    tm->tm_sec = val % 60;
    val /= 60;
    /* Minutes */
    tm->tm_min = val % 60;
    val /= 60;
    /* Hours */
    tm->tm_hour = val % 24;
    val /= 24;

    /* Calculate days since Year 0 */
    val += 1969 * 365 + 1969 / 4 - 1969 / 100 + 1969 / 400;

    /* Calculate year (assuming all years are leap year) */
    year = val / 366;
    val = val % 366;

    /* Add days of common years */
    val += year - year / 4 + year / 100 - year / 400;

    for ( ;; ) {
        if ( (0 == (year % 4) && 0 != (year % 100)) || 0 == (year % 400) ) {
            /* Leap year */
            leap = 1;
        } else {
            /* Common year */
            leap = 0;
        }
        if ( (int)val >= 365 + leap ) {
            year++;
            val -= 365 + leap;
        } else {
            break;
        }
    }

    year++;
    tm->tm_year = year;

    if ( (0 == (year % 4) && 0 != (year % 100)) || 0 == (year % 400) ) {
        /* Leap year */
        leap = 1;
    } else {
        /* Common year */
        leap = 0;
    }

    /* Month */
    if ( val < 31 ) {
        /* January */
        tm->tm_mon = 0;
        tm->tm_mday = val + 1;
        return 0;
    }
    val -= 31;
    if ( (int)val < 28 + leap ) {
        /* February */
        tm->tm_mon = 1;
        tm->tm_mday = val + 1;
        return 0;
    }
    val -= 28 + leap;
    if ( (int)val < 31 ) {
        /* March */
        tm->tm_mon = 2;
        tm->tm_mday = val + 1;
        return 0;
    }
    val -= 31;
    if ( (int)val < 30 ) {
        /* April */
        tm->tm_mon = 3;
        tm->tm_mday = val + 1;
        return 0;
    }
    val -= 30;
    if ( (int)val < 31 ) {
        /* May */
        tm->tm_mon = 4;
        tm->tm_mday = val + 1;
        return 0;
    }
    val -= 31;
    if ( (int)val < 30 ) {
        /* June */
        tm->tm_mon = 5;
        tm->tm_mday = val + 1;
        return 0;
    }
    val -= 30;
    if ( (int)val < 31 ) {
        /* July */
        tm->tm_mon = 6;
        tm->tm_mday = val + 1;
        return 0;
    }
    val -= 31;
    if ( (int)val < 31 ) {
        /* August */
        tm->tm_mon = 7;
        tm->tm_mday = val + 1;
        return 0;
    }
    val -= 31;
    if ( (int)val < 30 ) {
        /* September */
        tm->tm_mon = 8;
        tm->tm_mday = val + 1;
        return 0;
    }
    val -= 30;
    if ( (int)val < 31 ) {
        /* October */
        tm->tm_mon = 9;
        tm->tm_mday = val + 1;
        return 0;
    }
    val -= 31;
    if ( (int)val < 30 ) {
        /* November */
        tm->tm_mon = 10;
        tm->tm_mday = val + 1;
        return 0;
    }
    val -= 30;
    /* December */
    tm->tm_mon = 11;
    tm->tm_mday = val + 1;
    return 0;
}

int
pash_module_clock_help(struct pash *pash, char *args[])
{
    return 0;
}

int
pash_module_clock_show(struct pash *pash, char *args[])
{
    struct timeval tv;
    struct tm tm;
    uint64_t val;

    /* Get the current unix timestamp */
    if ( 0 != gettimeofday(&tv, NULL) ) {
        return -1;
    }
    val = tv.tv_sec;

    _unixtime_to_datetime(val, &tm);
    printf("%d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year, tm.tm_mon + 1,
           tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    return 0;
}


static char *pash_module_clock_name = "clock";
static struct pash_module_api pash_module_clock_api = {
    .clear = NULL,
    .help = &pash_module_clock_help,
    .request = NULL,
    .show = &pash_module_clock_show,
};


/*
 * Initialize
 */
int
pash_module_clock_init(struct pash *pash)
{
    return pash_register_module(pash, pash_module_clock_name,
                                &pash_module_clock_api);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
