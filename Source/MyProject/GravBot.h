// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GravBot.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

UCLASS()
class MYPROJECT_API AGravBot : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected:

	/** Break Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* FlipAction;

	/** Break Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* BrakeAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseLookAction;

public:
	// Sets default values for this character's properties
	AGravBot();

	// Declare all the movement variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FVector CurrentVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FVector CurrentDirectionVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float CurrentSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float Acceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float FrictionCoefficient;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float BrakingAmplifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WallBounceFactor;

	// Declare the friction function
	FVector ApplyFrictionToVector(FVector Value, float Friction, float DeltaTime);

	bool isBraking;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	// Setter and getter functions to get CurrentVelocity
	UFUNCTION(BlueprintCallable, Category = "Movement")
	FVector GetCurrentVelocity() const;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetCurrentVelocity(FVector NewVector);

	// Function that changes velocity direction to camera direction
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void RealignMovement();

	// Getter functions to get WallBounceFactor
	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetWallBounceFactor() const;

	// Function that reverses velocity direction 
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void WallBounce(float factor);

	// Getter function to get isBraking
	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool GetIsBraking() const;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	/** Handles break pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoBrakeStart();

	/** Handles break pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoBrakeEnd();

	/** Handles break pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoFlip();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
