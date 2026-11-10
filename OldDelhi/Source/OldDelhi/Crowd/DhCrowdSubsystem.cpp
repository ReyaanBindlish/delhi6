#include "Crowd/DhCrowdSubsystem.h"

#include "Crowd/DhCrowdInstanceRenderer.h"
#include "Crowd/DhCrowdSettings.h"
#include "DelhiPopulationGovernor.h"
#include "DelhiPopulationTypes.h"
#include "DelhiWeatherSubsystem.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogDhCrowd, Log, All);

namespace
{
	// ------------------------------------------------------------------------------------------------
	// Street classes and group weights (research/05 section 5: "archetype share for a time block and street type").
	// Columns: Main, Bazaar, Resid, Worship, Station, Harbour, Check, Highway, Jail
	// ------------------------------------------------------------------------------------------------
	constexpr int32 NumStreetClasses = 9;

	int32 StreetClass(FName Street)
	{
		static const TMap<FName, int32> Map = {
			{ TEXT("MainRoad"), 0 }, { TEXT("BazaarGalli"), 1 }, { TEXT("ResidentialGalli"), 2 }, { TEXT("SectorStreet"), 2 },
			{ TEXT("LyariGali"), 2 }, { TEXT("MosqueGate"), 3 }, { TEXT("ShrineForecourt"), 3 }, { TEXT("StationForecourt"), 4 },
			{ TEXT("HarbourFront"), 5 }, { TEXT("CheckpointApproach"), 6 }, { TEXT("HighwayVerge"), 7 }, { TEXT("JailGate"), 8 } };
		const int32* Found = Map.Find(Street);
		return Found ? *Found : 0;
	}

	uint32 StreetBit(FName Street)
	{
		using namespace DhCrowdArchetypeBits;
		switch (StreetClass(Street))
		{
		case 1: return Market;
		case 2: return Residential;
		case 3: return Worship;
		case 4: return Station;
		case 5: return Harbour;
		case 6: return Checkpoint;
		case 7: return Highway;
		case 8: return Jail;
		default: return Market | Highway;
		}
	}

	float GroupStreetWeight(EDhCrowdGroup G, FName Street)
	{
		static const float W[12][NumStreetClasses] = {
			/* Vendor     */ { 3.f, 10.f, 1.f, 4.f, 6.f, 5.f, 2.f, 1.f, 2.f },
			/* Worker     */ { 4.f, 4.f, 2.f, 1.f, 4.f, 6.f, 1.f, 3.f, 1.f },
			/* Driver     */ { 6.f, 2.f, 1.f, 1.f, 5.f, 2.f, 5.f, 6.f, 1.f },
			/* Police     */ { 2.f, 1.f, 0.3f, 2.f, 3.f, 1.f, 10.f, 2.f, 6.f },
			/* Student    */ { 2.f, 2.f, 3.f, 0.5f, 1.f, 0.2f, 0.1f, 0.1f, 0.f },
			/* Commuter   */ { 4.f, 3.f, 2.f, 1.f, 6.f, 0.5f, 2.f, 2.f, 0.5f },
			/* Tourist    */ { 1.f, 2.f, 0.2f, 3.f, 2.f, 0.5f, 0.f, 0.f, 0.f },
			/* Worshipper */ { 0.5f, 0.5f, 1.f, 12.f, 0.5f, 0.f, 0.f, 0.f, 0.5f },
			/* Static     */ { 1.f, 2.f, 0.5f, 6.f, 3.f, 0.5f, 1.f, 1.f, 2.f },
			/* Special    */ { 0.5f, 1.f, 0.5f, 1.f, 1.f, 0.f, 0.2f, 0.f, 0.f },
			/* Resident   */ { 1.f, 2.f, 6.f, 2.f, 0.5f, 1.f, 0.2f, 0.f, 0.5f },
			/* Other      */ { 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f } };
		return W[static_cast<int32>(G)][StreetClass(Street)];
	}

	/** 1 inside [A,B] hours, fading to 0 over Fade hours on either side. */
	float Bump(float H, float A, float B, float Fade)
	{
		if (H >= A && H <= B) { return 1.f; }
		const float D = (H < A) ? (A - H) : (H - B);
		return FMath::Clamp(1.f - D / FMath::Max(Fade, 0.01f), 0.f, 1.f);
	}

	float TimeFactor(EDhCrowdGroup G, float H)
	{
		switch (G)
		{
		case EDhCrowdGroup::Vendor:     return FMath::Max(0.12f, Bump(H, 9.5f, 20.5f, 2.f));
		case EDhCrowdGroup::Worker:     return FMath::Max(0.2f, Bump(H, 6.f, 19.f, 1.5f));
		case EDhCrowdGroup::Driver:     return FMath::Max(0.35f, Bump(H, 6.f, 22.f, 1.5f));
		case EDhCrowdGroup::Police:     return FMath::Max(0.6f, Bump(H, 8.f, 22.f, 2.f));
		case EDhCrowdGroup::Student:    return FMath::Max(0.03f, FMath::Max(Bump(H, 7.f, 8.5f, 0.75f), Bump(H, 13.f, 14.75f, 0.75f)));
		case EDhCrowdGroup::Commuter:   return FMath::Max(0.1f, FMath::Max(Bump(H, 8.f, 10.f, 1.f), Bump(H, 17.f, 20.f, 1.f)) + 0.35f * Bump(H, 10.f, 17.f, 0.5f));
		case EDhCrowdGroup::Tourist:    return FMath::Max(0.08f, Bump(H, 10.f, 18.f, 1.5f));
		case EDhCrowdGroup::Worshipper: return FMath::Max(0.1f, FMath::Max3(0.8f * Bump(H, 4.75f, 6.f, 0.75f), Bump(H, 12.25f, 14.f, 0.75f), FMath::Max(0.6f * Bump(H, 16.f, 17.25f, 0.5f), 0.9f * Bump(H, 18.5f, 20.5f, 0.75f))));
		case EDhCrowdGroup::Special:    return (H < 6.f || H > 20.f) ? 1.f : 0.5f;
		case EDhCrowdGroup::Resident:   return 0.6f + 0.4f * Bump(H, 17.f, 21.f, 1.f);
		case EDhCrowdGroup::Static:
		default:                        return 1.f;
		}
	}

	EDhCrowdGroup ParseGroup(const FName& Name)
	{
		const FString S = Name.ToString();
		if (S.Equals(TEXT("Vendor"), ESearchCase::IgnoreCase)) return EDhCrowdGroup::Vendor;
		if (S.Equals(TEXT("Worker"), ESearchCase::IgnoreCase)) return EDhCrowdGroup::Worker;
		if (S.Equals(TEXT("Driver"), ESearchCase::IgnoreCase)) return EDhCrowdGroup::Driver;
		if (S.Equals(TEXT("Police"), ESearchCase::IgnoreCase)) return EDhCrowdGroup::Police;
		if (S.Equals(TEXT("Student"), ESearchCase::IgnoreCase)) return EDhCrowdGroup::Student;
		if (S.Equals(TEXT("Commuter"), ESearchCase::IgnoreCase)) return EDhCrowdGroup::Commuter;
		if (S.Equals(TEXT("Tourist"), ESearchCase::IgnoreCase)) return EDhCrowdGroup::Tourist;
		if (S.Equals(TEXT("Worshipper"), ESearchCase::IgnoreCase)) return EDhCrowdGroup::Worshipper;
		if (S.Equals(TEXT("Static"), ESearchCase::IgnoreCase)) return EDhCrowdGroup::Static;
		if (S.Equals(TEXT("Special"), ESearchCase::IgnoreCase)) return EDhCrowdGroup::Special;
		if (S.Equals(TEXT("Resident"), ESearchCase::IgnoreCase)) return EDhCrowdGroup::Resident;
		return EDhCrowdGroup::Other;
	}

