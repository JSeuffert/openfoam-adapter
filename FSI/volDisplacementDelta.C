#include "volDisplacementDelta.H"

using namespace Foam;

preciceAdapter::FSI::volDisplacementDelta::volDisplacementDelta
(
    const Foam::fvMesh& mesh,
    const std::string nameDCellDisplacement
)
:
DcellDisplacement_(
    const_cast<volVectorField*>
    (
        &mesh.lookupObject<volVectorField>(nameDCellDisplacement)
    )
)
{
    dataType_ = vector;
}

void preciceAdapter::FSI::volDisplacementDelta::write(double * buffer, bool meshConnectivity, const unsigned int dim)
{
    // For every element in the buffer    
    int bufferIndex = 0;
    
    // For every boundary patch of the interface
    for (uint j = 0; j < patchIDs_.size(); j++)
    {
        int patchID = patchIDs_.at(j);
        
        // Get the displacement on the patch
        fvPatchVectorField& DcellDisplacementFluidPatch
        (
            refCast<fvPatchVectorField>
            (
                DcellDisplacement_->boundaryFieldRef()[patchID]
            )
        );
        
        // Write the displacements to the preCICE buffer
        // For every cell of the patch
        forAll(DcellDisplacement_->boundaryFieldRef()[patchID], i)
        {
            // Copy the dispalcement into the buffer
            // x-dimension
            buffer[bufferIndex++]
            = 
            DcellDisplacementFluidPatch[i][0];

            // y-dimension
            buffer[bufferIndex++]
            =
            DcellDisplacementFluidPatch[i][1];

            if(dim == 3)
                // z-dimension
                buffer[bufferIndex++]
                =
                DcellDisplacementFluidPatch[i][2];
        }
    }
}

// return the displacement to use later in the velocity?
void preciceAdapter::FSI::volDisplacementDelta::read(double * buffer, const unsigned int dim)
{    
    // For every element in the buffer
    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < patchIDs_.size(); j++)
    {
        int patchID = patchIDs_.at(j);

        // Get the displacementDelta on the patch
        fvPatchVectorField& DcellDisplacementFluidPatch
        (
            refCast<fvPatchVectorField>
            (
                DcellDisplacement_->boundaryFieldRef()[patchID]
            )
        );

        // For every cell of the patch
        forAll(DcellDisplacement_->boundaryFieldRef()[patchID], i)
        {
            // Set the displacementDeltas to the received one
            DcellDisplacementFluidPatch[i][0] = buffer[bufferIndex++];
            DcellDisplacementFluidPatch[i][1] = buffer[bufferIndex++];
            if(dim == 3)
                DcellDisplacementFluidPatch[i][2] = buffer[bufferIndex++];
        }
    }
}
