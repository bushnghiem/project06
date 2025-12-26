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

// Sets default values
AGravBot::AGravBot()
{
	//Default Code from MyProjectCharacter
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

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
	CurrentVelocity = FVector(0.0f, 0.0f, 0.0f); // Set velocity to 0
	CurrentDirectionVector = FVector(0.0f, 0.0f, 0.0f); // Set direction to 0
	CurrentSpeed = 0.0f; // Set a speed to 0
	GetCharacterMovement()->MaxWalkSpeed = 0.0f;
	Acceleration = 1000.0f; // Set a default acceleration
	FrictionCoefficient = 500.0f; // Set a default friction
	BrakingAmplifier = 100.0f; // Set a default brake amplifier
	WallBounceFactor = 0.5f; // Set a default wall bounce factor
	isBraking = false;
}

// Custom Friction function 
FVector AGravBot::ApplyFrictionToVector(FVector Value, float Friction, float DeltaTime)
{
	// Ensure friction factor is positive
	if (Friction < 0.0f)
	{
		Friction = 0.0f;
	}

	float Magnitude = Value.Size();

	// If the magnitude is zero, no need to apply friction (it's already stopped)
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

	// Return the vector with the new magnitude, maintaining the direction
	return Value.GetSafeNormal() * NewMagnitude;  // Scales the vector to the reduced magnitude
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
	// Get the control rotation, which includes pitch, yaw, and roll
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
	// Ensure bounce factor is positive
	if (factor < 0.0f)
	{
		factor = 0.0f;
	}

	//Reverse direction
	const FVector NewDirection = CurrentDirectionVector * -1;
	//Set velocity in reverse direction with speed dependent in bounce factor
	CurrentVelocity = NewDirection * CurrentSpeed * factor;
}

bool AGravBot::GetIsBraking() const
{
	return isBraking;
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
	else if (GetCharacterMovement()->IsMovingOnGround() and isBraking) {
		FVector NewVelocity = ApplyFrictionToVector(CurrentVelocity, FrictionCoefficient * BrakingAmplifier, DeltaTime);
		CurrentVelocity = NewVelocity;
		FString SpeedString = FString::Printf(TEXT("Braking"));
		FColor TextColor = FColor::Green;
		float DisplayTime = 5.0f;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, DisplayTime, TextColor, SpeedString);
		}
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
	DoMove(MovementVector.X, MovementVector.Y);
}

void AGravBot::Look(const FInputActionValue& Value)
{
	// Input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// Route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AGravBot::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// Get the control rotation, which includes pitch, yaw, and roll
		const FRotator Rotation = GetController()->GetControlRotation();

		// Get the forward direction based on the full rotation (including pitch)
		const FVector ForwardDirection = FRotationMatrix(Rotation).GetUnitAxis(EAxis::X);

		// Get the right direction based on the full rotation (including pitch)
		const FVector RightDirection = FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y);

		// Add movement 
		FVector DesiredMovement = ForwardDirection * Forward + RightDirection * Right;

		// Update velocity based on the desired movement and acceleration
		CurrentVelocity += DesiredMovement * Acceleration * GetWorld()->GetDeltaSeconds();

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
	Jump();
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
	// Reverses characters gravity direction
	FVector CurrentGravity = GetCharacterMovement()->GetGravityDirection();
	GetCharacterMovement()->SetGravityDirection(CurrentGravity * -1);
}

