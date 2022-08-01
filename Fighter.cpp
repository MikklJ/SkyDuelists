// Fill out your copyright notice in the Description page of Project Settings.


#include "Fighter.h"

// Sets default values
AFighter::AFighter()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AFighter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFighter::Tick(float DeltaTime)
{
	// Calculate forces for total linear force on plane
	CalculateAoA(Velocity);
	CalculateGravity();
	CalculateDrag(Velocity);
	CalculateLift(Velocity);
	TotalForce = Thrust + Drag + Gravity + Lift;

	// Apply total force
	ApplyTotalForce(DeltaTime);

	// Apply rotation
	ApplyTotalRotation(DeltaTime);

	GEngine->AddOnScreenDebugMessage(0, 0.f, FColor::Green,
		"Gravity " + Gravity.ToString() + "\n"
		+ "Thrust " + Thrust.ToString() + "\n"
		+ "Drag " + Drag.ToString() + "\n"
		+ "Lift " + Lift.ToString() + "\n"
		+ "Acceleration " + Acceleration.ToString() + "\n"
		+ "Velocity " + Velocity.ToString());
}
// Called to bind functionality to input
void AFighter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Throttle", this, &AFighter::ProcessThrottle);
	PlayerInputComponent->BindAxis("Pitch", this, &AFighter::ProcessPitch);
	PlayerInputComponent->BindAxis("Roll", this, &AFighter::ProcessRoll);
	PlayerInputComponent->BindAxis("Yaw", this, &AFighter::ProcessYaw);

}


// Calculate AoAs from local velocity
void AFighter::CalculateAoA(FVector LocalVelocity) {
	AoAPitch = atan2(-LocalVelocity.Z, LocalVelocity.X);
	AoAYaw = atan2(LocalVelocity.Y, LocalVelocity.X);
}

// Calculate Local Gravity force
void AFighter::CalculateGravity() {
	FVector WorldGravity = 9.8 * FVector(0, 0, -1) * Mass;
	Gravity = GetLocalVector(WorldGravity);
}

// Calculate Local Thrust Force
void AFighter::CalculateThrust(float Throttle) {
	Thrust = Throttle * MaxThrust * FVector(1, 0, 0);
}

// Calculate Local Drag Force
void AFighter::CalculateDrag(FVector LocalVelocity) {
	float VelocitySquared = LocalVelocity.Size() * LocalVelocity.Size();

	FVector CoDVector = GetCDragVector(LocalVelocity.GetSafeNormal());

	Drag = CoDVector.Size() * VelocitySquared * -LocalVelocity.GetSafeNormal();
}

void AFighter::CalculateLift(FVector LocalVelocity) {
	FVector LiftVelocity = FVector(LocalVelocity.X, LocalVelocity.Y, 0);
	float LVelocitySquared = LiftVelocity.Size() * LiftVelocity.Size();

	// Calculate main Pitch Lift force
	float CoLPitch = CoefficientOfLiftCurve->GetFloatValue(AoAPitch);
	FVector LiftForce = LVelocitySquared * CoLPitch * LiftScale * FVector(0, 0, 1);

	float IDragCoefficient = CoLPitch * CoLPitch * IDragStrength;
	FVector IDragForce = (-LiftVelocity.GetSafeNormal()) * LVelocitySquared * IDragCoefficient;

	Lift = LiftForce + IDragForce;
}

// Calculate coefficient vector based on velocity
FVector AFighter::GetCDragVector(FVector NormVelocity) {
	FVector CoD = NormVelocity;

	// Apply coefficients
	NormVelocity.X = (NormVelocity.X > 0) ? NormVelocity.X * CoefficentOfDragForward :
		NormVelocity.X * CoefficentOfDragBackward;
	NormVelocity.Y = (NormVelocity.Y > 0) ? NormVelocity.Y * CoefficentOfDragRight :
		NormVelocity.Y * CoefficentOfDragLeft;
	NormVelocity.Z = (NormVelocity.Z > 0) ? NormVelocity.Z * CoefficentOfDragUp :
		NormVelocity.Z * CoefficentOfDragDown;

	return NormVelocity;
}

void AFighter::ProcessThrottle(float InputThrottle)
{
	CalculateThrust(InputThrottle);
}

void AFighter::ProcessPitch(float InputPitch)
{
	const float TargetPitchSpeed = InputPitch * PitchRateMultiplier;
	CurrentPitchSpeed = FMath::FInterpTo(CurrentPitchSpeed, TargetPitchSpeed,
		GetWorld()->GetDeltaSeconds(), 1.0f);
}

void AFighter::ProcessYaw(float InputYaw)
{
	const float TargetYawSpeed = InputYaw * YawRateMultiplier;
	CurrentYawSpeed = FMath::FInterpTo(CurrentYawSpeed, TargetYawSpeed,
		GetWorld()->GetDeltaSeconds(), 1.0f);
}

void AFighter::ProcessRoll(float InputRoll)
{
	const float TargetRollSpeed = InputRoll * RollRateMultiplier;
	CurrentRollSpeed = FMath::FInterpTo(CurrentRollSpeed, TargetRollSpeed,
		GetWorld()->GetDeltaSeconds(), 1.0f);
}

void AFighter::ApplyTotalForce(float DeltaTime)
{
	// Delta Acceleration in m/s^2
	Acceleration = TotalForce / Mass;

	// Delta Velocity in m/s
	Velocity = Velocity + Acceleration * DeltaTime;

	// Calculate displacement in meters, apply to actor
	FVector LocalDisplacement = Velocity * DeltaTime;
	AddActorLocalOffset(LocalDisplacement * 100);
}

void AFighter::ApplyTotalRotation(float DeltaTime) 
{
	FRotator PlaneRotation = FRotator(0, 0, 0);
	PlaneRotation.Roll = CurrentRollSpeed * DeltaTime;
	PlaneRotation.Pitch = CurrentPitchSpeed * DeltaTime;
	PlaneRotation.Yaw = CurrentYawSpeed * DeltaTime;

	AddActorLocalRotation(PlaneRotation);
}

// Converts vectors in global frame to local frame
FVector AFighter::GetLocalVector(FVector WorldVector)
{
	FQuat InverseRotation = GetActorRotation().GetInverse().Quaternion();
	return InverseRotation * WorldVector;
}
