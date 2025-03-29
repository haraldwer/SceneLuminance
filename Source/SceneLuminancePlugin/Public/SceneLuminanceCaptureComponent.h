// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponentCube.h"
#include "SceneLuminanceCaptureComponent.generated.h"

/**
 * Captures the rendered scene and provides luminance information on the CPU.   
 */
UCLASS(hidecategories = (Collision, Object, Physics, SceneComponent), ClassGroup=Rendering, editinlinenew, meta = (BlueprintSpawnableComponent))
class SCENELUMINANCEPLUGIN_API USceneLuminanceCaptureComponent : public USceneCaptureComponentCube
{
	GENERATED_BODY()

public:

	USceneLuminanceCaptureComponent();
	
	UPROPERTY(EditDefaultsOnly)
	float CaptureRefreshTime = 0.1f; // Max time between captures in seconds.

	UPROPERTY(EditDefaultsOnly)
	float CaptureRefreshDistance = 50.0f; // Distance threshold before refreshing capture.

	UPROPERTY(EditDefaultsOnly)
	float BlurRadius = 0.3f;

	UPROPERTY(EditDefaultsOnly)
	int BlurSamples = 32;
	
	UFUNCTION(BlueprintPure)
	FColor GetLuminance(const FVector& InViewDirection);

	UFUNCTION(BlueprintPure)
	static FVector2f GetCubemapUV(const FVector& InDirection);

private:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void TryRefreshCapture();
	void UpdateCapture();
	FLinearColor Sample(const FVector& InDirection) const;
	static TTuple<FVector2f, int> DirToFaceUV(const FVector& InDir);
	
	TArray<TArray<FLinearColor>> Sides;
	int SizeX;
	int SizeY;
	
	FVector LastCaptureLocation = FVector::Zero();
	float CaptureTimestamp = 0;
};
