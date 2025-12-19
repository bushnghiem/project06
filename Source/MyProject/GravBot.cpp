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
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Configure character movement
	//GetCharacterMovement()->bOrientRotationToMovement = true;
	//GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

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

	CurrentVelocity = FVector(0.0f, 0.0f, 0.0f); // Set velocity to 0
	CurrentDirectionVector = FVector(0.0f, 0.0f, 0.0f); // Set direction to 0
	CurrentSpeed = 0.0f; // Set a speed to 0
	GetCharacterMovement()->MaxWalkSpeed = 0.0f;
	Acceleration = 1000.0f; // Set a default acceleration
	FrictionCoefficient = 500.0f; // Set a default friction
	isSpinning = false;
}

FVector AGravBot::ApplyFrictionToVector(FVector Value, float Friction, float DeltaTime)
{
	// Ensure friction factor is positive
	if (Friction < 0.0f)
	{
		Friction = 0.0f;
	}

	// Get the magnitude (length) of the vector
	float Magnitude = Value.Size();

	// If the magnitude is zero, no need to apply friction (it's already stopped)
	if (Magnitude == 0.0f)
	{
		return Value;
	}

	// Reduce the magnitude by the friction coefficient
	float NewMagnitude = Magnitude - (FrictionCoefficient * DeltaTime);

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

FVector AGravBot::GetCurrentVelocity() const
{
	return CurrentVelocity;
}

bool AGravBot::GetIsSpinning() const
{
	return false;
}

// Called every frame
void AGravBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (GetCharacterMovement()->IsMovingOnGround()) {
		FVector NewVelocity = ApplyFrictionToVector(CurrentVelocity, FrictionCoefficient, DeltaTime);
		CurrentVelocity = NewVelocity;
	}
	CurrentDirectionVector = CurrentVelocity;
	CurrentDirectionVector.Normalize();
	CurrentSpeed = CurrentVelocity.Size();
	GetCharacterMovement()->MaxWalkSpeed = CurrentSpeed;
	AddMovementInput(CurrentDirectionVector, CurrentSpeed);

	

	FString SpeedString = FString::Printf(TEXT("Speed: %.1f units/sec"), GetCharacterMovement()->MaxWalkSpeed);
	FColor TextColor = FColor::Green;  // Text color can be any color
	float DisplayTime = 5.0f;  // Time in seconds the message will stay on screen
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, DisplayTime, TextColor, SpeedString);
	}

}



// Called to bind functionality to input
void AGravBot::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGravBot::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AGravBot::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGravBot::Look);
	}

}

void AGravBot::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AGravBot::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AGravBot::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		//AddMovementInput(ForwardDirection, Forward);
		//AddMovementInput(RightDirection, Right);
		FVector DesiredMovement = ForwardDirection * Forward + RightDirection * Right;

		// Update velocity based on the desired movement and acceleration
		CurrentVelocity += DesiredMovement * Acceleration * GetWorld()->GetDeltaSeconds();

	}
}

void AGravBot::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AGravBot::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AGravBot::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

