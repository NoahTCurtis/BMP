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

void URoomManagerComponent::BeginLoadRoom(FName RoomTag, FName DoorTag, FTransform DoorTransform, FOnRoomLoadedDelegate InLoadCompleteDelegate)
{
	UE_LOG(LogRoomManager, Log, TEXT("URoomManagerComponent::BeginLoadRoom"));

	LoadCompleteDelegate = InLoadCompleteDelegate;

	//todo: replace this member with a tag-based lookup
	//TSoftObjectPtr<UWorld> LevelToLoad

	if (!GetWorld())
	{
		return;
	}

	if (LevelToLoad.IsNull())
	{
		UE_LOG(LogRoomManager, Warning, TEXT("LoadRoom: Invalid RoomWorld! Has no asset path set!"));
		return;
	}

	//decide location
	//...

	// Async load level instance
	bool Success = false;
	RoomLoadingHandle = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		GetWorld(),
		LevelToLoad,
		DoorTransform,
		Success,
		RoomTag.ToString()
	);

	if (Success == false)
	{
		UE_LOG(LogRoomManager, Warning, TEXT("LoadRoom: Invalid LoadLevelInstance Failed!"));
		return;
	}

	RoomLoadingHandle->OnLevelLoaded.AddDynamic(this, &URoomManagerComponent::LoadRoomComplete);
}

void URoomManagerComponent::LoadRoomComplete() //temp func
{
	LoadCompleteDelegate.Execute();
}





