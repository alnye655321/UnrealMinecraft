// Fill out your copyright notice in the Description page of Project Settings.

#include "UnrealMinecraft.h"
#include "Wieldable.h"
#include "UnrealMinecraftCharacter.h"


// Sets default values
AWieldable::AWieldable()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WieldableMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WieldableMesh")); // can be attached to player

	PickupTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("PickupTrigger")); // set as UBoxComponent inherited from UShapeComponent in header file, can be changed here to any shape

	PickupTrigger->bGenerateOverlapEvents = true;

	PickupTrigger->OnComponentBeginOverlap.AddDynamic(this, &AWieldable::OnRadiusEnter);  // call OnRadiusEnter() below when a collision overlap occurs
	PickupTrigger->AttachTo(WieldableMesh); // attach collision box to wieldable pickup
	//PickupTrigger->AttachToComponent(WieldableMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale); // updated syntax

	MaterialType = EMaterial::None;
	ToolType = ETool::Unarmed;

}

// Called when the game starts or when spawned
void AWieldable::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWieldable::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	// make the wieldable item spin in a loop in the game
	FRotator rotation = WieldableMesh->GetComponentRotation(); // get the current rotation
	rotation.Yaw += 1.f; // rotatle slightly on yaw axis, 1 degree
	WieldableMesh->SetRelativeRotation(rotation);
}

void AWieldable::OnRadiusEnter(class UPrimitiveComponent* HitComp, AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	AUnrealMinecraftCharacter* Character = Cast<AUnrealMinecraftCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)); // get reference to player character

	Character->FP_WieldedItem->SetSkeletalMesh(WieldableMesh->SkeletalMesh); // get publically available wielded_item and attach to character skeletel mesh

	Character->MaterialType = MaterialType; // set character values

	Character->ToolType = ToolType;

	Destroy(); // destroy the pickup, does not need to be in world anymore
}