	bool Has(const FString& Hay, const TCHAR* Needle) { return Hay.Contains(Needle, ESearchCase::IgnoreCase); }

	EDhCrowdRegion RegionFor(FName Location)
	{
		const FString L = Location.ToString().ToLower();
		if (L.StartsWith(TEXT("karachi")) || L.StartsWith(TEXT("lyari")) || L.StartsWith(TEXT("gulbahar"))) { return EDhCrowdRegion::Karachi; }
		if (L.StartsWith(TEXT("old_delhi")) || L.StartsWith(TEXT("olddelhi")) || L.StartsWith(TEXT("delhi6"))) { return EDhCrowdRegion::OldDelhi; }
		return EDhCrowdRegion::DelhiNCR;
	}

	FLinearColor Jitter(const FLinearColor& Base, FRandomStream& R)
	{
		FLinearColor HSV = Base.LinearRGBToHSV();
		HSV.R = FMath::Fmod(HSV.R + R.FRandRange(-12.f, 12.f) + 360.f, 360.f);
		HSV.G = FMath::Clamp(HSV.G * R.FRandRange(0.85f, 1.1f), 0.f, 1.f);
		HSV.B = FMath::Clamp(HSV.B * R.FRandRange(0.85f, 1.1f), 0.02f, 1.f);
		FLinearColor Out = HSV.HSVToLinearRGB();
		Out.A = 1.f;
		return Out;
	}

	float Square(float X) { return X * X; }

	// Console commands (dh.crowd.*)
	UDhCrowdSubsystem* FromWorld(UWorld* W) { return W ? W->GetSubsystem<UDhCrowdSubsystem>() : nullptr; }

	FAutoConsoleCommandWithWorldAndArgs GCmdDensity(TEXT("dh.crowd.density"), TEXT("dh.crowd.density <0.5|0.75|1.0>: crowd density setting"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* W)
		{
			if (UDhCrowdSubsystem* S = FromWorld(W)) { if (Args.Num() > 0) { S->SetCrowdDensity(FCString::Atof(*Args[0])); } }
		}));
	FAutoConsoleCommandWithWorldAndArgs GCmdLocation(TEXT("dh.crowd.location"), TEXT("dh.crowd.location <id>: switch crowd location (karachi_lyari, nizamuddin, ...)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* W)
		{
			if (UDhCrowdSubsystem* S = FromWorld(W)) { if (Args.Num() > 0) { S->SetLocation(FName(*Args[0])); } }
		}));
	FAutoConsoleCommandWithWorldAndArgs GCmdHour(TEXT("dh.crowd.hour"), TEXT("dh.crowd.hour <0-24 | -1>: override crowd time of day"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* W)
		{
			if (UDhCrowdSubsystem* S = FromWorld(W)) { S->SetHoursOverride(Args.Num() > 0 ? FCString::Atof(*Args[0]) : -1.f); }
		}));
	FAutoConsoleCommandWithWorldAndArgs GCmdPause(TEXT("dh.crowd.pause"), TEXT("dh.crowd.pause <0|1>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* W)
		{
			if (UDhCrowdSubsystem* S = FromWorld(W)) { S->SetPaused(Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !S->IsPaused()); }
		}));
	FAutoConsoleCommandWithWorldAndArgs GCmdWarm(TEXT("dh.crowd.warm"), TEXT("dh.crowd.warm: fill the streets immediately"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* W)
		{
			if (UDhCrowdSubsystem* S = FromWorld(W)) { S->WarmStart(); }
		}));
	FAutoConsoleCommandWithWorldAndArgs GCmdStats(TEXT("dh.crowd.stats"), TEXT("dh.crowd.stats: log tier counts"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* W)
		{
			if (UDhCrowdSubsystem* S = FromWorld(W)) { S->DebugPrint(); }
		}));
	FAutoConsoleCommandWithWorldAndArgs GCmdShoot(TEXT("dh.crowd.disturb"), TEXT("dh.crowd.disturb: scare everyone within 60 m of the player"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* W)
		{
			if (UDhCrowdSubsystem* S = FromWorld(W))
			{
				S->ReportDisturbance(S->GetPlayerInfo().Pos, 60.f, 14.f);
			}
		}));
}

// =====================================================================================================
// Lifecycle
// =====================================================================================================

bool UDhCrowdSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* W = Cast<UWorld>(Outer);
	if (!W || !W->IsGameWorld()) { return false; }
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();
	return S && S->bEnableCrowd;
}

void UDhCrowdSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UDelhiPopulationGovernor>();
	Collection.InitializeDependency<UDelhiWeatherSubsystem>();
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();
	Density = S->CrowdDensity;
	LocationId = S->DefaultLocation;
	LaneOffsetCm = S->LaneWorldOffsetCm;
	CurrentVisibleCap = S->VisibleCap;
}

void UDhCrowdSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	InitWorld();
}

void UDhCrowdSubsystem::Deinitialize()
{
	ResetAgents();
	MassBridge.Shutdown();
	for (ADhCrowdCharacter* C : ActorPool) { if (IsValid(C)) { C->Destroy(); } }
	ActorPool.Reset();
	if (IsValid(Renderer)) { Renderer->Destroy(); }
	Renderer = nullptr;
	Super::Deinitialize();
}

void UDhCrowdSubsystem::InitWorld()
{
	if (bWorldReady || !GetWorld()) { return; }
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();

	Rng.Initialize(7313);
	LoadVisuals();
	CreateRenderer();

	bUseMass = false;
	if (S->Backend != EDhCrowdBackend::Pooled)
	{
		bUseMass = MassBridge.Init(GetWorld(), S->MassEntityConfig);
		if (!bUseMass)
		{
			UE_LOG(LogDhCrowd, Warning, TEXT("Mass backend requested but unavailable: falling back to the pooled backend"));
		}
	}

	Agents.SetNum(FMath::Clamp(S->TotalPopulation, 100, 8000));
	for (int32 i = 0; i < Agents.Num(); ++i) { Agents[i].Id = i; }

	bWorldReady = true;
	if (!SetLocation(LocationId))
	{
		UE_LOG(LogDhCrowd, Warning, TEXT("No lane data for '%s': run Tools/ue/build_zonegraph.py. The crowd stays empty."), *LocationId.ToString());
	}
	UE_LOG(LogDhCrowd, Log, TEXT("Crowd ready: pool %d, backend %s, visible cap %d"), Agents.Num(), bUseMass ? TEXT("Mass") : TEXT("Pooled"), S->VisibleCap);
}

void UDhCrowdSubsystem::LoadVisuals()
{
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();
	LoadedVisuals.Reset();
	for (const FDhCrowdVisualSet& Set : S->VisualSets)
	{
		USkeletalMesh* Mesh = Cast<USkeletalMesh>(Set.Mesh.TryLoad());
		if (!Mesh) { continue; }
		FDhCrowdLoadedVisual V;
		V.Mesh = Mesh;
		V.Walk = Cast<UAnimSequence>(Set.Walk.TryLoad());
		V.Idle = Cast<UAnimSequence>(Set.Idle.TryLoad());
		V.bFemale = Set.bFemale;
		if (!V.Walk && !V.Idle) { continue; }
		LoadedVisuals.Add(V);
	}
	SkeletalMaterial = Cast<UMaterialInterface>(S->SkeletalMaterial.TryLoad());
	if (LoadedVisuals.Num() == 0)
	{
		UE_LOG(LogDhCrowd, Warning, TEXT("No mannequin stand-in found (Project Settings > Dhurandhar Crowd > Visual Sets). Near tiers use instanced proxies. Run Tools/ue/setup_crowd.py."));
	}
}

