/*
 * Twitter Date parser for etwitt
 * Copyright (C) 20011 Raoul Hecky <raoul.hecky@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include "twitt_date.h"
#include <Eina.h>

//Date string exemple:
// Mon Dec 05 09:05:13 +0000 2011

static const char months[][4] = 
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char week_days[][4] = 
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static int _decode_int(const char *str, int *val)
{
    int sign = 1;
    int v = 0;

    if (!str)
        return -1;

    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    else if (*str == '+')
    {
        str++;
    }

    while (*str)
    {
        if (*str >= '0' && *str <= '9')
            v = (v * 10) + (*str - '0');
        else
            return -1;

        str++;
    }

    v *= sign;
    *val = v;

    return 0;
}

static int _decode_week_day(const char *date)
{
    int d;

    if (!date)
        return -1;

    for (d = 0;d < 7;d++)
    {
        if (!strncasecmp(date, week_days[d], 3))
            return d;
    }

    return -1;
}

static int _decode_month(const char *date)
{
    int m;

    if (!date)
        return -1;

    for (m = 0;m < 12;m++)
    {
        if (!strncasecmp(date, months[m], 3))
            return m;
    }

    return -1;
}

static int _decode_mday(const char *date)
{
    int d;

    if (!date)
        return -1;

    if (_decode_int(date, &d) < 0)
        return -1;

    if (d < 0 || d > 31)
        return -1;

    return d;
}

static int _decode_time(const char *date, int *h, int *m, int *s)
{
    int i, fail = 0;
    char **tokens;
    
    if (!date)
        return -1;

    tokens = eina_str_split(date, ":", 0);
    *h = *m = *s = 0;

    for (i = 0;tokens[i];i++)
    {
        if (i == 0) //hours
            fail += _decode_int(tokens[i], h);
        else if (i == 1) //minutes
            fail += _decode_int(tokens[i], m);
        else if (i == 2) //seconds
            fail += _decode_int(tokens[i], s);
    }

    if (fail < 0)
        return -1;

    return 1;
}

static int _decode_year(const char *date)
{
    int y;

    if (!date)
        return -1;

    if (_decode_int(date, &y) < 0)
        return -1;

    if (y == -1)
        return y;

    if (y < 100)
        y += (y < 70) ? 2000:1900;

    if (y < 1969)
        return -1;

    return y;
}

static int _decode_tzone(const char *date)
{
    int v;

    if (!date)
        return 0;

    if (_decode_int(date, &v) < 0)
    {
        printf("decode_twitt_date(%s): Decoding timezone other than numerical is not supported !\n", date);
        return 0;
    }

    return v;
}

time_t decode_twitt_date(const char *date)
{
    int i, v, hour, min, sec, tzone;
    struct tm tm;
    time_t t;
    char **tokens = eina_str_split(date, " ", 0);

    for (i = 0;tokens[i];i++)
    {
        if (i == 0) //week day
        {
            //Only decode week day if available
            if ((v = _decode_week_day(tokens[i])) != -1)
                tm.tm_wday = v;
        }
        else if (i == 1) //month
        {
            if ((v = _decode_month(tokens[i])) == -1)
                return (time_t) -1;

            tm.tm_mon = v;
        }
        else if (i == 2) //month day
        {
            if ((v = _decode_mday(tokens[i])) == -1)
                return (time_t) -1;

            tm.tm_mday = v;
        }
        else if (i == 3) //get hour:min:sec
        {
            if (!_decode_time(tokens[i], &hour, &min, &sec))
                return (time_t) -1;

            tm.tm_hour = hour;
            tm.tm_min = min;
            tm.tm_sec = sec;
        }
        else if (i == 4) //get timezone
        {
            tzone = _decode_tzone(tokens[i]);
        }
        else if (i == 5) //get year
        {
            if ((v = _decode_year(tokens[i])) == -1)
                return (time_t) -1;

            tm.tm_year = v - 1900;
        }
    }

    if ((t = mktime(&tm)) == -1)
        return t;

    // Convert with timezone infos
    t -= ((tzone / 100) * 60 * 60) + (tzone % 100) * 60;

    return t;
}

