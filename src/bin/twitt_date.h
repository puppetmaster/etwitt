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
#ifndef ETWITT_DATE_H
#define ETWITT_DATE_H

#include <stdio.h>
#include <string.h>
#include <time.h>

/**
 * Decode a twitter date into a time_t value. Date format is like: Mon Dec 05 09:05:13 +0000 2011
 *
 * @param date The date represented as an RFC822 kind of string
 *
 * @return The time_t decoded value, -1 if decoding failed.
 **/
time_t decode_twitt_date(const char *date);

#endif

