// Fill out your copyright notice in the Description page of Project Settings.


#include "GravBot.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "MyProject.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

// Sets default values
AGravBot::AGravBot()
{
	// Default Code from MyProjectCharacter
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(24.f, 96.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// Set the default values for new custom movement variables
	CurrentVelocity = FVector(0.0f, 0.0f, 0.0f);
	CurrentDirectionVector = FVector(0.0f, 0.0f, 0.0f);
	CurrentSpeed = 0.0f;
	GetCharacterMovement()->MaxWalkSpeed = 0.0f;
	Acceleration = 1000.0f;
	FrictionCoefficient = 500.0f;
	BrakingAmplifier = 10.0f;
	WallBounceFactor = 0.2f;
	isBraking = false;
	FlipHack = false;
	TouchingFlipPad = false;
	Working = true;
}

// Custom Friction function 
FVector AGravBot::ApplyFrictionToVector(FVector Value, float Friction, float DeltaTime)
{
	if (Friction < 0.0f)
	{
		Friction = 0.0f;
	}

	float Magnitude = Value.Size();

	if (Magnitude == 0.0f)
	{
		return Value;
	}

	// Reduce the magnitude by the friction coefficient
	float NewMagnitude = Magnitude - (Friction * DeltaTime);

	// Ensure the magnitude doesn't go below zero (i.e., no reverse velocity)
	if (NewMagnitude < 0.0f)
	{
		NewMagnitude = 0.0f;
	}

	return Value.GetSafeNormal() * NewMagnitude;
}

// Called when the game starts or when spawned
void AGravBot::BeginPlay()
{
	Super::BeginPlay();
	
}
// Getter and setter for CurrentVelocity
FVector AGravBot::GetCurrentVelocity() const
{
	return CurrentVelocity;
}

void AGravBot::SetCurrentVelocity(FVector NewVector)
{
	CurrentVelocity = NewVector;
}
// Function that realigns velocity to camera direction, used for when player switches gravity direction by jumping on wall
void AGravBot::RealignMovement()
{
	const FRotator Rotation = GetController()->GetControlRotation();

	// Get the forward direction based on the full rotation (including pitch)
	const FVector NewDirection = FRotationMatrix(Rotation).GetUnitAxis(EAxis::X);

	CurrentVelocity = NewDirection * CurrentSpeed;
}

float AGravBot::GetWallBounceFactor() const
{
	return WallBounceFactor;
}

void AGravBot::WallBounce(float factor)
{
	if (factor < 0.0f)
	{
		factor = 0.0f;
	}

	//Reverse direction
	const FVector NewDirection = CurrentDirectionVector * -1;
	CurrentVelocity = NewDirection * CurrentSpeed * factor;
}

bool AGravBot::GetIsBraking() const
{
	return isBraking;
}

bool AGravBot::GetFlipHack() const
{
	return FlipHack;
}

void AGravBot::SetWorking(bool value)
{
	Working = value;
}

int AGravBot::GetJumpCount() const
{
	return JumpCounter;
}

int AGravBot::GetFlipCount() const
{
	return FlipCounter;
}


void AGravBot::SetFlipHack(bool value)
{
	FlipHack = value;
}

bool AGravBot::GetTouchFlipPad() const
{
	return TouchingFlipPad;
}

void AGravBot::SetTouchFlipPad(bool value)
{
	TouchingFlipPad = value;
}

// Called every frame
void AGravBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Applies friction is on ground and braking amplifier if brake is pressed
	if (GetCharacterMovement()->IsMovingOnGround() and !isBraking) {
		FVector NewVelocity = ApplyFrictionToVector(CurrentVelocity, FrictionCoefficient, DeltaTime);
		CurrentVelocity = NewVelocity;
	}
	else if (GetCharacterMovement()->IsMovingOnGround() and isBraking and (CurrentSpeed > 0)) {
		FVector NewVelocity = ApplyFrictionToVector(CurrentVelocity, FrictionCoefficient * BrakingAmplifier, DeltaTime);
		CurrentVelocity = NewVelocity;
		if (BrakeSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				BrakeSound,
				GetActorLocation()
			);
		}
		/*
		FString SpeedString = FString::Printf(TEXT("Braking"));
		FColor TextColor = FColor::Green;
		float DisplayTime = 5.0f;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, DisplayTime, TextColor, SpeedString);
		}
		*/
	}
	// Custom movement for the Gravbot
	CurrentDirectionVector = CurrentVelocity;
	CurrentDirectionVector.Normalize();
	CurrentSpeed = CurrentVelocity.Size();
	GetCharacterMovement()->MaxWalkSpeed = CurrentSpeed;
	AddMovementInput(CurrentDirectionVector, CurrentSpeed);
}