void UDhCrowdSubsystem::CreateRenderer()
{
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();
	bool bFeetPivot = true;
	InstancedMeshAsset = Cast<UStaticMesh>(S->InstancedMesh.TryLoad());
	if (!InstancedMeshAsset)
	{
		InstancedMeshAsset = Cast<UStaticMesh>(S->FallbackInstancedMesh.TryLoad());
		bFeetPivot = false;
	}
	if (!InstancedMeshAsset) { return; }

	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Renderer = GetWorld()->SpawnActor<ADhCrowdInstanceRenderer>(ADhCrowdInstanceRenderer::StaticClass(), FTransform::Identity, Params);
	if (Renderer)
	{
		UMaterialInterface* Mat = Cast<UMaterialInterface>(S->InstancedMaterial.TryLoad());
		const int32 NearCap = S->NearInstancedCount + 24;
		const int32 FarCap = FMath::Max(S->EventVisibleCap, S->VisibleCap);
		Renderer->Setup(InstancedMeshAsset, Mat, NearCap, FarCap, bFeetPivot, 170.f);
	}
}

UDataTable* UDhCrowdSubsystem::LoadCsvTable(UScriptStruct* RowStruct, const FString& FileName) const
{
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), S->CsvFolder, FileName);
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		UE_LOG(LogDhCrowd, Warning, TEXT("CSV not found: %s"), *Path);
		return nullptr;
	}
	UDataTable* Table = NewObject<UDataTable>(GetTransientPackage());
	Table->RowStruct = RowStruct;
	const TArray<FString> Problems = Table->CreateTableFromCSVString(Text);
	for (const FString& P : Problems) { UE_LOG(LogDhCrowd, Warning, TEXT("%s: %s"), *FileName, *P); }
	return Table;
}

bool UDhCrowdSubsystem::LoadLocationData()
{
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();
	Region = RegionFor(LocationId);

	// ---- archetypes (asset first, then the CSV next to the project)
	const FSoftObjectPath* AssetPath = nullptr;
	const TCHAR* CsvName = TEXT("DT_Archetypes.csv");
	switch (Region)
	{
	case EDhCrowdRegion::Karachi: AssetPath = &S->ArchetypeTableKarachi; CsvName = TEXT("DT_Archetypes_Karachi.csv"); break;
	case EDhCrowdRegion::DelhiNCR: AssetPath = &S->ArchetypeTableDelhiNCR; CsvName = TEXT("DT_Archetypes_DelhiNCR.csv"); break;
	default: AssetPath = &S->ArchetypeTableOldDelhi; CsvName = TEXT("DT_Archetypes.csv"); break;
	}
	ArchetypeTable = AssetPath->IsValid() ? Cast<UDataTable>(AssetPath->TryLoad()) : nullptr;
	if (!ArchetypeTable) { ArchetypeTable = LoadCsvTable(FDelhiArchetypeRow::StaticStruct(), CsvName); }
	BuildArchetypeInfos(ArchetypeTable);

	// ---- street densities: hand to the governor (Old Delhi keeps its own table if already set)
	UDelhiPopulationGovernor* Gov = GetWorld()->GetSubsystem<UDelhiPopulationGovernor>();
	if (Gov)
	{
		if (Region != EDhCrowdRegion::OldDelhi || !Gov->StreetDensity)
		{
			StreetDensityTable = S->StreetDensityStory.IsValid() ? Cast<UDataTable>(S->StreetDensityStory.TryLoad()) : nullptr;
			if (!StreetDensityTable) { StreetDensityTable = LoadCsvTable(FDelhiStreetDensityRow::StaticStruct(), TEXT("DT_StreetDensity_Story.csv")); }
			if (StreetDensityTable) { Gov->StreetDensity = StreetDensityTable; }
		}
	}

	// ---- lanes
	Net.Reset();
	const FString Id = LocationId.ToString();
	for (const FString& Folder : S->LaneFolders)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectDir(), Folder, Id + TEXT("_lanes.json"));
		if (FPaths::FileExists(Path) && Net.LoadFromJsonFile(Path, LaneOffsetCm)) { break; }
	}
	return !Net.IsEmpty() && Archetypes.Num() > 0 && Gov && Gov->StreetDensity;
}

void UDhCrowdSubsystem::BuildArchetypeInfos(const UDataTable* Table)
{
	Archetypes.Reset();
	if (!Table) { return; }

	Table->ForeachRow<FDelhiArchetypeRow>(TEXT("DhCrowd"), [this](const FName& Key, const FDelhiArchetypeRow& Row)
	{
		FDhCrowdArchetypeInfo I;
		I.Id = Key;
		I.Label = Row.Label;
		I.Group = ParseGroup(Row.SpawnGroup);
		I.AgeMin = Row.AgeMin;
		I.AgeMax = FMath::Max(Row.AgeMax, Row.AgeMin);
		const float Total = FMath::Max(Row.MaleShare + Row.FemaleShare + Row.OtherShare, 1.f);
		I.FemaleShare = (Row.FemaleShare + 0.5f * Row.OtherShare) / Total;

		// walking speed (cm/s): adult 135, older / younger slower, hurried groups faster
		float Speed = 135.f;
		if (I.AgeMin >= 60) { Speed = 95.f; }
		else if (I.AgeMax <= 14) { Speed = 105.f; }
		switch (I.Group)
		{
		case EDhCrowdGroup::Police: Speed = 125.f; break;
		case EDhCrowdGroup::Worker: Speed = 120.f; break;
		case EDhCrowdGroup::Student: Speed = 140.f; break;
		case EDhCrowdGroup::Commuter: Speed = 150.f; break;
		case EDhCrowdGroup::Tourist: Speed = 105.f; break;
		default: break;
		}
		I.BaseSpeedCms = Speed;

		switch (I.Group)
		{
		case EDhCrowdGroup::Vendor: I.StationaryChance = 0.75f; break;
		case EDhCrowdGroup::Static: I.StationaryChance = 1.0f; break;
		case EDhCrowdGroup::Police: I.StationaryChance = 0.45f; break;
		case EDhCrowdGroup::Driver: I.StationaryChance = 0.30f; break;
		case EDhCrowdGroup::Resident: I.StationaryChance = 0.25f; break;
		case EDhCrowdGroup::Special: I.StationaryChance = 0.30f; break;
		case EDhCrowdGroup::Worshipper: I.StationaryChance = 0.15f; break;
		case EDhCrowdGroup::Worker: I.StationaryChance = 0.20f; break;
		default: I.StationaryChance = 0.05f; break;
		}

		const FString Text = Row.Label + TEXT(" ") + Row.Places;
		using namespace DhCrowdArchetypeBits;
		if (Has(Text, TEXT("shrine")) || Has(Text, TEXT("dargah")) || Has(Text, TEXT("mosque")) || Has(Text, TEXT("masjid")) || Has(Text, TEXT("temple")) || Has(Text, TEXT("gurdwara")) || Has(Text, TEXT("mandir")) || Has(Text, TEXT("pilgrim"))) { I.PlaceMask |= Worship; }
		if (Has(Text, TEXT("harbour")) || Has(Text, TEXT("harbor")) || Has(Text, TEXT("fish")) || Has(Text, TEXT("boat")) || Has(Text, TEXT("jetty")) || Has(Text, TEXT("creek")) || Has(Text, TEXT("ibrahim hyderi"))) { I.PlaceMask |= Harbour; }
		if (Has(Text, TEXT("station")) || Has(Text, TEXT("railway")) || Has(Text, TEXT("isbt")) || Has(Text, TEXT("bus ")) || Has(Text, TEXT("porter"))) { I.PlaceMask |= Station; }
		if (Has(Text, TEXT("checkpoint")) || Has(Text, TEXT("ranger")) || Has(Text, TEXT("police")) || Has(Text, TEXT("naka"))) { I.PlaceMask |= Checkpoint; }
		if (Has(Text, TEXT("market")) || Has(Text, TEXT("bazaar")) || Has(Text, TEXT("shop")) || Has(Text, TEXT("stall")) || Has(Text, TEXT("vendor")) || Has(Text, TEXT("hawker"))) { I.PlaceMask |= Market; }
		if (Has(Text, TEXT("jail")) || Has(Text, TEXT("prison"))) { I.PlaceMask |= Jail; }
		if (Has(Text, TEXT("highway")) || Has(Text, TEXT("truck")) || Has(Text, TEXT("transport")) || Has(Text, TEXT("dhaba"))) { I.PlaceMask |= Highway; }
		if (Has(Text, TEXT("school")) || Has(Text, TEXT("college")) || Has(Text, TEXT("madrassa")) || Has(Text, TEXT("madrasa"))) { I.PlaceMask |= School; }
		if (Has(Text, TEXT("gali")) || Has(Text, TEXT("lane")) || Has(Text, TEXT("resident")) || Has(Text, TEXT("home")) || Has(Text, TEXT("mohalla")) || Has(Text, TEXT("colony"))) { I.PlaceMask |= Residential; }

		if (Has(Row.Label, TEXT("ranger"))) { I.UniformKind = 2; }
		else if (Has(Row.Label, TEXT("police")) || Has(Row.Label, TEXT("constable")) || Has(Row.Label, TEXT("sub-inspector")) || Has(Row.Label, TEXT("sergeant")) || Has(Row.Label, TEXT("warder"))) { I.UniformKind = 1; }
		else if (Has(Row.Label, TEXT("sanitation")) || Has(Row.Label, TEXT("sweeper")) || Has(Row.Label, TEXT("kmc")) || Has(Row.Label, TEXT("mcd")) || Has(Row.Label, TEXT("safai"))) { I.UniformKind = 3; }
		else if (Has(Row.Label, TEXT("student")) && (Has(Row.Label, TEXT("school")) || Has(Row.Label, TEXT("uniform")) || Has(Row.Label, TEXT("madrassa")))) { I.UniformKind = 4; }
		else if (Has(Row.Label, TEXT("security guard")) || Has(Row.Label, TEXT("guard"))) { I.UniformKind = 5; }

		Archetypes.Add(I);
	});
	UE_LOG(LogDhCrowd, Log, TEXT("Loaded %d archetypes for region %d"), Archetypes.Num(), static_cast<int32>(Region));
}

