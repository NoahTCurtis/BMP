// Fill out your copyright notice in the Description page of Project Settings.

#include "RoomManagerComponent.h"

#include "DoorComponent.h"
#include "RoomComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"
#include "DrawDebugHelpers.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogRoomManager);

URoomManagerComponent::URoomManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URoomManagerComponent::CollectDoorTransforms()
{
	KnownDoorLocations.Empty();

	// --- Collect all map assets under /Game/Rooms ---
	FAssetRegistryModule& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistry.Get().SearchAllAssets(/*bSynchronousSearch=*/true);

	TArray<FAssetData> AssetList;
	AssetRegistry.Get().GetAssetsByPath(RoomLevelInstanceDirectory, AssetList, /*bRecursive=*/true);

	TArray<FAssetData> MapAssets;
	for (const FAssetData& Asset : AssetList)
	{
		if (Asset.AssetClassPath == FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("World")))
		{
			MapAssets.Add(Asset);
		}
	}

	if (MapAssets.IsEmpty())
	{
		UE_LOG(LogRoomManager, Warning, TEXT("ScanAllRoomDoors: No maps found in %s"), *RoomLevelInstanceDirectory.ToString());
		return;
	}

	// --- Spawn a Slate progress notification ---
	FNotificationInfo Info(FText::FromString(TEXT("Scanning room doors...")));
	Info.bFireAndForget = false;          // We'll dismiss it manually
	Info.bUseThrobber = true;
	Info.ExpireDuration = 0.0f;
	TSharedPtr<SNotificationItem> Notification =
		FSlateNotificationManager::Get().AddNotification(Info);
	if (Notification)
		Notification->SetCompletionState(SNotificationItem::CS_Pending);

	// --- Process each map synchronously ---
	for (int32 i = 0; i < MapAssets.Num(); i++)
	{
		const FAssetData& MapAsset = MapAssets[i];
		FString LevelName = MapAsset.AssetName.ToString();

		// Update the notification text with current progress
		if (Notification)
		{
			FString ProgressText = FString::Printf(
				TEXT("Scanning room doors... (%d / %d) %s"),
				i + 1, MapAssets.Num(), *LevelName
			);
			Notification->SetText(FText::FromString(ProgressText));
		}

		// Force Slate to repaint so the user actually sees the update
		FSlateApplication::Get().Tick();
		FSlateApplication::Get().GetRenderer()->Sync();

		// --- Load the package synchronously ---
		FString PackagePath = MapAsset.PackageName.ToString();
		UPackage* Package = LoadPackage(nullptr, *PackagePath, LOAD_None);
		if (!Package)
		{
			UE_LOG(LogRoomManager, Warning, TEXT("ScanAllRoomDoors: Failed to load package %s"), *PackagePath);
			continue;
		}

		// Find the UWorld inside the loaded package
		UWorld* LoadedWorld = nullptr;
		ForEachObjectWithPackage(Package, [&](UObject* Obj)
			{
				if (UWorld* World = Cast<UWorld>(Obj))
				{
					LoadedWorld = World;
					return false; // Stop iterating
				}
				return true;
			}, /*bIncludeNestedObjects=*/false);

		if (!LoadedWorld)
		{
			UE_LOG(LogRoomManager, Warning, TEXT("ScanAllRoomDoors: No UWorld found in %s"), *PackagePath);
			continue;
		}

		// --- Scan actors in the persistent level ---
		ULevel* Level = LoadedWorld->PersistentLevel;
		if (!Level) continue;

		FRoomDoorTransformListStruct NewRoomDoorList;
		NewRoomDoorList.LevelPtr = TSoftObjectPtr<UWorld>(MapAsset.GetSoftObjectPath());
		FGameplayTag RoomTag = FGameplayTag::EmptyTag;

		for (AActor* Actor : Level->Actors)
		{
			if (!IsValid(Actor)) continue;

			if (UDoorComponent* DoorComp = Actor->FindComponentByClass<UDoorComponent>())
			{
				//RecordDoorTransform(Actor->GetActorTransform(), LevelName);
				UE_LOG(LogRoomManager, Log, TEXT("ScanAllRoomDoors: Found a door (%s) in room (%s) with Location (%s)"), *Actor->GetName(), *PackagePath, *Actor->GetActorLocation().ToString());

				FDoorTransformPairStruct Door;
				Door.DoorTransform = Actor->GetActorTransform();
				Door.DoorTag = DoorComp->ThisDoorTag;

				//TODO: If there's a duplicate door tag in this room, throw an error and continue

				NewRoomDoorList.DoorTransforms.Add(Door);
			}
			else if (URoomComponent* RoomComp = Actor->FindComponentByClass<URoomComponent>())
			{
				RoomTag = RoomComp->RoomTag;

				//TODO: If there's a duplicate room in the list, throw an error and quit
			}
		}

		if (RoomTag == FGameplayTag::EmptyTag)
		{
			UE_LOG(LogRoomManager, Warning, TEXT("ScanAllRoomDoors: No RoomComponent found in %s"), *PackagePath);
			continue;
		}

		if (NewRoomDoorList.DoorTransforms.Num() == 0)
		{
			UE_LOG(LogRoomManager, Warning, TEXT("ScanAllRoomDoors: No doors found in %s"), *PackagePath);
			continue;
		}
		
		KnownDoorLocations.Add(RoomTag, NewRoomDoorList);

		// Optionally: release the package from memory after scanning
		// to avoid holding all rooms loaded at once.
		///Package->ClearFlags(RF_RootSet);
		///CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		
		// Mark the package as eligible for GC
		Package->SetFlags(RF_Transient);
		Package->ClearFlags(RF_Standalone);

		// Detach the world so it doesn't hold references
		if (LoadedWorld)
		{
			LoadedWorld->CleanupWorld();
		}

		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}

	// --- Dismiss notification with success ---
	if (Notification)
	{
		Notification->SetText(FText::FromString(TEXT("Room door scan complete.")));
		Notification->SetCompletionState(SNotificationItem::CS_Success);
		Notification->ExpireAndFadeout();
	}
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
	switch (RoomQueue[0].LoadState)
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
	if (KnownDoorLocations.Contains(InRoomStateStruct.RoomTag) == false)
	{
		UE_LOG(LogRoomManager, Error, TEXT("RequestLoadRoom: Failed to find room (%s) in map. Try rebuilding room list"), *InRoomStateStruct.RoomTag.ToString());
		return;
	}

	FRoomStateStruct NewRoomState = InRoomStateStruct;

	NewRoomState.LoadState = ERoomLoadState::WantsLoad;

	// Todo: Perform room level lookup here
	//check room is not loaded yet

	RoomQueue.Add(NewRoomState);
}



