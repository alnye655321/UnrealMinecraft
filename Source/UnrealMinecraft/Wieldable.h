// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "UnrealMinecraftCharacter.h"
#include "Wieldable.generated.h"


UCLASS()
class UNREALMINECRAFT_API AWieldable : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWieldable();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// types of pickups
	enum ETool : uint8
	{
		Unarmed,
		Pickaxe,
		Axe,
		Shovel,
		Sword
	};

	// materials, using assigned values for multiplicative action regarding damage to blocks
	enum EMaterial : uint8
	{
		None = 1,
		Wooden = 2,
		Stone = 4,
		Iron = 6,
		Diamond = 8,
		Golden = 12
	};

	// what type is the tool
	UPROPERTY(EditAnywhere)
		uint8 ToolType;

	// what type is the material
	UPROPERTY(EditAnywhere)
		uint8 MaterialType;

	// skeletal mesh of pickup item
	UPROPERTY(EditAnywhere)
		USkeletalMeshComponent* WieldableMesh;

	// walk into radius of pickup wieldable item
	UPROPERTY(EditAnywhere)
		UShapeComponent* PickupTrigger;

	// collision of on radius enter function
	UFUNCTION()
		void OnRadiusEnter(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
};
