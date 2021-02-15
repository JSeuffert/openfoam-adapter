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
force_field_created(false)
{}


preciceAdapter::FSI::Force::Force
(
    const Foam::fvMesh& mesh,
    const fileName& timeName,
<<<<<<< Updated upstream
    const std::string solverType,
    const bool solidForces
=======
    const std::string solverType
>>>>>>> Stashed changes
    /* TODO: We should add any required field names here.
    /  They would need to be vector fields.
    /  See FSI/Temperature.C for details.
    */
)
:
mesh_(mesh),
solverType_(solverType),
<<<<<<< Updated upstream
solidForces_(solidForces)
=======
force_field_created(true)
>>>>>>> Stashed changes
{
    //What about type "basic"?
    if (solverType_.compare("incompressible") != 0 && solverType_.compare("compressible") != 0) 
    {
        FatalErrorInFunction
            << "Forces calculation does only support "
            << "compressible or incompressible solver type."
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
            IOobject::NO_READ,
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
<<<<<<< Updated upstream
}

//Calculate solid force
Foam::tmp<Foam::volSymmTensorField> preciceAdapter::FSI::Force::devSigma() const
{
    return mesh_.lookupObject<volSymmTensorField>("sigma");
=======
>>>>>>> Stashed changes
}

//Calculate viscous force
Foam::tmp<Foam::volSymmTensorField> preciceAdapter::FSI::Force::devRhoReff() const
{
    //For turbulent flows 
    typedef compressible::turbulenceModel cmpTurbModel;
    typedef incompressible::turbulenceModel icoTurbModel;   
    
    if (mesh_.foundObject<cmpTurbModel>(cmpTurbModel::propertiesName))
<<<<<<< Updated upstream
    {        
        const cmpTurbModel& turb =
            mesh_.lookupObject<cmpTurbModel>(cmpTurbModel::propertiesName);    
        
=======
    {
        const cmpTurbModel & turb
        (
            mesh_.lookupObject<cmpTurbModel>(cmpTurbModel::propertiesName)
        );

>>>>>>> Stashed changes
        return turb.devRhoReff();

    }    
    else if (mesh_.foundObject<icoTurbModel>(icoTurbModel::propertiesName))
<<<<<<< Updated upstream
    {        
        const incompressible::turbulenceModel& turb =
            mesh_.lookupObject<icoTurbModel>(icoTurbModel::propertiesName);
            
=======
    {
        const incompressible::turbulenceModel& turb
        (
            mesh_.lookupObject<icoTurbModel>(icoTurbModel::propertiesName)
        );

>>>>>>> Stashed changes
        return rho()*turb.devReff();        
    }
    else
    {        
        // For laminar flows get the velocity  
<<<<<<< Updated upstream
        const volVectorField& U = mesh_.lookupObject<volVectorField>("U");
=======
        const volVectorField & U
        (
            mesh_.lookupObject<volVectorField>("U")
        );
>>>>>>> Stashed changes
        
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
<<<<<<< Updated upstream
            const iitpMixture& mixture =
                mesh_.lookupObject<iitpMixture>("mixture");
=======
            const iitpMixture& mixture
            (
                mesh_.lookupObject<iitpMixture>("mixture")
            );
>>>>>>> Stashed changes
                
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
<<<<<<< Updated upstream
    tmp<volSymmTensorField> tdevRhoReff = devRhoReff();
    const volSymmTensorField::Boundary& devRhoReffb =
        tdevRhoReff().boundaryField();
=======
    tmp<volSymmTensorField> tdevRhoReff(devRhoReff());
    const volSymmTensorField::Boundary& devRhoReffb
    (
        tdevRhoReff().boundaryField()
    );
>>>>>>> Stashed changes

    // Density boundary field
    tmp<volScalarField> trho(rho());
    const volScalarField::Boundary& rhob =
        trho().boundaryField();

    // Pressure boundary field
    tmp<volScalarField> tp = mesh_.lookupObject<volScalarField>("p");
<<<<<<< Updated upstream
    const volScalarField::Boundary& pb =
        tp().boundaryField();        
=======
    const volScalarField::Boundary& pb
    (
        tp().boundaryField()
    );
>>>>>>> Stashed changes

    int bufferIndex = 0;
    // For every boundary patch of the interface
    for (uint j = 0; j < patchIDs_.size(); j++)
    {

        int patchID = patchIDs_.at(j);

<<<<<<< Updated upstream
        
        
        //pressure forces
=======
        // Pressure forces
>>>>>>> Stashed changes
        if (solverType_.compare("incompressible") == 0)
        {
            Force_->boundaryFieldRef()[patchID] =
                Sfb[patchID] * pb[patchID] * rhob[patchID];
        }
        else if (solverType_.compare("compressible") == 0)
        {
            Force_->boundaryFieldRef()[patchID] =
<<<<<<< Updated upstream
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
        if (solidForces_ == true)
=======
                Sfb[patchID] * pb[patchID];
        }
        else
>>>>>>> Stashed changes
        {
            FatalErrorInFunction
                << "Forces calculation does only support "
                << "compressible or incompressible solver type."
                << exit(FatalError);
        }
        
        // Viscous forces
        Force_->boundaryFieldRef()[patchID] +=
            Sfb[patchID] & devRhoReffb[patchID];
<<<<<<< Updated upstream
            
=======

>>>>>>> Stashed changes
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
<<<<<<< Updated upstream
                =
                Force_->boundaryFieldRef()[patchID][i].z();
=======
                        =
                        Force_->boundaryFieldRef()[patchID][i].z();
>>>>>>> Stashed changes
        }
    }
}

void preciceAdapter::FSI::Force::read(double * buffer, const unsigned int dim)
{
<<<<<<< Updated upstream
    // For every element in the buffer
    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < patchIDs_.size(); j++)
    {
        int patchID = patchIDs_.at(j);

        // Get the force on the patch
        fixedValueFvPatchVectorField& ForcePatch =
            refCast<fixedValueFvPatchVectorField>
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
=======
    /* TODO: Implement
    * We need two nested for-loops for each patch,
    * the outer for the locations and the inner for the dimensions.
    * See the preCICE readBlockVectorData() implementation.
    */
    FatalErrorInFunction
        << "Reading forces is not supported."
        << exit(FatalError);
>>>>>>> Stashed changes
}

preciceAdapter::FSI::Force::~Force()
{
  if(force_field_created)
    delete Force_;
}
