// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

#include "GameplayTagContainer.h"

#include "DoorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOpenImmediateEvent);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BMP_API UDoorComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDoorComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//Called when this door is loaded to be opened from another room
	void OpenImmediate();

	UPROPERTY(BlueprintAssignable, Category = "Door")
	FOnOpenImmediateEvent OnOpenImmediateDelegate;

	// Tag for which door in this one I am. (e.g. Door.1, Door.2, Door.Special)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ThisDoorTag;

	// Tag of the room this door goes to (e.g. Room.Lobby, Room.Boss1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ThatRoomTag;

	// Tag of the door this door goes to (e.g. Door.3, Door.4, Door.Secret)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ThatDoorTag;
};
