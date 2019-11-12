/******************************************************************************************************
Copyright <YEAR> <COPYRIGHT HOLDER>
Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*******************************************************************************************************/

#include "BinauralAudioSource.h"

// Sets default values
ABinauralAudioSource::ABinauralAudioSource()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	AudioPlayer = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio Player"));
	AudioPlayer->SetupAttachment(this->RootComponent);

	// Sets the values for the non-dynamic sound attenuation variables
	SoundAttenuation.bAttenuateWithLPF = true;
	SoundAttenuation.bEnableOcclusion = true;
	SoundAttenuation.bEnableLogFrequencyScaling = true;
	SoundAttenuation.SpatializationAlgorithm = ESoundSpatializationAlgorithm::SPATIALIZATION_HRTF;
	SoundAttenuation.StereoSpread = 15.3f;
	SoundAttenuation.LPFRadiusMin = 0.f;
	SoundAttenuation.LPFFrequencyAtMin = 20000.f;
	SoundAttenuation.LPFFrequencyAtMax = 20000.f;
	SoundAttenuation.HPFFrequencyAtMax = 2124.f;
	SoundAttenuation.OcclusionVolumeAttenuation = 0.95f;
}

// Called when the game starts or when spawned
void ABinauralAudioSource::BeginPlay()
{
	Super::BeginPlay();

	// Applies the attenuation settins to the audio file
	if(Audio)
	{
		USoundAttenuation* AudioAttenSettings = NewObject<USoundAttenuation>(this);
		AudioAttenSettings->Attenuation = SoundAttenuation;
		Audio->AttenuationSettings = AudioAttenSettings;
	}

	AudioPlayer->SetSound(Audio);
	
	// Dictates whether the sound plays multiple times or just once
	if (PlayOnceOrLoop == EPlayStyle::Loop)
	{
		FTimerHandle PlaySoundTimer;
		FTimerDelegate PlaySoundDelegate;
		PlaySoundDelegate.BindUFunction(this, FName("PlaySound"));
		GetWorld()->GetTimerManager().SetTimer(PlaySoundTimer, PlaySoundDelegate, 1.f, true);
	}else PlaySound();
}

// Simulates head occlusion
float ABinauralAudioSource::GetOcclusionFrequency()
{
	float TestingElevation = GetElevation();

	if (TestingElevation < -15)
	{
		return (MaxFrequencies.Minus20Degrees - MinFrequencies.Minus20Degrees);
	}else if (TestingElevation >= -15 && TestingElevation < -5)
	{
		return (MaxFrequencies.Minus10Degrees - MinFrequencies.Minus10Degrees);
	}else if (TestingElevation >= 5 && TestingElevation <= 5)
	{
		return (MaxFrequencies.At0Degrees - MinFrequencies.At0Degrees);
	}else if (TestingElevation <= 15 && TestingElevation > 5)
	{
		return (MaxFrequencies.Positive10Degrees - MinFrequencies.Positive10Degrees);
	} else if (TestingElevation > 15)
	{
		return (MaxFrequencies.Positive20Degrees - MinFrequencies.Positive20Degrees);
	}

	return 0;
}

// Called every frame
void ABinauralAudioSource::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Keeps the maximum absorption and reverb levels at the object's range, and the lower between 0 and 1400
	SoundAttenuation.LPFRadiusMax = GetRange();
	SoundAttenuation.ReverbDistanceMin = FMath::Clamp(GetRange(), 0.f, 1400.f);
	SoundAttenuation.ReverbDistanceMax = GetRange();
	
	// This keeps the occlusion frequency filter to a max of the default max minus the highest 
	SoundAttenuation.OcclusionLowPassFilterFrequency = SoundAttenuation.LPFFrequencyAtMax - GetOcclusionFrequency();
	
	SoundAttenuation.OcclusionInterpolationTime = 0.02;
}

// Calculates Range between audio source and player
float ABinauralAudioSource::GetRange()
{
	float Range = FVector::Dist(this->GetActorLocation(), PlayerReference->GetActorLocation());
	return Range;
}

// Calculates Elevation of audio source relative to player
float ABinauralAudioSource::GetElevation()
{
	float Elevation = FMath::Sin((this->GetActorLocation().Z - PlayerReference->GetTransform().GetLocation().Z) / GetRange());
	return Elevation;
}

// Calculates Azimuth of audio source around player
float ABinauralAudioSource::GetAzimuth()
{
	FVector ForwardPoint = PlayerReference->GetActorForwardVector();
	ForwardPoint.Z = 0;
	ForwardPoint.Normalize();
	FVector ThisLoc = this->GetActorLocation() - PlayerReference->GetActorLocation();
	ThisLoc.Z = 0;
	ThisLoc.Normalize();
	float Azimuth = FVector::DotProduct(ThisLoc, ForwardPoint);

	return Azimuth;
}

// Plays the newly attenuated sound
void ABinauralAudioSource::PlaySound()
{
	AudioPlayer->Play();
}