bool UDhCrowdSubsystem::SetLocation(FName NewLocation)
{
	if (!bWorldReady && !GetWorld()) { return false; }
	ResetAgents();
	LocationId = NewLocation;
	Rng.Initialize(7313 + static_cast<int32>(GetTypeHash(NewLocation) & 0xFFFF));
	const bool bOk = LoadLocationData();
	if (bOk) { WarmStart(); }
	return bOk;
}

void UDhCrowdSubsystem::ResetAgents()
{
	FreeDormant.Reset();
	VisibleIdx.Reset();
	for (int32 i = 0; i < Agents.Num(); ++i)
	{
		FDhCrowdAgent& A = Agents[i];
		if (A.Tier != EDhCrowdTier::Dormant) { SetTier(A, EDhCrowdTier::Dormant); }
		A.Tier = EDhCrowdTier::Dormant;
	}
	for (int32 i = Agents.Num() - 1; i >= 0; --i) { FreeDormant.Add(i); }
	for (int32& C : TierCounts) { C = 0; }
	ActiveCount = 0;
	AbstractCursor = 0;
	GroundCursor = 0;
}

void UDhCrowdSubsystem::WarmStart()
{
	if (!bWorldReady || Net.IsEmpty()) { return; }
	UpdatePlayer();
	if (!Player.bValid) { bPendingWarmStart = true; return; } // pawn not spawned yet: retry from Tick
	bPendingWarmStart = false;
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();
	// Loading / teleport: nobody sees the streets fill, so ignore the no-pop-in-view rule and use a big budget.
	bWarmStarting = true;
	for (int32 Pass = 0; Pass < 3; ++Pass) { Rebalance(S->ActiveSimCap); }
	bWarmStarting = false;
	UpdateTiers();
}

// =====================================================================================================
// Tick
// =====================================================================================================

void UDhCrowdSubsystem::Tick(float DeltaTime)
{
	if (!bWorldReady)
	{
		if (GetWorld() && GetWorld()->HasBegunPlay()) { InitWorld(); }
		return;
	}
	if (bPaused || Net.IsEmpty()) { return; }

	const UDhCrowdSettings* S = UDhCrowdSettings::Get();
	const double Now = GetWorld()->GetTimeSeconds();
	const float Dt = FMath::Min(DeltaTime, 0.1f);

	UpdatePlayer();
	if (bPendingWarmStart && Player.bValid) { WarmStart(); }

	TierTimer += Dt;
	RebalanceTimer += Dt;
	if (RebalanceTimer >= S->RebalanceIntervalS) { RebalanceTimer = 0.f; Rebalance(); }
	if (TierTimer >= S->TierIntervalS) { TierTimer = 0.f; UpdateTiers(); }

	SimulateVisible(Dt, Now);
	SimulateAbstract(Now);
	UpdateGround();
	if (Renderer) { Renderer->Flush(); }

	if (S->bShowDebug) { DebugPrint(); }
}

void UDhCrowdSubsystem::UpdatePlayer()
{
	Player.bValid = false;
	UWorld* W = GetWorld();
	if (!W) { return; }

	AActor* Focus = FocusActor.Get();
	APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0);
	if (!Focus && PC) { Focus = PC->GetPawn(); }

	FVector ViewLoc = FVector::ZeroVector;
	FRotator ViewRot = FRotator::ZeroRotator;
	if (PC) { PC->GetPlayerViewPoint(ViewLoc, ViewRot); }

	if (Focus)
	{
		Player.Pos = Focus->GetActorLocation();
		Player.Vel = Focus->GetVelocity();
		Player.bValid = true;
	}
	else if (PC)
	{
		Player.Pos = ViewLoc;
		Player.Vel = FVector::ZeroVector;
		Player.bValid = true;
	}
	if (PC)
	{
		Player.ViewDir = ViewRot.Vector();
	}
	else if (Focus)
	{
		Player.ViewDir = Focus->GetActorForwardVector();
	}
}

float UDhCrowdSubsystem::GetHours() const
{
	if (HoursOverride >= 0.f) { return HoursOverride; }
	if (const UWorld* W = GetWorld())
	{
		if (const UDelhiWeatherSubsystem* Wx = W->GetSubsystem<UDelhiWeatherSubsystem>()) { return Wx->TimeOfDayHours; }
	}
	return 12.f;
}

bool UDhCrowdSubsystem::InViewCone(const FVector& P, float MinDot) const
{
	if (!Player.bValid) { return false; }
	const FVector2D To(P.X - Player.Pos.X, P.Y - Player.Pos.Y);
	const FVector2D View(Player.ViewDir.X, Player.ViewDir.Y);
	return FVector2D::DotProduct(To.GetSafeNormal(), View.GetSafeNormal()) > MinDot;
}

bool UDhCrowdSubsystem::CanRetire(const FDhCrowdAgent& A, float D2, float VisR) const
{
	if (A.Tier >= EDhCrowdTier::Skeletal) { return false; }
	if (D2 < Square(3000.f)) { return false; }                    // never vanish within 30 m
	if (D2 < Square(VisR) && InViewCone(A.Pos, 0.35f)) { return false; } // never vanish in view
	return true;
}

