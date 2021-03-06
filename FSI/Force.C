#include "Force.H"

using namespace Foam;

preciceAdapter::FSI::Force::Force
(
        const Foam::fvMesh& mesh,
        const std::string solverType
)
:
mesh_(mesh),
solverType_(solverType),
porousForces_(false),
force_field_created(false)
{}


preciceAdapter::FSI::Force::Force
(
    const Foam::fvMesh& mesh,
    const fileName& timeName,
    const std::string solverType,
    const bool porousForces
    /* TODO: We should add any required field names here.
    /  They would need to be vector fields.
    /  See FSI/Temperature.C for details.
    */
)
:
mesh_(mesh),
solverType_(solverType),
porousForces_(porousForces),
force_field_created(true)
{
    //What about type "basic"?
    if (solverType_.compare("incompressible") != 0 &&                                                   solverType_.compare("compressible") != 0 && 
    solverType_.compare("solid") != 0) 
    {
        FatalErrorInFunction
            << "Forces calculation does only support "
            << "compressible, incompressible or solid solver type."
            << exit(FatalError);
    }
    
    dataType_ = vector;

    Force_ = new volVectorField
    (
        IOobject
        (
            "Force",
            timeName,
            mesh,
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedVector
        (
            "fdim",
            dimensionSet(1,1,-2,0,0,0,0),
            Foam::vector::zero
        )
    );
}

//Calculate solid force
Foam::tmp<Foam::volSymmTensorField> preciceAdapter::FSI::Force::devSigma() const
{
    return mesh_.lookupObject<volSymmTensorField>("sigmaTot");
}

//Calculate viscous force
Foam::tmp<Foam::volSymmTensorField> preciceAdapter::FSI::Force::devRhoReff() const
{
    //For turbulent flows 
    typedef compressible::turbulenceModel cmpTurbModel;
    typedef incompressible::turbulenceModel icoTurbModel;   
    
    if (mesh_.foundObject<cmpTurbModel>(cmpTurbModel::propertiesName))
    {
        const cmpTurbModel & turb
        (
            mesh_.lookupObject<cmpTurbModel>(cmpTurbModel::propertiesName)
        );

        return turb.devRhoReff();
    }    
    else if (mesh_.foundObject<icoTurbModel>(icoTurbModel::propertiesName))
    {       
        const incompressible::turbulenceModel& turb
        (
            mesh_.lookupObject<icoTurbModel>(icoTurbModel::propertiesName)
        );

        return rho()*turb.devReff();        
    }
    else
    {        
        // For laminar flows get the velocity  
        const volVectorField & U
        (
            mesh_.lookupObject<volVectorField>("U")
        );
        
        return -mu()*dev(twoSymm(fvc::grad(U)));
    }
}

//lookup correct rho
Foam::tmp<Foam::volScalarField> preciceAdapter::FSI::Force::rho() const
{
    // If volScalarField exists, read it from registry (for compressible cases)
    // interFoam is incompressible but has volScalarField rho

    if (mesh_.foundObject<volScalarField>("rho"))
    {        
        return mesh_.lookupObject<volScalarField>("rho");            
    }
    else if (solverType_.compare("incompressible") == 0)
    {        
        const dictionary& FSIDict =
            mesh_.lookupObject<IOdictionary>("preciceDict").subOrEmptyDict("FSI");
            
        return tmp<volScalarField>
        (
            new volScalarField
            (
                IOobject
                (
                    "rho",
                    mesh_.time().timeName(),
                    mesh_,
                    IOobject::NO_READ,
                    IOobject::NO_WRITE
                ),
                mesh_,
                dimensionedScalar(FSIDict.lookup("rho"))
            )
        );
        
    } 
    else
    {
        FatalErrorInFunction
            << "Did not find the correct rho."
            << exit(FatalError);
            
        return volScalarField::null();
    }
}

//lookup correct mu
Foam::tmp<Foam::volScalarField> preciceAdapter::FSI::Force::mu() const
{ 

    if (solverType_.compare("incompressible") == 0)
    {
        typedef immiscibleIncompressibleTwoPhaseMixture iitpMixture;
        if (mesh_.foundObject<iitpMixture>("mixture"))
        {
            const iitpMixture& mixture
            (
                mesh_.lookupObject<iitpMixture>("mixture")
            );
                
            return mixture.mu();
        }
        else
        {        
        
            const dictionary& FSIDict =
                mesh_.lookupObject<IOdictionary>("preciceDict").subOrEmptyDict("FSI");
                
            dimensionedScalar nu(FSIDict.lookup("nu"));       
            
            return tmp<volScalarField>
            (
                new volScalarField
                (  
                    nu*rho()
                )
            );
        }

    }
    else if (solverType_.compare("compressible") == 0)
    {
        return mesh_.lookupObject<volScalarField>("thermo:mu");
    }
    else
    {
        FatalErrorInFunction
            << "Did not find the correct mu."
            << exit(FatalError);
            
        return volScalarField::null();
    }
}

void preciceAdapter::FSI::Force::write(double * buffer, bool meshConnectivity, const unsigned int dim)
{
    // Compute forces. See the Forces function object.

    // Normal vectors on the boundary, multiplied with the face areas
    const surfaceVectorField::Boundary& Sfb
    (
        mesh_.Sf().boundaryField()
    );

    // Stress tensor boundary field
    tmp<volSymmTensorField> tdevRhoReff(devRhoReff());
    const volSymmTensorField::Boundary& devRhoReffb
    (
        tdevRhoReff().boundaryField()
    );

    // Density boundary field
    tmp<volScalarField> trho(rho());
    const volScalarField::Boundary& rhob =
        trho().boundaryField();

    // Pressure boundary field
    tmp<volScalarField> tp = mesh_.lookupObject<volScalarField>("p");
    const volScalarField::Boundary& pb
    (
        tp().boundaryField()
    );

    int bufferIndex = 0;
    // For every boundary patch of the interface
    for (uint j = 0; j < patchIDs_.size(); j++)
    {

        int patchID = patchIDs_.at(j);
        
        if (solverType_.compare("solid") == 0)
        {
            tmp<volSymmTensorField> tdevSigma(devSigma());
            const volSymmTensorField::Boundary& devSigmab
            (
                tdevSigma().boundaryField()
            );            
                            
            Force_->boundaryFieldRef()[patchID] = 
                    - Sfb[patchID] & devSigmab[patchID];
        }
        else
        {
            // Pressure forces
            if (solverType_.compare("incompressible") == 0)
            {
                Force_->boundaryFieldRef()[patchID] =
                    Sfb[patchID] * pb[patchID] * rhob[patchID];
            }
            else if (solverType_.compare("compressible") == 0)
            {
                Force_->boundaryFieldRef()[patchID] =
                    Sfb[patchID] * (pb[patchID] - 1.0e5);
            }
            else
            {
                FatalErrorInFunction
                    << "Forces calculation does only support "
                    << "compressible or incompressible solver type."
                    << exit(FatalError);
            }
            
            //Solid forces
            if (porousForces_ == true)
            {
                tmp<volSymmTensorField> tdevSigma(devSigma());
                const volSymmTensorField::Boundary& devSigmab
                (
                    tdevSigma().boundaryField()
                );                
                                
                Force_->boundaryFieldRef()[patchID] -= 
                    Sfb[patchID] & devSigmab[patchID];                
            }
            
            // Viscous forces
            Force_->boundaryFieldRef()[patchID] +=
                Sfb[patchID] & devRhoReffb[patchID];            
        }
        
        // Write the forces to the preCICE buffer
        // For every cell of the patch
        forAll(Force_->boundaryFieldRef()[patchID], i)
        {
            // Copy the force into the buffer
            // x-dimension
            buffer[bufferIndex++]
            = 
            Force_->boundaryFieldRef()[patchID][i].x();

            // y-dimension
            buffer[bufferIndex++]
            =
            Force_->boundaryFieldRef()[patchID][i].y();

            if(dim == 3)
                // z-dimension
                buffer[bufferIndex++]
                        =
                        Force_->boundaryFieldRef()[patchID][i].z();
        }
    }
}

void preciceAdapter::FSI::Force::read(double * buffer, const unsigned int dim)
{
    // For every element in the buffer
    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < patchIDs_.size(); j++)
    {
        int patchID = patchIDs_.at(j);

        // Get the force on the patch
        fvPatchVectorField& ForcePatch =
            refCast<fvPatchVectorField>
            (
                Force_->boundaryFieldRef()[patchID]
            );

        // For every cell of the patch
        forAll(Force_->boundaryFieldRef()[patchID], i)
        {
            // Set the force to the received one
            ForcePatch[i][0] = buffer[bufferIndex++];
            ForcePatch[i][1] = buffer[bufferIndex++];
            if(dim == 3)
                ForcePatch[i][2] = buffer[bufferIndex++];
            
        }
    }
}

preciceAdapter::FSI::Force::~Force()
{
  if(force_field_created)
    delete Force_;
}
