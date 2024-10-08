/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2020-2022 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is derivative work of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "nonUnityLewisViscosityETFCDiffusivity.H"
#include "fvcDiv.H"
#include "fvcLaplacian.H"
#include "fvcSnGrad.H"
#include "fvmSup.H"
#include "surfaceInterpolate.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace turbulenceThermophysicalTransportModels
{

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class TurbulenceThermophysicalTransportModel>
nonUnityLewisViscosityETFCDiffusivity<TurbulenceThermophysicalTransportModel>::
nonUnityLewisViscosityETFCDiffusivity
(
    const momentumTransportModel& momentumTransport,
    const thermoModel& thermo
)
:
    unityLewisEddyDiffusivity<TurbulenceThermophysicalTransportModel>
    (
        typeName,
        momentumTransport,
        thermo,
        false
    ),

    Sct_("Sct", dimless, this->coeffDict_),
    alpha_u_("alpha_u", dimensionSet(0, 2, -1, 0, 0, 0, 0), this->coeffDict_),
    Le_("Le", dimless, this->coeffDict_)
{
    this->printCoeffs(typeName);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class TurbulenceThermophysicalTransportModel>
bool
nonUnityLewisViscosityETFCDiffusivity<TurbulenceThermophysicalTransportModel>::read()
{
    if
    (
        unityLewisEddyDiffusivity<TurbulenceThermophysicalTransportModel>
        ::read()
    )
    {
        Sct_.read(this->coeffDict());
        alpha_u_.read(this->coeffDict());
        Le_.read(this->coeffDict());

        return true;
    }
    else
    {
        return false;
    }
}


template<class TurbulenceThermophysicalTransportModel>
tmp<surfaceScalarField>
nonUnityLewisViscosityETFCDiffusivity<TurbulenceThermophysicalTransportModel>::q() const
{
    tmp<surfaceScalarField> tmpq
    (
        surfaceScalarField::New
        (
            IOobject::groupName
            (
                "q",
                this->momentumTransport().alphaRhoPhi().group()
            ),
           -fvc::interpolate(this->alpha()*this->kappaEff())
           *fvc::snGrad(this->thermo().T())
        )
    );

    const PtrList<volScalarField>& Y = this->thermo().Y();

    if (Y.size())
    {
        surfaceScalarField hGradY
        (
            surfaceScalarField::New
            (
                "hGradY",
                Y[0].mesh(),
                dimensionedScalar(dimEnergy/dimMass/dimLength, 0)
            )
        );

        forAll(Y, i)
        {
            const volScalarField hi
            (
                this->thermo().hsi(i, this->thermo().p(), this->thermo().T())
            );

            hGradY += fvc::interpolate(hi)*fvc::snGrad(Y[i]);
        }

        tmpq.ref() -=
            fvc::interpolate
            (
                this->alpha()
                *(
                    this->DEff()
                )
            )*hGradY;
    }

    return tmpq;
}


template<class TurbulenceThermophysicalTransportModel>
tmp<fvScalarMatrix>
nonUnityLewisViscosityETFCDiffusivity<TurbulenceThermophysicalTransportModel>::divq
(
    volScalarField& he
) const
{
    tmp<fvScalarMatrix> tmpDivq
    (
        fvm::Su
        (
            -fvc::laplacian(this->alpha()*this->kappaEff(), this->thermo().T()),
            he
        )
    );

    const PtrList<volScalarField>& Y = this->thermo().Y();

    tmpDivq.ref() -=
        fvm::laplacianCorrection(this->alpha()*this->alphaEff(), he);

    surfaceScalarField hGradY
    (
        surfaceScalarField::New
        (
            "hGradY",
            he.mesh(),
            dimensionedScalar(he.dimensions()/dimLength, 0)
        )
    );

    forAll(Y, i)
    {
        const volScalarField hi
        (
            this->thermo().hsi(i, this->thermo().p(), this->thermo().T())
        );

        hGradY += fvc::interpolate(hi)*fvc::snGrad(Y[i]);
    }

    tmpDivq.ref() -=
        fvc::div
        (
            fvc::interpolate
            (
                this->alpha()
               *(
                    this->DEff()
               )
            )*hGradY*he.mesh().magSf()
        );

    return tmpDivq;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace turbulenceThermophysicalTransportModels
} // End namespace Foam

// ************************************************************************* //
