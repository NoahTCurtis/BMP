#pragma once

#include "CoreMinimal.h"
#include "MeshOperationDesc.generated.h"

UENUM(BlueprintType)
enum class EMeshOperationType : uint8
{
    MOT_Beam UMETA(DisplayName = "Beam"),
    MOT_Sphere UMETA(DisplayName = "Sphere"),
};

USTRUCT(BlueprintType)
struct FMeshOperationDesc
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Radius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Origin;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Direction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCopyToDestination;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Destination;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bInvert;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMeshOperationType Type;
};
