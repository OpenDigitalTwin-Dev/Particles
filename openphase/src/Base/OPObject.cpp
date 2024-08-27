/*
 *   This file is part of the OpenPhase (R) software library.
 *
 *   Copyright (c) 2009-2022 Ruhr-Universitaet Bochum,
 *                 Universitaetsstrasse 150, D-44801 Bochum, Germany
 *             AND 2018-2022 OpenPhase Solutions GmbH,
 *                 Universitaetsstrasse 136, D-44799 Bochum, Germany.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   File created :   2014
 *   Main contributors :   Efim Borukhovich; Oleg Shchyglo
 *
 */

#include "Base/OPObject.h"
#include "Info.h"

namespace openphase
{

OPObject::~OPObject(void)
{
    if(thisclassname.size() != 0)
    {
        Info::WriteStandard(this->thisclassname, "Exited normally");
    }
#ifdef DEBUG
    else
    {
        Info::WriteWarning("Uninitialized object exited normally",
                           "OPObject","~OPObject");
    }
#endif
}

}// namespace openphase