void URoomManagerComponent::DoRoomState_WantsLoad()
{
	FRoomStateStruct& RoomState = RoomQueue[0];
	
	//UE_LOG(LogRoomManager, Verbose, TEXT("DoRoomState_WantsLoad %s"), RoomState.RoomTag.ToString());

	if (!GetWorld())
	{
		return;
	}

	//todo: replace this member with a tag-based lookup
	TSoftObjectPtr<UWorld> LevelToLoad;
	FTransform RoomLoadTransform = FTransform::Identity;
	if (FRoomDoorTransformListStruct* RoomDoorList = KnownDoorLocations.Find(RoomState.RoomTag))
	{
		//find the door transform
		FTransform LoadedDoorTransform;
		for (int i = 0; i < RoomDoorList->DoorTransforms.Num(); i++)
		{
			if (RoomDoorList->DoorTransforms[i].DoorTag == RoomState.DoorTag)
			{
				LoadedDoorTransform = RoomDoorList->DoorTransforms[i].DoorTransform;
				break;
			}
		}

		//create the room's transform
		LevelToLoad = RoomDoorList->LevelPtr;

		//Find the transformation to apply to the new room
		//opposite of Bside transform
		FTransform InverseBSide = LoadedDoorTransform.Inverse();
		//180 degree pivot
		FTransform Pivot = FTransform::Identity;
		Pivot.SetRotation(FQuat(FVector::UpVector, 3.1415926535f));
		//Aside transform
		FTransform ASide = RoomState.RoomEntryDoorTransform;

		RoomLoadTransform = InverseBSide * Pivot * ASide;
	}
	else
	{
		UE_LOG(LogRoomManager, Error, TEXT("DoRoomState_WantsLoad: Failed to find room (%s). Try rebuilding room list"), *RoomState.RoomTag.ToString());
		return;
	}

	FString dumbhack = FString::FromInt(FMath::Rand());

	// Async load level instance
	bool Success = false;
	RoomState.StreamingLevel = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		GetWorld(),
		LevelToLoad,
		RoomLoadTransform,
		Success,
		//RoomState.RoomTag.ToString()
		*dumbhack
	);

	if (Success == false)
	{
		UE_LOG(LogRoomManager, Warning, TEXT("DoRoomState_WantsLoad: Invalid LoadLevelInstance Failed!"));
		return;
	}

	//RoomState.StreamingLevel->OnLevelLoaded.AddDynamic(this, &URoomManagerComponent::AsyncLoadRoomComplete);
	RoomState.StreamingLevel->OnLevelShown.AddDynamic(this, &URoomManagerComponent::AsyncLoadRoomComplete);

	RoomState.LoadState = ERoomLoadState::Loading;
}