// Called to bind functionality to input
void AGravBot::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Flipping
		EnhancedInputComponent->BindAction(FlipAction, ETriggerEvent::Started, this, &AGravBot::DoFlip);

		// Braking
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &AGravBot::DoBrakeStart);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &AGravBot::DoBrakeEnd);

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AGravBot::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AGravBot::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGravBot::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AGravBot::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGravBot::Look);
	}

}

void AGravBot::Move(const FInputActionValue& Value)
{
	// Input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// Route the input
	if (Working)
	{
		DoMove(MovementVector.X, MovementVector.Y);
	}
}

void AGravBot::Look(const FInputActionValue& Value)
{
	// Input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// Route the input
	if (Working)
	{
		DoLook(LookAxisVector.X, LookAxisVector.Y);
	}
}

// This is where the movement with different gravity directions and camera orientations gets done
void AGravBot::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// Get the control rotation, which includes pitch, yaw, and roll
		const FRotator Rotation = GetController()->GetControlRotation();

		// Get the gravity direction
		const FVector GravityDirection = GetCharacterMovement()->GetGravityDirection();
		const FVector GravityNorm = GravityDirection.GetSafeNormal();

		// Get the forward and right vectors from the camera
		const FVector ForwardDirection = FRotationMatrix(Rotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y);

		// Create Vectors for the adjusted forward/right directions relative to gravity
		FVector AdjustedForward = FVector::ZeroVector;
		FVector AdjustedRight = FVector::ZeroVector;

		// If gravity is aligned with Z-axis, aka normal gravity
		if (FVector::DotProduct(GravityNorm, FVector::UpVector) > 0.99f || FVector::DotProduct(GravityNorm, FVector::UpVector) < -0.99f)
		{
			// Don't change anything, with normal gravity following the camera works
			AdjustedForward = ForwardDirection;
			AdjustedRight = RightDirection;
		}
		// Gravity is not aligned with Z-axis, aka walking on walls where gravity is X-axis/Y-axis
		else
		{
			// Create a plane perpendicular to the gravity direction
			FVector RightPlane = FVector::CrossProduct(GravityNorm, FVector::UpVector).GetSafeNormal();
			FVector ForwardPlane = FVector::CrossProduct(GravityNorm, RightPlane).GetSafeNormal();

			// Project the camera's forward and right directions onto the gravity plane
			AdjustedForward = FVector::VectorPlaneProject(ForwardDirection, GravityNorm);
			AdjustedRight = FVector::VectorPlaneProject(RightDirection, GravityNorm);
			AdjustedForward.Normalize();
			AdjustedRight.Normalize();

			// Now ensure the left/right movement aligns to the gravity plane since cameras left/right is actually X-axis/Y-axis, not Z-axis
			FVector GravityRight = FVector::CrossProduct(GravityNorm, AdjustedForward).GetSafeNormal();
			AdjustedRight = GravityRight * -1;
		}

		// Make the character move in the desired direction
		FVector DesiredMovement = AdjustedForward * Forward + AdjustedRight * Right;
		DesiredMovement = DesiredMovement.GetSafeNormal();
		CurrentVelocity += DesiredMovement * Acceleration * GetWorld()->GetDeltaSeconds();
		

		/*
		if (GEngine)
		{
			FString DebugText = FString::Printf(TEXT("Controller Rotation: %s, Gravity: %s, Right: %s, Forward: %s"),
				*Rotation.ToString(),
				*GravityDirection.ToString(),
				*AdjustedRight.ToString(),
				*AdjustedForward.ToString());
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, DebugText);
		}
		*/
	}
}





void AGravBot::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// Add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AGravBot::DoJumpStart()
{
	// Signal the character to jump
	// Reverses characters gravity direction if touching Flip Pad
	if (Working)
	{
		if (TouchingFlipPad)
		{
			FVector CurrentGravity = GetCharacterMovement()->GetGravityDirection();
			GetCharacterMovement()->SetGravityDirection(CurrentGravity * -1);
			FlipCounter++;
		}
		// Jump Normally if not touching Flip Pad
		else
		{
			if (CanJump())
			{
				if (JumpSound)
				{
					UGameplayStatics::PlaySoundAtLocation(
						this,
						JumpSound,
						GetActorLocation()
					);
				}
				JumpCounter++;
			}
			Jump();
		}
	}
}

void AGravBot::DoJumpEnd()
{
	// Signal the character to stop jumping
	StopJumping();
}

void AGravBot::DoBrakeStart()
{
	isBraking = true;
}

void AGravBot::DoBrakeEnd()
{
	isBraking = false;
}

void AGravBot::DoFlip()
{
	if (FlipHack && GetCharacterMovement()->IsMovingOnGround())
	{
		if (FlipSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				FlipSound,
				GetActorLocation()
			);
		}
		// Reverses characters gravity direction if FlipHack is true
		FVector CurrentGravity = GetCharacterMovement()->GetGravityDirection();
		GetCharacterMovement()->SetGravityDirection(CurrentGravity * -1);
		FlipCounter++;
	}
}

