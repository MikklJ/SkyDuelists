// Core Fighter Physics Module
// MikklJ 2022

#include "Fighter.h"

// Sets default values
AFighter::AFighter()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	FighterMesh = CreateDefaultSubobject<USkeletalMeshComponent>("Fighter Mesh");

	CameraArm = CreateDefaultSubobject<USpringArmComponent>("Camera Arm");
	CameraArm->SetupAttachment(FighterMesh);

	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(CameraArm);
}

// Called when the game starts or when spawned
void AFighter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFighter::Tick(float DeltaTime)
{
	// Get Local Velocity
	FVector LocalVelocity = GetLocalVector(Velocity);

	// Calculate angles of attack for lift forces
	CalculateAoA(LocalVelocity);

	// Calculate local forces for total linear force on plane
	CalculateGravity();
	CalculateDrag(LocalVelocity);
	CalculatePitchLift(LocalVelocity);
	CalculateYawLift(LocalVelocity);

	// Add forces for total force, apply to physics mechanics
	TotalForce = Thrust + Drag + Gravity + PitchLift + YawLift;
	ApplyTotalForce(DeltaTime);

	// Apply rotation from controller input
	if (!Freecam || !LockRotationInFreecam) {
		ApplyTotalRotation(DeltaTime);
	}

	if (Debug) {
		DisplayDebugInfo();
	}
}

// Called to bind functionality to input
void AFighter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Freecam", IE_Pressed, this, &AFighter::OnBeginFreecam);
	PlayerInputComponent->BindAction("Freecam", IE_Released, this, &AFighter::OnEndFreecam);

	PlayerInputComponent->BindAxis("Throttle", this, &AFighter::ProcessThrottle);
	PlayerInputComponent->BindAxis("Pitch", this, &AFighter::ProcessPitch);
	PlayerInputComponent->BindAxis("Roll", this, &AFighter::ProcessRoll);
	PlayerInputComponent->BindAxis("Yaw", this, &AFighter::ProcessYaw);

	PlayerInputComponent->BindAxis("FreecamPitch", this, &AFighter::ProcessFreecamPitch);
	PlayerInputComponent->BindAxis("FreecamYaw", this, &AFighter::ProcessFreecamYaw);

}

// Calculate AoAs from local velocity, for pitch and yaw
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

	// Varies final coefficient size with orientation of plane w.r.t velocity
	FVector CoDVector = GetCDragVector(LocalVelocity.GetSafeNormal());

	Drag = CoDVector.Size() * VelocitySquared * -LocalVelocity.GetSafeNormal();
}

// Calculate Local Lift on Pitch Orientation
void AFighter::CalculatePitchLift(FVector LocalVelocity) {

	// Project velocity onto XZ plane, find v^2
	FVector LiftVelocity = FVector(LocalVelocity.X, 0, LocalVelocity.Z);
	float LVelocitySquared = LiftVelocity.Size() * LiftVelocity.Size();

	// Calculate main pitch lift force from AoA
	float CoLPitch = CoefficientOfLiftCurve->GetFloatValue(AoAPitch);
	FVector LiftForce = LVelocitySquared * CoLPitch * LiftScalePitch * FVector(0, 0, 1);

	// Calculate induced drag
	float IDragCoefficient = CoLPitch * CoLPitch * IDragPitchStrength;
	FVector IDragForce = (-LiftVelocity.GetSafeNormal()) * LVelocitySquared * IDragCoefficient;

	PitchLift = LiftForce + IDragForce;
}

