// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Block.generated.h"

UCLASS()
class UNREALMINECRAFT_API ABlock : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABlock();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly)
		UStaticMeshComponent* SM_Block;

	uint8 MinimumMaterial; // level of pickaxe
	
	UPROPERTY(EditDefaultsOnly)
		float Resistance; // how fast block can break

	UPROPERTY(BlueprintReadWrite)
		float BreakingStage; // block will break in stages, starts at 0

	void Break(); // called every time we want to break block down another stage

	void ResetBlock(); // reset breaking stages

	void OnBroken(bool HasRequiredPickaxe); // called when block is fully broken, if player has the right level pickaxe
	
};
