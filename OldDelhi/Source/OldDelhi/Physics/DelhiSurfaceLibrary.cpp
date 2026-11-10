#include "Physics/DelhiSurfaceLibrary.h"

#include "DelhiWeatherSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	FDelhiSurfaceData Make(const TCHAR* Name, float Fric, float Rest, float Dens, float WetScale, float PenJ,
	                       float RicoDeg, bool bMultiply = false)
	{
		FDelhiSurfaceData D;
		D.SurfaceName = FName(Name);
		D.Friction = Fric;
		D.Restitution = Rest;
		D.DensityGCm3 = Dens;
		D.WetFrictionScale = WetScale;
		D.PenetrationJPerCm = PenJ;
		D.RicochetAngleDeg = RicoDeg;
		D.bMultiplyCombine = bMultiply;
		return D;
	}

	const TArray<FDelhiSurfaceData>& Table()
	{
		// Index == EDelhiSurface value == EPhysicalSurface value.
		// Friction / restitution / density: implementation.md 4.1. Wet scale and penetration values are tuning
		// starting points (see docs/PHYSICS.md section 3).
		static const TArray<FDelhiSurfaceData> T = {
			Make(TEXT("Default"),           0.60f, 0.10f, 2.0f, 0.85f, 60.f,  0.f),
			Make(TEXT("LakhoriBrick"),      0.65f, 0.10f, 1.9f, 0.85f, 90.f,  12.f),
			Make(TEXT("LimePlaster"),       0.60f, 0.05f, 1.6f, 0.80f, 40.f,  0.f),
			Make(TEXT("RedSandstone"),      0.60f, 0.10f, 2.3f, 0.72f, 95.f,  14.f),
			Make(TEXT("KotaStone"),         0.55f, 0.10f, 2.7f, 0.55f, 110.f, 16.f),
			Make(TEXT("CorrugatedShutter"), 0.40f, 0.30f, 7.8f, 0.70f, 60.f,  18.f),
			Make(TEXT("Glass"),             0.30f, 0.20f, 2.5f, 0.70f, 8.f,   0.f),
			Make(TEXT("Asphalt"),           0.75f, 0.05f, 2.3f, 0.65f, 120.f, 10.f),
			Make(TEXT("Mud"),               0.45f, 0.00f, 1.7f, 1.00f, 55.f,  0.f, true),
			Make(TEXT("Wood"),              0.50f, 0.25f, 0.7f, 0.75f, 25.f,  0.f),
			Make(TEXT("SheetMetal"),        0.35f, 0.25f, 7.8f, 0.70f, 12.f,  20.f),
			Make(TEXT("Water"),             0.05f, 0.00f, 1.0f, 1.00f, 5.f,   0.f),
			Make(TEXT("Concrete"),          0.70f, 0.05f, 2.4f, 0.80f, 150.f, 12.f),
			Make(TEXT("Flesh"),             0.60f, 0.05f, 1.0f, 1.00f, 35.f,  0.f),
		};
		return T;
	}
}

const FDelhiSurfaceData& UDelhiSurfaceLibrary::Lookup(EDelhiSurface Surface)
{
	const TArray<FDelhiSurfaceData>& T = Table();
	const int32 Idx = static_cast<int32>(Surface);
	return T.IsValidIndex(Idx) ? T[Idx] : T[0];
}

FDelhiSurfaceData UDelhiSurfaceLibrary::GetSurfaceData(EDelhiSurface Surface)
{
	return Lookup(Surface);
}

EDelhiSurface UDelhiSurfaceLibrary::FromPhysicalSurface(EPhysicalSurface PhysicalSurface)
{
	const int32 V = static_cast<int32>(PhysicalSurface);
	return (V >= 0 && V < static_cast<int32>(EDelhiSurface::Count)) ? static_cast<EDelhiSurface>(V) : EDelhiSurface::Default;
}

EPhysicalSurface UDelhiSurfaceLibrary::ToPhysicalSurface(EDelhiSurface Surface)
{
	return static_cast<EPhysicalSurface>(static_cast<int32>(Surface));
}

EDelhiSurface UDelhiSurfaceLibrary::GetSurfaceFromHit(const FHitResult& Hit)
{
	if (const UPhysicalMaterial* PM = Hit.PhysMaterial.Get())
	{
		return FromPhysicalSurface(PM->SurfaceType.GetValue());
	}
	return EDelhiSurface::Default;
}

float UDelhiSurfaceLibrary::GetCurrentWetness(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (World)
	{
		if (const UDelhiWeatherSubsystem* Weather = World->GetSubsystem<UDelhiWeatherSubsystem>())
		{
			return FMath::Clamp(Weather->GetWetness(), 0.f, 1.f);
		}
	}
	return 0.f;
}

float UDelhiSurfaceLibrary::GetWetFrictionScale(const UObject* WorldContextObject, EDelhiSurface Surface, float WetnessOverride)
{
	const float Wet = WetnessOverride >= 0.f ? FMath::Clamp(WetnessOverride, 0.f, 1.f) : GetCurrentWetness(WorldContextObject);
	return FMath::Lerp(1.f, Lookup(Surface).WetFrictionScale, Wet);
}

float UDelhiSurfaceLibrary::GetEffectiveFriction(const UObject* WorldContextObject, EDelhiSurface Surface, float WetnessOverride)
{
	return Lookup(Surface).Friction * GetWetFrictionScale(WorldContextObject, Surface, WetnessOverride);
}
