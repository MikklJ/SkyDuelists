// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Math/Rotator.h"
#include "Math/Quat.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Fighter.generated.h"

UCLASS()
class SKYDUELISTS_API AFighter : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AFighter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditDefaultsOnly, Category = Curve)
		UCurveFloat* CoefficientOfLiftCurve;

	// Input process functions
	void ProcessThrottle(float InputThrottle);
	void ProcessPitch(float InputPitch);
	void ProcessYaw(float InputYaw);
	void ProcessRoll(float InputRoll);

private:
	// DYNAMIC ATTRIBUTES

	// Dynamic forces, in kg * m / s^2, local
	FVector TotalForce = FVector::ZeroVector;	// Sum of all forces
	FVector Thrust = FVector::ZeroVector;		// Thrust, forward of nose
	FVector Drag = FVector::ZeroVector;			// Drag, acts against velocity
	FVector Gravity = FVector::ZeroVector;		// Gravity, pulls down world -z
	FVector PitchLift = FVector::ZeroVector;	// Lift, acts perpendicular to planar velocity
	FVector YawLift = FVector::ZeroVector;

	// Dynamic mechanics, in SI, global
	FVector Velocity = FVector::ZeroVector;		// Velocity in m / s, global
	FVector Acceleration = FVector::ZeroVector;	// Acceleration in m / s^2, global

	// Angle of Attack and Rotation
	float AoAPitch = 0.0f;						// Current AoA of plane in xz plane
	float AoAYaw = 0.0f;						// Current AoA of plane in xy plane
	float CurrentPitchSpeed;					// Current pitch rate of plane
	float CurrentRollSpeed;						// Current roll rate of plane
	float CurrentYawSpeed;						// Current yaw rate of plane


	// STATIC ATTRIBUTES

	// Rotational Attributes (response rating)
	float PitchRateMultiplier = 150.0f;
	float RollRateMultiplier = 250.0f;
	float YawRateMultiplier = 30.0f;

	float Mass = 5500.0f;						// Mass, in kg
	float MaxThrust = 45000.0f;					// Maximum thrust in Newtons
	float LiftScalePitch = 150.0f;				// Coefficient lift strength
	float LiftScaleYaw = 60.0f;
	float IDragPitchStrength = 0.2f;
	float IDragYawStrength = 0.2f;

	// Coefficients of Drag, 6-scaled
	float CoefficentOfDragForward = 1.1f;
	float CoefficentOfDragBackward = 1.1f;
	float CoefficentOfDragLeft = 8.7f;
	float CoefficentOfDragRight = 8.7f;
	float CoefficentOfDragUp = 15.6f;
	float CoefficentOfDragDown = 15.6f;


	// METHODS

	// Calculation
	void CalculateAoA(FVector LocalVelocity);
	void CalculateGravity();
	void CalculateThrust(float Throttle);
	void CalculateDrag(FVector LocalVelocity);
	void CalculatePitchLift(FVector LocalVelocity);
	void CalculateYawLift(FVector LocalVelocity);

	// Applies current Force to displace object
	void ApplyTotalForce(float DeltaTime);

	// Applies current rotation rates
	void ApplyTotalRotation(float DeltaTime);

	FVector GetLocalVector(FVector WorldVector);
	FVector GetGlobalVector(FVector LocalVector);
	FVector GetCDragVector(FVector NormVelocity);

	void DrawDebugForce(FVector ForceVector, FColor Color);
};