void UDhCrowdSubsystem::SyncGovernor()
{
	UDelhiPopulationGovernor* Gov = GetWorld() ? GetWorld()->GetSubsystem<UDelhiPopulationGovernor>() : nullptr;
	if (!Gov) { return; }
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();
	Gov->CrowdDensitySetting = Density;
	Gov->VisibleCap = S->VisibleCap;
	Gov->EventVisibleCap = S->EventVisibleCap;
	Gov->EventMultiplier = bEventActive ? EventMultiplier : 1.f;
	if (S->bDriveGovernorFromWeather)
	{
		const UDelhiWeatherSubsystem* Wx = GetWorld()->GetSubsystem<UDelhiWeatherSubsystem>();
		const float Wet = Wx ? FMath::Clamp(Wx->GetWetness(), 0.f, 1.f) : 0.f;
		Gov->WeatherMultiplier = FMath::Lerp(1.f, 0.55f, Wet); // research/05: rain 0.55
	}
	CurrentVisibleCap = bEventActive ? S->EventVisibleCap : S->VisibleCap;
}

// =====================================================================================================
// Population: governor -> per-cell targets -> activate / retire
// =====================================================================================================

void UDhCrowdSubsystem::Rebalance(int32 SpawnBudgetOverride)
{
	if (!Player.bValid || Net.IsEmpty()) { return; }
	UDelhiPopulationGovernor* Gov = GetWorld()->GetSubsystem<UDelhiPopulationGovernor>();
	if (!Gov || !Gov->StreetDensity || Archetypes.Num() == 0) { return; }

	const UDhCrowdSettings* S = UDhCrowdSettings::Get();
	SyncGovernor();
	const float Hours = GetHours();
	const FVector P = Player.Pos;
	const float SimR = S->SimRadiusM * 100.f;
	const float VisR = S->VisibleRadiusM * 100.f;

	// 1. census of the active agents; retire the ones that wandered out of the simulated radius
	TMap<FIntPoint, TArray<int32>> CellAgents;
	ActiveCount = 0;
	for (int32 i = 0; i < Agents.Num(); ++i)
	{
		FDhCrowdAgent& A = Agents[i];
		if (A.Tier == EDhCrowdTier::Dormant) { continue; }
		if (FVector::DistSquaredXY(A.Pos, P) > Square(SimR * 1.2f)) { Retire(i); continue; }
		++ActiveCount;
		CellAgents.FindOrAdd(FDhCrowdLaneNetwork::CellOf(A.Pos)).Add(i);
	}

	// 2. governor density per street type (cached: one table lookup per street per rebalance)
	TMap<FName, float> DensityCache;
	auto DensityOf = [&](FName Street) -> float
	{
		if (const float* F = DensityCache.Find(Street)) { return *F; }
		const float D = Gov->GetTargetDensity(Street, Hours);
		DensityCache.Add(Street, D);
		return D;
	};

	struct FCellWant { FIntPoint Cell; float Dist; float Want; bool bNear; };
	TArray<FCellWant> Wants;
	float NearTotal = 0.f;
	float OuterTotal = 0.f;
	const float CellHalf = FDhCrowdLaneNetwork::CellSizeCm * 0.5f;
	for (const TPair<FIntPoint, TArray<FDhLaneSample>>& Pair : Net.GetCells())
	{
		const FVector Center = FDhCrowdLaneNetwork::CellCenter(Pair.Key);
		const float Dc = static_cast<float>(FVector::DistXY(Center, P));
		if (Dc > SimR + CellHalf * 1.5f) { continue; }

		float W = 0.f;
		for (const FDhLaneSample& Sm : Pair.Value)
		{
			W += DensityOf(Net.GetLane(Sm.Lane).Street) / 100.f * (FDhCrowdLaneNetwork::SampleStepCm / 100.f);
		}
		FCellWant CW;
		CW.Cell = Pair.Key;
		CW.Dist = Dc;
		CW.bNear = Dc < VisR + CellHalf;
		CW.Want = CW.bNear ? W : W * S->OuterDensityFactor;
		(CW.bNear ? NearTotal : OuterTotal) += CW.Want;
		Wants.Add(CW);
	}
	const float NearScale = (NearTotal > CurrentVisibleCap) ? (CurrentVisibleCap / NearTotal) : 1.f;
	float Total = NearTotal * NearScale + OuterTotal;
	float AllScale = 1.f;
	const float ActiveCap = static_cast<float>(FMath::Min(S->ActiveSimCap, Agents.Num()));
	if (Total > ActiveCap) { AllScale = ActiveCap / Total; }

	Wants.Sort([](const FCellWant& A, const FCellWant& B) { return A.Dist < B.Dist; });

	// 3. converge each cell on its target without popping agents in or out of sight
	int32 SpawnBudget = SpawnBudgetOverride > 0 ? SpawnBudgetOverride : S->MaxSpawnPerRebalance;
	int32 RetireBudget = S->MaxSpawnPerRebalance;
	for (const FCellWant& CW : Wants)
	{
		const float WantF = CW.Want * (CW.bNear ? NearScale : 1.f) * AllScale;
		const int32 Want = FMath::FloorToInt(WantF + 0.5f);
		TArray<int32>* Present = CellAgents.Find(CW.Cell);
		const int32 Have = Present ? Present->Num() : 0;

		if (Have < Want)
		{
			for (int32 n = Have; n < Want && SpawnBudget > 0; ++n)
			{
				if (!TryActivate(CW.Cell, Hours, DensityCache)) { break; }
				--SpawnBudget;
			}
		}
		else if (Present && Have > FMath::CeilToInt(Want * 1.15f) + 1)
		{
			// farthest first, only agents that nobody can see disappear
			Present->Sort([&](int32 L, int32 R)
			{
				return FVector::DistSquaredXY(Agents[L].Pos, P) > FVector::DistSquaredXY(Agents[R].Pos, P);
			});
			int32 ToRetire = Have - Want;
			for (int32 Idx : *Present)
			{
				if (ToRetire <= 0 || RetireBudget <= 0) { break; }
				if (CanRetire(Agents[Idx], static_cast<float>(FVector::DistSquaredXY(Agents[Idx].Pos, P)), VisR))
				{
					Retire(Idx);
					--ToRetire;
					--RetireBudget;
				}
			}
		}
	}
}

bool UDhCrowdSubsystem::TryActivate(const FIntPoint& Cell, float Hours, const TMap<FName, float>& DensityCache)
{
	if (FreeDormant.Num() == 0) { return false; }
	const TArray<FDhLaneSample>* Samples = Net.GetCells().Find(Cell);
	if (!Samples || Samples->Num() == 0) { return false; }
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();

	// weighted pick of a lane sample (busier street types are chosen more often)
	float TotalW = 0.f;
	for (const FDhLaneSample& Sm : *Samples)
	{
		const float* D = DensityCache.Find(Net.GetLane(Sm.Lane).Street);
		TotalW += (D ? *D : 1.f) + 0.05f;
	}

	for (int32 Attempt = 0; Attempt < 4; ++Attempt)
	{
		float Pick = Rng.FRandRange(0.f, TotalW);
		const FDhLaneSample* Chosen = &(*Samples)[0];
		for (const FDhLaneSample& Sm : *Samples)
		{
			const float* D = DensityCache.Find(Net.GetLane(Sm.Lane).Street);
			Pick -= (D ? *D : 1.f) + 0.05f;
			if (Pick <= 0.f) { Chosen = &Sm; break; }
		}

		const float Dist = Chosen->Dist + Rng.FRandRange(0.f, FDhCrowdLaneNetwork::SampleStepCm);
		FVector Pos, Tangent;
		Net.SampleLane(Chosen->Lane, Dist, Pos, Tangent);

		const float D2 = static_cast<float>(FVector::DistSquaredXY(Pos, Player.Pos));
		if (D2 < Square(S->MinSpawnDistanceM * 100.f)) { continue; }
		if (!bWarmStarting && D2 < Square(S->NoPopInViewRadiusM * 100.f) && InViewCone(Pos, 0.3f)) { continue; }

		const int32 Index = FreeDormant.Pop(EAllowShrinking::No);
		FDhCrowdAgent& A = Agents[Index];
		InitAgent(A, Chosen->Lane, Dist, Hours);
		A.Tier = EDhCrowdTier::Abstract;
		A.LastSimTime = GetWorld()->GetTimeSeconds();
		return true;
	}
	return false;
}

