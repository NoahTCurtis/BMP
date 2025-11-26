// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/LevelStreamingDynamic.h"
#include "RoomManagerComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRoomManager, Log, All);

DECLARE_DYNAMIC_DELEGATE(FOnRoomLoadedDelegate); //for talking to blueprint

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BMP_API URoomManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URoomManagerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	//Called by doors when they want their room loaded
	UFUNCTION(BlueprintCallable)
	void BeginLoadRoom(FName RoomTag, FName DoorTag, FTransform DoorTransform, FOnRoomLoadedDelegate InLoadCompleteDelegate);

	// Called by level streaming system when a room is done loading
	UFUNCTION()
	void LoadRoomComplete();

	// todo: replace this with a tag and doormatching lookup system
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UWorld> LevelToLoad;

protected:
	// Stored blueprint callback to tell a door it has permission to open
	FOnRoomLoadedDelegate LoadCompleteDelegate;

	// Handle to the level streaming instance. Can also be used to unload the room.
	UPROPERTY()
	ULevelStreamingDynamic* RoomLoadingHandle;
};
