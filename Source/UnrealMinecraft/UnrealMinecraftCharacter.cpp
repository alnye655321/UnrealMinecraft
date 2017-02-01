// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealMinecraft.h"
#include "UnrealMinecraftCharacter.h"
#include "UnrealMinecraftProjectile.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/HeadMountedDisplayFunctionLibrary.h"
#include "MotionControllerComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AUnrealMinecraftCharacter

AUnrealMinecraftCharacter::AUnrealMinecraftCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->RelativeLocation = FVector(-39.56f, 1.75f, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->RelativeRotation = FRotator(1.9f, -19.19f, 5.2f);
	Mesh1P->RelativeLocation = FVector(-0.5f, -4.4f, -155.7f);

	// Create a gun mesh component
	FP_WieldedItem = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_WieldedItem"));
	FP_WieldedItem->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_WieldedItem->bCastDynamicShadow = false;
	FP_WieldedItem->CastShadow = false;
	// FP_WieldedItem->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_WieldedItem->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_WieldedItem);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 30.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_WieldedItem, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->Hand = EControllerHand::Right;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetupAttachment(R_MotionController);
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));		// Counteract the rotation of the VR gun model.

	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;

	Reach = 250.f; // the characters reach for a raycast
}

void AUnrealMinecraftCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_WieldedItem->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		VR_Gun->SetHiddenInGame(false, true);
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		VR_Gun->SetHiddenInGame(true, true);
		Mesh1P->SetHiddenInGame(false, true);
	}
}

void AUnrealMinecraftCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckForBlocks(); // check for blocks infront of player on each frame
}

//////////////////////////////////////////////////////////////////////////
// Input

void AUnrealMinecraftCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	//InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AUnrealMinecraftCharacter::TouchStarted);
	if (EnableTouchscreenMovement(PlayerInputComponent) == false)
	{
		PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AUnrealMinecraftCharacter::OnHit); // called OnHit() when left mouse button is pressed
		PlayerInputComponent->BindAction("Interact", IE_Released, this, &AUnrealMinecraftCharacter::EndHit); // called EndHit() when left mouse button is released
	}

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AUnrealMinecraftCharacter::OnResetVR);

	PlayerInputComponent->BindAxis("MoveForward", this, &AUnrealMinecraftCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AUnrealMinecraftCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AUnrealMinecraftCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AUnrealMinecraftCharacter::LookUpAtRate);
}

void AUnrealMinecraftCharacter::OnFire()
{
	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AUnrealMinecraftCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AUnrealMinecraftCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AUnrealMinecraftCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnFire();
	}
	TouchItem.bIsPressed = false;
}

void AUnrealMinecraftCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if ((TouchItem.bIsPressed == true) && (TouchItem.FingerIndex == FingerIndex))
	{
		if (TouchItem.bIsPressed)
		{
			if (GetWorld() != nullptr)
			{
				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
				if (ViewportClient != nullptr)
				{
					FVector MoveDelta = Location - TouchItem.Location;
					FVector2D ScreenSize;
					ViewportClient->GetViewportSize(ScreenSize);
					FVector2D ScaledDelta = FVector2D(MoveDelta.X, MoveDelta.Y) / ScreenSize;
					if (FMath::Abs(ScaledDelta.X) >= 4.0 / ScreenSize.X)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.X * BaseTurnRate;
						AddControllerYawInput(Value);
					}
					if (FMath::Abs(ScaledDelta.Y) >= 4.0 / ScreenSize.Y)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.Y * BaseTurnRate;
						AddControllerPitchInput(Value);
					}
					TouchItem.Location = Location;
				}
				TouchItem.Location = Location;
			}
		}
	}
}

void AUnrealMinecraftCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AUnrealMinecraftCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AUnrealMinecraftCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AUnrealMinecraftCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AUnrealMinecraftCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	bool bResult = false;
	if (FPlatformMisc::GetUseVirtualJoysticks() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		bResult = true;
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AUnrealMinecraftCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AUnrealMinecraftCharacter::EndTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AUnrealMinecraftCharacter::TouchUpdate);
	}
	return bResult;
}

void AUnrealMinecraftCharacter::OnHit() // called after left mouse button is pressed
{
	PlayHitAnim(); // play the hit animation

	if (CurrentBlock != nullptr) // if player is looking at a block
	{
		bIsBreaking = true; // player is breaking block

		float TimeBetweenBreaks = ((CurrentBlock->Resistance) / 100.f) / 2; // strength of weapon dictates how fast player breaks the blocks

		GetWorld()->GetTimerManager().SetTimer(BlockBreakingHandle, this, &AUnrealMinecraftCharacter::BreakBlock, TimeBetweenBreaks, true); // timer for breaking blocks, calls BreakBlock()
		GetWorld()->GetTimerManager().SetTimer(HitAnimHandle, this, &AUnrealMinecraftCharacter::PlayHitAnim, 0.4f, true); // timer for swing animation, calls PlayHitAnim(), harded carded to 0.4 sec
	}
}

void AUnrealMinecraftCharacter::EndHit()
{
	GetWorld()->GetTimerManager().ClearTimer(BlockBreakingHandle); // clear both timers
	GetWorld()->GetTimerManager().ClearTimer(HitAnimHandle);

	bIsBreaking = false; // no longer breaking blocks

	if (CurrentBlock != nullptr) // if player is looking at a block
	{
		CurrentBlock->ResetBlock(); // reset the block if player is still looking at it
	}
}

void AUnrealMinecraftCharacter::PlayHitAnim()
{
	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AUnrealMinecraftCharacter::BreakBlock()
{
	if (bIsBreaking && CurrentBlock != nullptr && !CurrentBlock->IsPendingKill())
	{
		CurrentBlock->Break(); // break the block if player is looking at it, not already breaking, and hasnt already killed it
	}
}

void AUnrealMinecraftCharacter::CheckForBlocks()
{
	FHitResult LinetraceHit;

	FVector StartTrace = FirstPersonCameraComponent->GetComponentLocation(); // start of raycast
	FVector EndTrace = (FirstPersonCameraComponent->GetForwardVector() * Reach) + StartTrace; // end of raycast

	FCollisionQueryParams CQP;
	CQP.AddIgnoredActor(this); // ignore the player in the raycast collision

	GetWorld()->LineTraceSingleByChannel(LinetraceHit, StartTrace, EndTrace, ECollisionChannel::ECC_WorldDynamic, CQP);

	ABlock* PotentialBlock = Cast<ABlock>(LinetraceHit.GetActor()); // whatever actor the player hits with the raycast see if its a block

	if (PotentialBlock != CurrentBlock && CurrentBlock != nullptr) // has player stopped looking at block, if so reset the block
	{
		CurrentBlock->ResetBlock();
	}

	if (PotentialBlock == NULL) //not a block
	{
		CurrentBlock = nullptr;
		return;
	}
	else
	{
		if (CurrentBlock != nullptr && !bIsBreaking)
		{
			CurrentBlock->ResetBlock();
		}
		CurrentBlock = PotentialBlock;
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, *CurrentBlock->GetName()); // log actor
	}
}
