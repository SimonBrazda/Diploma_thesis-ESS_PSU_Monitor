// A Module for Power Supply Analysis of Electronic Security Systems
// Copyright (C) 2021 by Šimon Brázda <simonbrazda@seznam.cz>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "DateTimeFormatter.h"

String DateTimeFormatter::GetDateTime(const RtcDateTime& dt) {
    char datestring[20];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    return String(datestring);
}

String DateTimeFormatter::GetDate(const RtcDateTime& dt) {
    char datestring[11];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%04u/%02u/%02u"),
            dt.Year(),
            dt.Month(),
            dt.Day() );
    return String(datestring);
}

String DateTimeFormatter::GetTime(const RtcDateTime& dt) {
    char datestring[9];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u:%02u:%02u"),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    return String(datestring);
}