int32 UDhCrowdSubsystem::ChooseArchetype(FName Street, float Hours, FRandomStream& R) const
{
	if (Archetypes.Num() == 0) { return INDEX_NONE; }
	const uint32 Bit = StreetBit(Street);
	TArray<float, TInlineAllocator<64>> Weights;
	float Total = 0.f;
	for (const FDhCrowdArchetypeInfo& I : Archetypes)
	{
		float W = GroupStreetWeight(I.Group, Street) * TimeFactor(I.Group, Hours);
		if (I.PlaceMask & Bit) { W *= 3.f; }
		W = FMath::Max(W, 0.001f);
		Weights.Add(W);
		Total += W;
	}
	float Pick = R.FRandRange(0.f, Total);
	for (int32 i = 0; i < Weights.Num(); ++i)
	{
		Pick -= Weights[i];
		if (Pick <= 0.f) { return i; }
	}
	return Weights.Num() - 1;
}

FLinearColor UDhCrowdSubsystem::PickTint(const FDhCrowdArchetypeInfo& Info, FRandomStream& R) const
{
	switch (Info.UniformKind)
	{
	case 1: return Jitter(FLinearColor(0.34f, 0.29f, 0.17f), R);  // khaki
	case 2: return Jitter(FLinearColor(0.42f, 0.34f, 0.2f), R);   // desert sand
	case 3: return Jitter(R.FRand() < 0.5f ? FLinearColor(0.55f, 0.2f, 0.04f) : FLinearColor(0.06f, 0.12f, 0.3f), R);
	case 4: return Jitter(R.FRand() < 0.6f ? FLinearColor(0.62f, 0.62f, 0.6f) : FLinearColor(0.05f, 0.08f, 0.2f), R);
	case 5: return Jitter(FLinearColor(0.16f, 0.18f, 0.21f), R);
	default: break;
	}

	// muted, dye-like palette (research/05 section 3.7: pinks, mustards, teals, maroons, whites that are never pure)
	static const FLinearColor Palette[] = {
		FLinearColor(0.45f, 0.18f, 0.22f), FLinearColor(0.55f, 0.38f, 0.06f), FLinearColor(0.05f, 0.30f, 0.30f),
		FLinearColor(0.25f, 0.04f, 0.06f), FLinearColor(0.65f, 0.62f, 0.55f), FLinearColor(0.05f, 0.08f, 0.25f),
		FLinearColor(0.20f, 0.22f, 0.08f), FLinearColor(0.40f, 0.14f, 0.06f), FLinearColor(0.20f, 0.30f, 0.20f),
		FLinearColor(0.70f, 0.62f, 0.45f), FLinearColor(0.18f, 0.24f, 0.32f), FLinearColor(0.18f, 0.12f, 0.08f) };
	const int32 N = UE_ARRAY_COUNT(Palette);
	if (Info.Group == EDhCrowdGroup::Worshipper && R.FRand() < 0.55f)
	{
		return Jitter(Palette[4], R); // off-white, common for prayer
	}
	return Jitter(Palette[R.RandRange(0, N - 1)], R);
}

void UDhCrowdSubsystem::InitAgent(FDhCrowdAgent& A, int32 LaneIdx, float Dist, float Hours)
{
	const FDhCrowdLane& Lane = Net.GetLane(LaneIdx);
	A.Arch = static_cast<int16>(ChooseArchetype(Lane.Street, Hours, Rng));
	const FDhCrowdArchetypeInfo& Info = Archetypes[FMath::Max<int32>(A.Arch, 0)];

	A.bFemale = Rng.FRand() < Info.FemaleShare;
	const bool bChild = Info.AgeMax <= 14;
	A.Scale = bChild ? Rng.FRandRange(0.62f, 0.78f) : (A.bFemale ? Rng.FRandRange(0.92f, 1.0f) : Rng.FRandRange(0.97f, 1.07f));
	if (Info.AgeMin >= 60) { A.Scale *= 0.97f; }
	A.Tint = PickTint(Info, Rng);
	A.Skin = Rng.FRand();

	A.Motion = FDhCrowdAgentMotion();
	A.Motion.BaseSpeed = Info.BaseSpeedCms * Rng.FRandRange(0.85f, 1.15f);
	A.Motion.bStationary = Rng.FRand() < Info.StationaryChance;
	A.Motion.DwellLeft = (!A.Motion.bStationary && Rng.FRand() < 0.10f) ? Rng.FRandRange(3.f, 15.f) : 0.f;

	A.Walker = FDhCrowdLaneWalker();
	A.Walker.RngState = Rng.RandRange(1, 0x7FFFFFF0);
	// stationary people stand near the building line, walkers keep to a side of the street
	Net.Place(A.Walker, LaneIdx, Dist, Rng.FRand() < 0.5f ? 1 : -1, A.Motion.bStationary ? 0.9f : Rng.FRandRange(0.25f, 0.7f), Rng.FRand() < 0.5f ? 1 : -1);
	Net.GetPose(A.Walker, A.Pos, A.Dir);
	A.Activity = A.Motion.bStationary ? EDhCrowdActivity::Work : EDhCrowdActivity::Walk;
	A.bHaveGround = false;
	A.GroundZ = static_cast<float>(A.Pos.Z);
	A.Phase = Rng.FRandRange(0.f, 6.28f);
	A.Slot = INDEX_NONE;
	A.Actor.Reset();
	A.bHasMassEntity = false;
}

void UDhCrowdSubsystem::Retire(int32 Index)
{
	FDhCrowdAgent& A = Agents[Index];
	if (A.Tier == EDhCrowdTier::Dormant) { return; }
	SetTier(A, EDhCrowdTier::Dormant);
	FreeDormant.Add(Index);
}

// =====================================================================================================
// Simulation
// =====================================================================================================

void UDhCrowdSubsystem::SimulateVisible(float Dt, double Now)
{
	for (int32 Idx : VisibleIdx)
	{
		if (!Agents.IsValidIndex(Idx)) { continue; }
		FDhCrowdAgent& A = Agents[Idx];
		A.LastSimTime = Now;

		switch (A.Tier)
		{
		case EDhCrowdTier::Hero:
		case EDhCrowdTier::Skeletal:
		{
			A.Activity = FDhCrowdSim::Step(Net, A.Walker, A.Motion, Player, Dt, A.Pos, A.Dir);
			if (ADhCrowdCharacter* Actor = A.Actor.Get())
			{
				const float Speed = A.Motion.BaseSpeed * (A.Activity == EDhCrowdActivity::Flee ? 2.4f : 1.f);
				Actor->Drive(A.Pos, A.Dir, Speed, A.Activity, Dt);
				A.GroundZ = Actor->GetFeetZ();
				A.bHaveGround = true;
			}
			break;
		}
		case EDhCrowdTier::NearInstanced:
		case EDhCrowdTier::FarInstanced:
		{
			if (A.bHasMassEntity)
			{
				MassBridge.Pull(A); // Mass processor already stepped it
			}
			else
			{
				A.Activity = FDhCrowdSim::Step(Net, A.Walker, A.Motion, Player, Dt, A.Pos, A.Dir);
				const bool bWalking = (A.Activity == EDhCrowdActivity::Walk || A.Activity == EDhCrowdActivity::Flee);
				if (bWalking) { A.Phase += Dt * (A.Activity == EDhCrowdActivity::Flee ? 14.f : 7.f); }
				if (Renderer)
				{
					const bool bFar = (A.Tier == EDhCrowdTier::FarInstanced);
					const float Z = A.bHaveGround ? A.GroundZ : static_cast<float>(A.Pos.Z);
					Renderer->SetSlot(bFar, A.Slot, FVector(A.Pos.X, A.Pos.Y, Z), static_cast<float>(A.Dir.Rotation().Yaw), A.Scale, A.Phase, bWalking);
				}
			}
			break;
		}
		default: break;
		}
	}
}

