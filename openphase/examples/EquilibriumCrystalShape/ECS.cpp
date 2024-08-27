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
 */

#include "Settings.h"
#include "RunTimeControl.h"
#include "DoubleObstacle.h"
#include "PhaseField.h"
#include "Initializations.h"
#include "BoundaryConditions.h"
#include "InterfaceProperties.h"

using namespace std;
using namespace openphase;

int main()
{
    Settings                        OPSettings;
    OPSettings.ReadInput();

    RunTimeControl                  RTC(OPSettings);
    PhaseField                      Phi(OPSettings);
    DoubleObstacle                  DO(OPSettings);
    InterfaceProperties             IP(OPSettings);
    BoundaryConditions              BC(OPSettings);

    int iRadius = OPSettings.Nx*0.45;
    // Initialize phase-fields //
    Initializations::Single(Phi, 0, BC, OPSettings);
    Initializations::Sphere(Phi, 1, iRadius,
    OPSettings.Nx*0.5, OPSettings.Ny*0.5, OPSettings.Nz*0.5, BC, OPSettings);

    IP.SetFacetOrientation(Phi);

    //------------------Starting the time loop ----------------//
    cout << "Entering the Time Loop!!!" << endl;
    for(RTC.tStep = RTC.tStart; RTC.tStep <= RTC.nSteps; RTC.IncrementTimeStep())
    {
    	IP.Set(Phi);
    	DO.CalculatePhaseFieldIncrements(Phi, IP);
    	Phi.NormalizeIncrements(BC, RTC.dt);
    	Phi.MergeIncrements(BC, RTC.dt);

        /// Output to file
        if (!(RTC.tStep%RTC.tFileWrite))
        {
            Phi.WriteVTK(RTC.tStep, OPSettings);
            IP.WriteVTK(Phi, RTC.tStep);
        }
        /// Output to screen
        if (!(RTC.tStep%RTC.tScreenWrite))
        {
            double I_En = DO.AverageEnergyDensity(Phi, IP);
            std::string message  = Info::GetStandard("Interface energy density", to_string(I_En));
            Info::WriteTimeStep(RTC.tStep, RTC.nSteps, message);
            Phi.PrintVolumeFractions();
        }
    }
    return 0;
}