void URoomManagerComponent::DoRoomState_Loading()
{
	FRoomStateStruct& RoomState = RoomQueue[0];
}

void URoomManagerComponent::DoRoomState_Loaded()
{
	UE_LOG(LogRoomManager, Warning, TEXT("DoRoomState_Loaded called but AsyncLoadRoomComplete should've removed this room from queue!"));
	/*
	FRoomStateStruct& RoomState = RoomQueue[0];

	RoomState.RoomStateLoadCompleteDelegate.Execute();

	// Move the room from the queue to the stored loaded rooms
	RoomState.LoadState = ERoomLoadState::Loaded;
	LoadedRooms.Add(RoomState);
	RoomQueue.RemoveAt(0, 1, true);
	*/
}

void URoomManagerComponent::DoRoomState_WantsUnload()
{
	FRoomStateStruct& RoomState = RoomQueue[0];
}

void URoomManagerComponent::DoRoomState_Unloading()
{
	FRoomStateStruct& RoomState = RoomQueue[0];
}

void URoomManagerComponent::DoRoomState_Unloaded()
{
	FRoomStateStruct& RoomState = RoomQueue[0];
}




// Called by level loading system
void URoomManagerComponent::AsyncLoadRoomComplete()
{
	FRoomStateStruct& RoomState = RoomQueue[0];

	// move the room to the correct position
	ULevel* LoadedLevel = RoomState.StreamingLevel->GetLoadedLevel();
	if (LoadedLevel == nullptr)
	{
		UE_LOG(LogRoomManager, Error, TEXT("AsyncLoadRoomComplete got invalid room!"));
		return;
	}

	// Find the matching door (door that points back into the room that summoned it)
	UDoorComponent* ActorDoorComp = nullptr;
	for (AActor* Actor : LoadedLevel->Actors)
	{
		ActorDoorComp = Actor->FindComponentByClass<UDoorComponent>();
		if (ActorDoorComp != nullptr)
		{
			if (ActorDoorComp->ThisDoorTag == RoomState.DoorTag)
			{
				//Open the other door. Both sides of the door need to be open.
				ActorDoorComp->OpenImmediate();

				break;
			}
		}
	}

	if(ActorDoorComp == nullptr)
	{
		UE_LOG(LogRoomManager, Error, TEXT("AsyncLoadRoomComplete: Loaded room with no back-pointing door! Something is very wrong in (%s)"), *RoomState.RoomTag.ToString());
		return;
	}

	// Mark the room as loaded
	// Move the room from the queue to the stored loaded rooms
	RoomState.LoadState = ERoomLoadState::Loaded;
	LoadedRooms.Add(RoomState);
	RoomQueue.RemoveAt(0, 1, true);

	// Tell the instigator the room is ready (door will open)
	RoomState.RoomStateLoadCompleteDelegate.Execute();
}

// Called by level loading system
void URoomManagerComponent::AsyncUnloadRoomComplete()
{
	FRoomStateStruct& RoomState = RoomQueue[0];
	RoomState.LoadState = ERoomLoadState::Unloaded;
}


