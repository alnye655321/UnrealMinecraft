// Fill out your copyright notice in the Description page of Project Settings.

#include "UnrealMinecraft.h"
#include "Block.h"


// Sets default values
ABlock::ABlock()
{
	SM_Block = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockMesh"));

	Resistance = 20.f;
	BreakingStage = 0.f;
	MinimumMaterial = 0.f;

}

// Called when the game starts or when spawned
void ABlock::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABlock::Break()
{
	++BreakingStage;

	float CrackingValue = 1.f - (BreakingStage / 5.f); // determine the value of how cracked the block will be based on what stage the block is at
	
	UMaterialInstanceDynamic* MatInstance = SM_Block->CreateDynamicMaterialInstance(0); //editing material at runtime for block breaking statges, getting the first material at 0

	if (MatInstance != nullptr) // if we successfully get the material instance
	{
		MatInstance->SetScalarParameterValue(FName("CrackingValue"), CrackingValue); // set the material to the determined crack value
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Material");
	}


	if (BreakingStage == 5.f) // are we at the final breaking stage
	{
		OnBroken(true);
	}
}

void ABlock::ResetBlock()
{
	BreakingStage = 0; // reset stage of brokenness

	UMaterialInstanceDynamic* MatInstance = SM_Block->CreateDynamicMaterialInstance(0);

	if (MatInstance != nullptr) // if we successfully get the material instance
	{
		MatInstance->SetScalarParameterValue(FName("CrackingValue"), 1.0f); // set the material to default crack value
	}


}

void ABlock::OnBroken(bool HasRequiredPickaxe)
{
	Destroy(); // destroy block
}


