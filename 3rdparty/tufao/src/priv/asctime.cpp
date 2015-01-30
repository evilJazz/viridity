/*  This file is part of the Tufão project
    Copyright (C) 2012 Vinícius dos Santos Oliveira <vini.ipsmaker@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any
    later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "asctime.h"

namespace Tufao {

const QRegExp Asctime::asctime("(?:\\w{3})\\s+" // day
                               "(\\w{3})\\s+" // month-1
                               "(\\d{1,2})\\s+" // day-2
                               "(\\d{2}):" // hour-3
                               "(\\d{2}):" // minutes-4
                               "(\\d{2})\\s+" // seconds-5
                               "(\\d{4})" // year-6
                               );

} // namespace Tufao
