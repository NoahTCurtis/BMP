// Fill out your copyright notice in the Description page of Project Settings.

#include "RoomManagerComponent.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogRoomManager);

URoomManagerComponent::URoomManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void URoomManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void URoomManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (RoomQueue.Num() == 0)
	{
		return; // Nothing to process
	}

	// Look at the next task
	FRoomStateStruct& NextTask = RoomQueue[0];

	switch (NextTask.LoadState)
	{
	case ERoomLoadState::WantsLoad:
		DoRoomState_WantsLoad();
		break;

		UPROPERTY()
	case ERoomLoadState::Loading:
		DoRoomState_Loading();
		break;

		UPROPERTY()
	case ERoomLoadState::Loaded:
		DoRoomState_Loaded();
		break;

		UPROPERTY()
	case ERoomLoadState::WantsUnload:
		DoRoomState_WantsUnload();
		break;

		UPROPERTY()
	case ERoomLoadState::Unloading:
		DoRoomState_Unloading();
		break;

		UPROPERTY()
	case ERoomLoadState::Unloaded:
		DoRoomState_Unloaded();
		break;
	}
}

void URoomManagerComponent::RequestLoadRoom(const FRoomStateStruct& InRoomStateStruct)
{
	FRoomStateStruct NewRoomState = InRoomStateStruct;

	NewRoomState.LoadState = ERoomLoadState::WantsLoad;

	// Todo: Perform room level lookup here
	//check room is not loaded yet

	RoomQueue.Add(NewRoomState);
}



void URoomManagerComponent::DoRoomState_WantsLoad()
{
	FRoomStateStruct& NextTask = RoomQueue[0];
	
	//UE_LOG(LogRoomManager, Verbose, TEXT("DoRoomState_WantsLoad %s"), NextTask.RoomTag.ToString());

	//todo: replace this member with a tag-based lookup
	//TSoftObjectPtr<UWorld> LevelToLoad

	if (!GetWorld())
	{
		return;
	}

	if (LevelToLoad.IsNull())
	{
		UE_LOG(LogRoomManager, Warning, TEXT("DoRoomState_WantsLoad: Invalid RoomWorld! Has no asset path set!"));
		return;
	}

	// Async load level instance
	bool Success = false;
	NextTask.StreamingLevel = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		GetWorld(),
		LevelToLoad,
		NextTask.RoomEntryDoorTransform,
		Success,
		NextTask.RoomTag.ToString()
	);

	if (Success == false)
	{
		UE_LOG(LogRoomManager, Warning, TEXT("DoRoomState_WantsLoad: Invalid LoadLevelInstance Failed!"));
		return;
	}

	NextTask.StreamingLevel->OnLevelLoaded.AddDynamic(this, &URoomManagerComponent::AsyncLoadRoomComplete);

	NextTask.LoadState = ERoomLoadState::Loading;
}

void URoomManagerComponent::DoRoomState_Loading()
{
	FRoomStateStruct& NextTask = RoomQueue[0];
}

void URoomManagerComponent::DoRoomState_Loaded()
{
	FRoomStateStruct& NextTask = RoomQueue[0];

	NextTask.RoomStateLoadCompleteDelegate.Execute();

	// Move the room from the queue to the stored loaded rooms
	NextTask.LoadState = ERoomLoadState::Loaded;
	LoadedRooms.Add(NextTask);
	RoomQueue.RemoveAt(0, 1, true);
}

void URoomManagerComponent::DoRoomState_WantsUnload()
{
	FRoomStateStruct& NextTask = RoomQueue[0];
}

void URoomManagerComponent::DoRoomState_Unloading()
{
	FRoomStateStruct& NextTask = RoomQueue[0];
}

void URoomManagerComponent::DoRoomState_Unloaded()
{
	FRoomStateStruct& NextTask = RoomQueue[0];
}




// Called by level loading system
void URoomManagerComponent::AsyncLoadRoomComplete()
{
	FRoomStateStruct& NextTask = RoomQueue[0];
	NextTask.LoadState = ERoomLoadState::Loaded;
}

// Called by level loading system
void URoomManagerComponent::AsyncUnloadRoomComplete()
{
	FRoomStateStruct& NextTask = RoomQueue[0];
	NextTask.LoadState = ERoomLoadState::Unloaded;
}