// Calculate Local Lift on Yaw Orientation
void AFighter::CalculateYawLift(FVector LocalVelocity) {

	// Project velocity onto XY plane, find v^2
	FVector LiftVelocity = FVector(LocalVelocity.X, LocalVelocity.Y, 0);
	float LVelocitySquared = LiftVelocity.Size() * LiftVelocity.Size();

	// Calculate main yaw lift force from AoA
	float CoLYaw = CoefficientOfLiftCurve->GetFloatValue(AoAYaw);
	FVector LiftForce = LVelocitySquared * CoLYaw * LiftScaleYaw * FVector(0, -1, 0);

	// Calculate induced drag
	float IDragCoefficient = CoLYaw * CoLYaw * IDragYawStrength;
	FVector IDragForce = (-LiftVelocity.GetSafeNormal()) * LVelocitySquared * IDragCoefficient;

	YawLift = LiftForce + IDragForce;
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

void AFighter::OnBeginFreecam()
{
	Freecam = true;
	
}

void AFighter::OnEndFreecam()
{
	Freecam = false;
	if (CameraArm) {
		CameraArm->SetRelativeRotation(FRotator(0, 0, 0));
	}
}

// Generate thrust force given input throttle scale
void AFighter::ProcessThrottle(float InputThrottle)
{
	CalculateThrust(InputThrottle);
}

// Generates pitch rotation
void AFighter::ProcessPitch(float InputPitch)
{
	const float TargetPitchSpeed = InputPitch * PitchRateMultiplier;
	CurrentPitchSpeed = FMath::FInterpTo(CurrentPitchSpeed, TargetPitchSpeed,
		GetWorld()->GetDeltaSeconds(), 1.0f);
}

// Generates yaw rotation
void AFighter::ProcessYaw(float InputYaw)
{
	const float TargetYawSpeed = InputYaw * YawRateMultiplier;
	CurrentYawSpeed = FMath::FInterpTo(CurrentYawSpeed, TargetYawSpeed,
		GetWorld()->GetDeltaSeconds(), 1.0f);
}

// Generates x-axis roll
void AFighter::ProcessRoll(float InputRoll)
{
	const float TargetRollSpeed = InputRoll * RollRateMultiplier;
	CurrentRollSpeed = FMath::FInterpTo(CurrentRollSpeed, TargetRollSpeed,
		GetWorld()->GetDeltaSeconds(), 1.0f);
}

void AFighter::ProcessFreecamPitch(float InputFreecamPitch)
{
	if (Freecam) {
		FRotator CameraRotation = FRotator(InputFreecamPitch * CameraSpeed, 0, 0);
		CameraArm->AddRelativeRotation(CameraRotation);
	}
}

void AFighter::ProcessFreecamYaw(float InputFreecamYaw)
{
	if (Freecam) {
		FRotator CameraRotation = FRotator(0, InputFreecamYaw * CameraSpeed, 0);
		CameraArm->AddRelativeRotation(CameraRotation);
	}
}

// Applies calculated forces onto plane mechanics
void AFighter::ApplyTotalForce(float DeltaTime)
{
	// Get world acceleration in m/s^2
	Acceleration = GetGlobalVector(TotalForce) / Mass;

	// Change velocity in m/s
	Velocity = Velocity + Acceleration * DeltaTime;

	// Calculate displacement in centimeters and apply to actor
	FVector LocalDisplacement = 100 *Velocity * DeltaTime;
	AddActorWorldOffset(LocalDisplacement);
}

// Creates rotator to generate local rotation on plane
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
	FQuat InverseRotationQuat = GetActorRotation().GetInverse().Quaternion();
	return InverseRotationQuat * WorldVector;
}

// Converts vectors in local frame to global frame
FVector AFighter::GetGlobalVector(FVector LocalVector)
{
	FQuat RotationQuat = GetActorRotation().Quaternion();
	return RotationQuat * LocalVector;
}

// Shows debug info on screen
void AFighter::DisplayDebugInfo()
{
	// Display force and physics values
	if (Debug)
		GEngine->AddOnScreenDebugMessage(0, 0.f, FColor::Green,
			"Gravity " + Gravity.ToString() + "\n"
			+ "Thrust " + Thrust.ToString() + "\n"
			+ "Drag " + Drag.ToString() + "\n"
			+ "Pitch Lift" + PitchLift.ToString() + "\n"
			+ "Yaw Lift" + YawLift.ToString() + "\n"
			+ "Acceleration " + Acceleration.ToString() + "\n"
			+ "Velocity " + Velocity.ToString() + "\n");


	// Draw force vectors
	DrawDebugForce(GetGlobalVector(Gravity), FColor(0, 255, 0));
	DrawDebugForce(GetGlobalVector(Drag), FColor(255, 0, 0));
	DrawDebugForce(GetGlobalVector(PitchLift), FColor(0, 0, 255));
	DrawDebugForce(GetGlobalVector(YawLift), FColor(255, 165, 0));
	DrawDebugForce(GetGlobalVector(Thrust), FColor(255, 255, 0));
}

// Draws a debug line for forces
void AFighter::DrawDebugForce(FVector ForceVector, FColor Color)
{
	DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + ForceVector, Color,
		false, 1, 1, 10);
}