// Fill out your copyright notice in the Description page of Project Settings.

#include "SceneLuminanceCaptureComponent.h"

#include "Engine/TextureRenderTargetCube.h"
#include "Kismet/GameplayStatics.h"

#pragma optimize("", off)

USceneLuminanceCaptureComponent::USceneLuminanceCaptureComponent()
{
	bCaptureEveryFrame = false;
	bCaptureOnMovement = false;
	bAlwaysPersistRenderingState = true;
	PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	ProfilingEventName = "SceneLuminanceCapture";
	MaxViewDistanceOverride = 2000.0f;
	LODDistanceFactor = 10.0f;
}

void USceneLuminanceCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(TextureTarget))
		return;

	TextureTarget->bHDR = true;
	TextureTarget->OverrideFormat = PF_FloatRGB;
	TextureTarget->ClearColor = FLinearColor(1, 1, 1, 1);
	TextureTarget->MipGenSettings = TMGS_NoMipmaps;
	TextureTarget->MipLoadOptions = ETextureMipLoadOptions::Default;
	TextureTarget->UpdateResourceImmediate();
}

inline void USceneLuminanceCaptureComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USceneLuminanceCaptureComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateCapture();
}

void USceneLuminanceCaptureComponent::TryRefreshCapture()
{
	float time = IsValid(GetWorld()) ? UGameplayStatics::GetRealTimeSeconds(this) : 0;
	if (time - CaptureTimestamp > CaptureRefreshTime ||
		FVector::Dist(GetComponentLocation(), LastCaptureLocation) > CaptureRefreshDistance)
		UpdateCapture();
}

void USceneLuminanceCaptureComponent::UpdateCapture()
{
	if (!IsValid(TextureTarget))
		return;
	
	float time = IsValid(GetWorld()) ? UGameplayStatics::GetRealTimeSeconds(this) : 0;
	CaptureTimestamp = time;
	LastCaptureLocation = GetComponentLocation();
	CaptureScene();

	auto t = TextureTarget->GameThread_GetRenderTargetResource();
	if (!t)
		return;
	auto cube = t->GetTextureRenderTargetCubeResource();
	if (!cube)
		return;
	
	SizeX = cube->GetSizeX();
	SizeY = cube->GetSizeY();
	int pixelCount = SizeX * SizeY;
	
	// Allocate arrays
	if (Sides.Num() != 6)
		Sides.SetNum(6);
	for (auto& p : Sides)
		if (p.Num() != pixelCount)
			p.SetNum(pixelCount);

	for (uint8 i = 0; i < CubeFace_MAX; i++)
		t->ReadLinearColorPixels(Sides[i], FReadSurfaceDataFlags(RCM_UNorm, static_cast<ECubeFace>(i)));
}

FLinearColor USceneLuminanceCaptureComponent::Sample(const FVector& InDirection) const
{
	// Convert 3D direction to 2D cubemap uv
	auto uv = DirToFaceUV(InDirection);
	const int x = uv.Key.X * SizeX;
	const int y = uv.Key.Y * SizeY;
	const int i = x + y * SizeX;
	if (Sides.IsValidIndex(uv.Value))
		if (Sides[uv.Value].IsValidIndex(i))
			return Sides[uv.Value][i];
	return FColor();
}

FColor USceneLuminanceCaptureComponent::GetLuminance(const FVector& InViewDirection)
{
	TryRefreshCapture();

	const FVector right = InViewDirection.Cross(FVector::UpVector).GetSafeNormal();
	const FVector up = InViewDirection.Cross(right).GetSafeNormal();
	
	FLinearColor color;
	for (int i = 0; i < BlurSamples; i++)
	{
		const float dist = (static_cast<float>(i) / static_cast<float>(BlurSamples)) * BlurRadius;
		const float phase = dist * PI * 3;
		const float x = FMath::Cos(phase) * dist;
		const float y = FMath::Sin(phase) * dist;
		const FVector off = right * x + up * y;
		const FVector dir = (InViewDirection + off).GetSafeNormal();
		color += Sample(dir);
	}
	color /= BlurSamples;
	color.A = 1.0f;
	return color.ToFColor(true);
}

FVector2f USceneLuminanceCaptureComponent::GetCubemapUV(const FVector& InDirection)
{
	FVector normalized = InDirection.GetSafeNormal();
	return FVector2f((1.0f + atan2(normalized.X, - normalized.Y) / PI) / 2.0f, acosf(normalized.Z) / PI);
}

TTuple<FVector2f, int> USceneLuminanceCaptureComponent::DirToFaceUV(const FVector& InDir)
{
	//Vector Parallel to the unit vector that lies on one of the cube faces
	float a = FMath::Max(FMath::Abs(InDir.X), FMath::Max(FMath::Abs(InDir.Y), FMath::Abs(InDir.Z)));
	float xa = InDir.X / a;
	float ya = InDir.Y / a;
	float za = InDir.Z / a;

	FVector2f uv;
	int side = 0;
	
	if (xa > 0.999)
	{
		// Right, index 0, rotate left
		side = 0;
		uv = { -za, -ya };
	}

	if (xa < -0.999)
	{
		// Left, index 1, rotate right
		side = 1;
		uv = { za, ya };
	}

	if (ya > 0.999)
	{
		// Up, index 2, upside down and flipped
		side = 2;
		uv = { -xa, -za };
	}

	if (ya < -0.999)
	{
		// Down, index3, correct
		side = 3;
		uv = { xa, za };
	}

	if (za > 0.999)
	{
		// Forwards, index4, correct
		side = 4;
		uv = { xa, ya };
	}

	if (za < -0.999)
	{
		// Backwards, index4, correct
		side = 5;
		uv = { xa, ya };
	}
	
	uv = (uv - FVector2f(1)) / 2; 

	while (uv.X < 0)
		uv.X += 1;
	while (uv.X >= 1)
		uv.X -= 1;
	while (uv.Y < 0)
		uv.Y += 1;
	while (uv.Y >= 1)
		uv.Y -= 1;
	
	return { uv, side };
}
