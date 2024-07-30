#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MeshOperationDesc.h"
#include "ReactiveMeshInterface.generated.h"

// Interface for all objects that react to collision-shaping effects
UINTERFACE(Blueprintable)
class UReactiveMeshInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface class
 */
class IReactiveMeshInterface
{
    GENERATED_BODY()

public:
    // Define the function that takes the struct as an argument
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ReactiveMeshInterface")
    void PerformMeshOperation(const FMeshOperationDesc& MeshOperationDesc);
};