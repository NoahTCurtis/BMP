// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Containers/Queue.h"
#include "Containers/Array.h"
#include "UObject/ScriptDelegates.h"
#include "RoomManagerComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRoomManager, Log, All);

DECLARE_DYNAMIC_DELEGATE(FOnRoomLoadedDelegate); //for talking to blueprint

UENUM(BlueprintType)
enum class ERoomLoadState : uint8
{
	WantsLoad			UMETA(DisplayName = "Wants Load"),
	Loading				UMETA(DisplayName = "Loading"),
	Loaded				UMETA(DisplayName = "Loaded"),
	WantsUnload			UMETA(DisplayName = "Wants Unload"),
	Unloading			UMETA(DisplayName = "Unloading"),
	Unloaded			UMETA(DisplayName = "Unloaded"),
};

USTRUCT(BlueprintType)
struct FRoomStateStruct
{
	GENERATED_BODY()

public:

	// The streaming level instance for this room
	UPROPERTY()
	ULevelStreamingDynamic* StreamingLevel = nullptr;

	// Current load state
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ERoomLoadState LoadState = ERoomLoadState::Unloaded;

	// Transform of the door that led to this room
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform RoomEntryDoorTransform;

	// Callback to the door that requested a room, so it can unlock
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FOnRoomLoadedDelegate RoomStateLoadCompleteDelegate;

	// Tag identifying the room (e.g. Room.Lobby, Room.Boss1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RoomTag;

	// Tag requesting a specific door in this room
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag DoorTag;

	// Tag identifying the room which requested this room
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag InstigatorRoomTag;

	// Tag identifying the door which requested this room
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag InstigatorDoorTag;

	FRoomStateStruct() = default;

	FRoomStateStruct(ULevelStreamingDynamic* InStreamingLevel,
					const FTransform& InRoomEntryDoorTransform,
					FOnRoomLoadedDelegate InRoomStateLoadCompleteDelegate,
					const FGameplayTag& InRoomTag,
					const FGameplayTag& InDoorTag, 
					const FGameplayTag& InInstigatorRoomTag, 
					const FGameplayTag& InInstigatorDoorTag)
		: StreamingLevel(InStreamingLevel)
		, LoadState(ERoomLoadState::Unloaded)
		, RoomEntryDoorTransform(InRoomEntryDoorTransform)
		, RoomStateLoadCompleteDelegate(InRoomStateLoadCompleteDelegate)
		, RoomTag(InRoomTag)
		, DoorTag(InDoorTag)
		, InstigatorRoomTag(InInstigatorRoomTag)
		, InstigatorDoorTag(InInstigatorDoorTag)
	{}
};

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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// Called by doors when they want their room loaded
	UFUNCTION(BlueprintCallable)
	void RequestLoadRoom(const FRoomStateStruct& InRoomStateStruct);

	// Called by level streaming system when a room is done loading
	UFUNCTION()
	void AsyncLoadRoomComplete();

	// Called by level streaming system when a room is done unloading
	UFUNCTION()
	void AsyncUnloadRoomComplete();

protected:
	UFUNCTION()
	void DoRoomState_WantsLoad();

	UFUNCTION()
	void DoRoomState_Loading();

	UFUNCTION()
	void DoRoomState_Loaded();

	UFUNCTION()
	void DoRoomState_WantsUnload();

	UFUNCTION()
	void DoRoomState_Unloading();

	UFUNCTION()
	void DoRoomState_Unloaded();


	// todo: replace this with a tag and doormatching lookup system
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UWorld> LevelToLoad;


	// Spawns the initial start room
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UWorld> FirstLevelToLoad;

protected:

	UPROPERTY()
	TArray<FRoomStateStruct> RoomQueue;

	UPROPERTY()
	TArray<FRoomStateStruct> LoadedRooms;

	// Handle to the level streaming instance. Can also be used to unload the room.
	UPROPERTY()
	ULevelStreamingDynamic* RoomLoadingHandle;
};