void UDhCrowdSubsystem::SimulateAbstract(double Now)
{
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();
	const int32 N = Agents.Num();
	if (N == 0) { return; }
	int32 Budget = 300;
	const FDhCrowdPlayerInfo NoPlayer;
	int32 k = 0;
	for (; k < N && Budget > 0; ++k)
	{
		FDhCrowdAgent& A = Agents[(AbstractCursor + k) % N];
		if (A.Tier != EDhCrowdTier::Abstract) { continue; }
		const double Dt = Now - A.LastSimTime;
		if (Dt < S->AbstractStepS) { continue; }
		FDhCrowdSim::Step(Net, A.Walker, A.Motion, NoPlayer, static_cast<float>(FMath::Min(Dt, 6.0)), A.Pos, A.Dir);
		A.LastSimTime = Now;
		--Budget;
	}
	AbstractCursor = (AbstractCursor + k) % N;
}

bool UDhCrowdSubsystem::TraceGround(const FVector& XY, float StartZ, float& OutZ) const
{
	UWorld* W = GetWorld();
	if (!W) { return false; }
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(DhCrowdGround), false);
	const FVector From(XY.X, XY.Y, StartZ + 300.f);
	const FVector To(XY.X, XY.Y, StartZ - 3000.f);
	if (W->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Q))
	{
		OutZ = static_cast<float>(Hit.ImpactPoint.Z);
		return true;
	}
	return false;
}

void UDhCrowdSubsystem::UpdateGround()
{
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();
	const int32 N = VisibleIdx.Num();
	if (N == 0) { return; }
	int32 Budget = S->GroundTracesPerTick;
	const float PlayerFeet = static_cast<float>(Player.Pos.Z) - 90.f;
	for (int32 k = 0; k < N && Budget > 0; ++k)
	{
		const int32 Idx = VisibleIdx[(GroundCursor + k) % N];
		if (!Agents.IsValidIndex(Idx)) { continue; }
		FDhCrowdAgent& A = Agents[Idx];
		if (A.Tier != EDhCrowdTier::NearInstanced && A.Tier != EDhCrowdTier::FarInstanced) { continue; }
		float Z = 0.f;
		if (TraceGround(A.Pos, A.bHaveGround ? A.GroundZ : PlayerFeet, Z))
		{
			A.GroundZ = Z;
			A.bHaveGround = true;
			if (A.bHasMassEntity) { MassBridge.SetGroundZ(A, Z); }
		}
		--Budget;
	}
	GroundCursor = (GroundCursor + S->GroundTracesPerTick) % FMath::Max(N, 1);
}

// =====================================================================================================
// Tiers
// =====================================================================================================

void UDhCrowdSubsystem::UpdateTiers()
{
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();
	const FVector P = Player.Pos;
	const float VisR = S->VisibleRadiusM * 100.f;
	const float HeroR = S->HeroRadiusM * 100.f;
	const float SkelR = S->SkeletalRadiusM * 100.f;
	const float NearR = S->NearInstancedRadiusM * 100.f;
	const bool bBodies = LoadedVisuals.Num() > 0 && Player.bValid;

	TArray<TPair<float, int32>> Cands;
	Cands.Reserve(Agents.Num() / 2);
	int32 Changes = S->MaxTierChangesPerUpdate;

	for (int32 i = 0; i < Agents.Num(); ++i)
	{
		FDhCrowdAgent& A = Agents[i];
		if (A.Tier == EDhCrowdTier::Dormant) { continue; }
		const float D2 = static_cast<float>(FVector::DistSquaredXY(A.Pos, P));
		if (D2 < Square(VisR * 1.15f))
		{
			Cands.Add(TPair<float, int32>(D2, i));
		}
		else if (A.Tier > EDhCrowdTier::Abstract)
		{
			SetTier(A, EDhCrowdTier::Abstract); // walked out of sight range; always allowed (frees budget)
		}
	}
	Cands.Sort([](const TPair<float, int32>& L, const TPair<float, int32>& R) { return L.Key < R.Key; });

	int32 Hero = 0, Skel = 0, Near = 0, Total = 0;
	VisibleIdx.Reset();
	for (const TPair<float, int32>& C : Cands)
	{
		FDhCrowdAgent& A = Agents[C.Value];
		const float D = FMath::Sqrt(C.Key);
		const EDhCrowdTier Cur = A.Tier;

		EDhCrowdTier Want = EDhCrowdTier::Abstract;
		if (Total < CurrentVisibleCap)
		{
			if (bBodies && D < HeroR * (Cur == EDhCrowdTier::Hero ? 1.3f : 1.f) && Hero < S->HeroCount) { Want = EDhCrowdTier::Hero; }
			else if (bBodies && D < SkelR * (Cur >= EDhCrowdTier::Skeletal ? 1.25f : 1.f) && Skel < S->SkeletalCount) { Want = EDhCrowdTier::Skeletal; }
			else if (D < NearR * (Cur == EDhCrowdTier::NearInstanced ? 1.2f : 1.f) && Near < S->NearInstancedCount) { Want = EDhCrowdTier::NearInstanced; }
			else if (D < VisR * (Cur == EDhCrowdTier::FarInstanced ? 1.1f : 1.f)) { Want = EDhCrowdTier::FarInstanced; }
		}

		if (Want != Cur)
		{
			if (Changes > 0)
			{
				--Changes;
				if (Want == EDhCrowdTier::Hero && Cur == EDhCrowdTier::Skeletal && A.Actor.IsValid())
				{
					A.Actor->SetHero(true); // promote in place, no respawn
					A.Tier = EDhCrowdTier::Hero;
				}
				else if (Want == EDhCrowdTier::Skeletal && Cur == EDhCrowdTier::Hero && A.Actor.IsValid())
				{
					A.Actor->SetHero(false);
					A.Tier = EDhCrowdTier::Skeletal;
				}
				else
				{
					SetTier(A, Want);
				}
			}
		}

		// count what the agent actually is now (a failed promotion keeps its old tier)
		switch (A.Tier)
		{
		case EDhCrowdTier::Hero: ++Hero; ++Total; VisibleIdx.Add(C.Value); break;
		case EDhCrowdTier::Skeletal: ++Skel; ++Total; VisibleIdx.Add(C.Value); break;
		case EDhCrowdTier::NearInstanced: ++Near; ++Total; VisibleIdx.Add(C.Value); break;
		case EDhCrowdTier::FarInstanced: ++Total; VisibleIdx.Add(C.Value); break;
		default: break;
		}
	}

	TierCounts[static_cast<int32>(EDhCrowdTier::Hero)] = Hero;
	TierCounts[static_cast<int32>(EDhCrowdTier::Skeletal)] = Skel;
	TierCounts[static_cast<int32>(EDhCrowdTier::NearInstanced)] = Near;
	TierCounts[static_cast<int32>(EDhCrowdTier::FarInstanced)] = Total - Hero - Skel - Near;
}

void UDhCrowdSubsystem::LeaveTier(FDhCrowdAgent& A)
{
	switch (A.Tier)
	{
	case EDhCrowdTier::Hero:
	case EDhCrowdTier::Skeletal:
		if (ADhCrowdCharacter* Actor = A.Actor.Get())
		{
			A.GroundZ = Actor->GetFeetZ();
			A.bHaveGround = true;
			Actor->Release();
		}
		A.Actor.Reset();
		break;
	case EDhCrowdTier::NearInstanced:
	case EDhCrowdTier::FarInstanced:
		if (A.bHasMassEntity)
		{
			MassBridge.Despawn(A);
		}
		else if (Renderer)
		{
			Renderer->FreeSlot(A.Tier == EDhCrowdTier::FarInstanced, A.Slot);
		}
		A.Slot = INDEX_NONE;
		break;
	default: break;
	}
}

void UDhCrowdSubsystem::SetTier(FDhCrowdAgent& A, EDhCrowdTier NewTier)
{
	if (A.Tier == NewTier) { return; }
	LeaveTier(A);
	A.Tier = EDhCrowdTier::Abstract;

	if (NewTier == EDhCrowdTier::Hero || NewTier == EDhCrowdTier::Skeletal)
	{
		if (AcquireActor(A, NewTier == EDhCrowdTier::Hero)) { A.Tier = NewTier; return; }
		NewTier = EDhCrowdTier::NearInstanced; // pool exhausted: still show a body
	}
	if (NewTier == EDhCrowdTier::NearInstanced || NewTier == EDhCrowdTier::FarInstanced)
	{
		if (EnterInstanced(A, NewTier == EDhCrowdTier::FarInstanced)) { A.Tier = NewTier; return; }
		NewTier = EDhCrowdTier::Abstract;    // visible cap or slots full
	}
	A.Tier = NewTier;
}

const FDhCrowdLoadedVisual* UDhCrowdSubsystem::GetVisual(bool bFemale) const
{
	for (const FDhCrowdLoadedVisual& V : LoadedVisuals) { if (V.bFemale == bFemale) { return &V; } }
	return LoadedVisuals.Num() > 0 ? &LoadedVisuals[0] : nullptr;
}

bool UDhCrowdSubsystem::AcquireActor(FDhCrowdAgent& A, bool bHero)
{
	const FDhCrowdLoadedVisual* Visual = GetVisual(A.bFemale);
	if (!Visual || !Visual->Mesh || !GetWorld()) { return false; }
	const UDhCrowdSettings* S = UDhCrowdSettings::Get();

	// ground under the agent (the actor will then settle on it)
	float Z = A.bHaveGround ? A.GroundZ : static_cast<float>(Player.Pos.Z) - 90.f;
	float Hit = 0.f;
	if (TraceGround(A.Pos, Z, Hit)) { Z = Hit; }
	A.GroundZ = Z;
	A.bHaveGround = true;

	ADhCrowdCharacter* Actor = nullptr;
	for (ADhCrowdCharacter* C : ActorPool)
	{
		if (C && !C->IsInUse()) { Actor = C; break; }
	}
	const FVector Loc(A.Pos.X, A.Pos.Y, Z + 90.f * A.Scale + 2.f);
	const FRotator Rot(0.f, static_cast<float>(A.Dir.Rotation().Yaw), 0.f);
	if (!Actor)
	{
		if (ActorPool.Num() >= S->HeroCount + S->SkeletalCount + 8) { return false; }
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		UClass* Cls = S->CharacterClass ? S->CharacterClass.Get() : ADhCrowdCharacter::StaticClass();
		Actor = GetWorld()->SpawnActor<ADhCrowdCharacter>(Cls, FTransform(Rot, Loc), Params);
		if (!Actor) { return false; }
		ActorPool.Add(Actor);
	}
	Actor->SetActorLocationAndRotation(Loc, Rot, false, nullptr, ETeleportType::TeleportPhysics);
	Actor->Acquire(A, *Visual, SkeletalMaterial, S->TintParameter, S->SkinParameter, bHero);
	A.Actor = Actor;
	return true;
}

bool UDhCrowdSubsystem::EnterInstanced(FDhCrowdAgent& A, bool bFar)
{
	if (!A.bHaveGround)
	{
		float Hit = 0.f;
		if (TraceGround(A.Pos, static_cast<float>(Player.Pos.Z) - 90.f, Hit)) { A.GroundZ = Hit; A.bHaveGround = true; }
	}

	if (bUseMass && MassBridge.Spawn(A)) { return true; }

	if (!Renderer) { return false; }
	int32 Slot = Renderer->AllocSlot(bFar);
	if (Slot == INDEX_NONE) { return false; }
	A.Slot = Slot;
	Renderer->SetSlotColor(bFar, Slot, A.Tint, A.Skin);
	Renderer->SetSlot(bFar, Slot, FVector(A.Pos.X, A.Pos.Y, A.bHaveGround ? A.GroundZ : A.Pos.Z), static_cast<float>(A.Dir.Rotation().Yaw), A.Scale, A.Phase, false);
	return true;
}

// =====================================================================================================
// Public API
// =====================================================================================================

void UDhCrowdSubsystem::SetCrowdDensity(float NewDensity)
{
	// snap to the three player settings when close (0.5 / 0.75 / 1.0), accept anything in between for tools
	Density = FMath::Clamp(NewDensity, 0.25f, 1.0f);
	SyncGovernor();
}

void UDhCrowdSubsystem::SetEventActive(bool bActive, float Multiplier)
{
	bEventActive = bActive;
	EventMultiplier = FMath::Max(Multiplier, 0.1f);
	SyncGovernor();
}

void UDhCrowdSubsystem::SetFocusActor(AActor* Actor)
{
	FocusActor = Actor;
}

void UDhCrowdSubsystem::ReportDisturbance(FVector WorldLocation, float RadiusM, float FleeSeconds)
{
	const float R2 = Square(RadiusM * 100.f);
	for (FDhCrowdAgent& A : Agents)
	{
		if (A.Tier == EDhCrowdTier::Dormant) { continue; }
		if (FVector::DistSquaredXY(A.Pos, WorldLocation) > R2) { continue; }
		if (A.bHasMassEntity) { MassBridge.Panic(A, Net, WorldLocation, FleeSeconds); }
		else { FDhCrowdSim::Panic(Net, A.Walker, A.Motion, WorldLocation, FleeSeconds); }
	}
}

FDhCrowdStats UDhCrowdSubsystem::GetStats() const
{
	FDhCrowdStats St;
	St.Location = LocationId;
	St.Hours = GetHours();
	St.PoolSize = Agents.Num();
	St.Active = ActiveCount;
	St.Hero = TierCounts[static_cast<int32>(EDhCrowdTier::Hero)];
	St.Skeletal = TierCounts[static_cast<int32>(EDhCrowdTier::Skeletal)];
	St.NearInstanced = TierCounts[static_cast<int32>(EDhCrowdTier::NearInstanced)];
	St.FarInstanced = TierCounts[static_cast<int32>(EDhCrowdTier::FarInstanced)];
	St.Visible = St.Hero + St.Skeletal + St.NearInstanced + St.FarInstanced;
	St.VisibleCap = CurrentVisibleCap;
	St.Density = Density;
	St.bMassBackend = bUseMass;
	St.Lanes = Net.NumLanes();
	return St;
}

void UDhCrowdSubsystem::DebugPrint() const
{
	const FDhCrowdStats St = GetStats();
	const FString Line = FString::Printf(TEXT("Crowd %s %02d:%02d  active %d/%d  visible %d/%d  hero %d skel %d near %d far %d  density %.2f  %s"),
		*St.Location.ToString(), FMath::FloorToInt(St.Hours), FMath::FloorToInt(FMath::Frac(St.Hours) * 60.f),
		St.Active, St.PoolSize, St.Visible, St.VisibleCap, St.Hero, St.Skeletal, St.NearInstanced, St.FarInstanced,
		St.Density, St.bMassBackend ? TEXT("Mass") : TEXT("Pooled"));
	if (GEngine) { GEngine->AddOnScreenDebugMessage(static_cast<uint64>(0xD4C0DE), 1.f, FColor::Cyan, Line); }
	UE_LOG(LogDhCrowd, Verbose, TEXT("%s"), *Line);
}
