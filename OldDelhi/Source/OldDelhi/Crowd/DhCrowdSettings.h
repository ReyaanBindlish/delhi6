#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DhCrowdTypes.h"
#include "DhCrowdSettings.generated.h"

class ADhCrowdCharacter;
class UDataTable;
class UMassEntityConfigAsset;
class UMaterialInterface;
class UStaticMesh;

/**
 * Project Settings > Game > Dhurandhar Crowd. Defaults follow research/05 section 1.5:
 * 5000 simulated per location, ~600 visible (hard cap 750), 12 hero + ~40 skeletal + ~150 near-instanced.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Dhurandhar Crowd"))
class OLDDELHI_API UDhCrowdSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDhCrowdSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	static const UDhCrowdSettings* Get() { return GetDefault<UDhCrowdSettings>(); }

	/** Master switch. Off = the subsystem does nothing (use for menus and tests). */
	UPROPERTY(Config, EditAnywhere, Category = "General") bool bEnableCrowd = true;
	/** Location id; selects the archetype table and the lane file. E.g. karachi_lyari, nizamuddin, vasundhara, old_delhi. */
	UPROPERTY(Config, EditAnywhere, Category = "General") FName DefaultLocation = TEXT("karachi_lyari");
	/** Player setting: 0.5 / 0.75 / 1.0 (Low / Medium / High). */
	UPROPERTY(Config, EditAnywhere, Category = "General", meta = (ClampMin = "0.25", ClampMax = "1.0")) float CrowdDensity = 1.0f;
	/** Drive UDelhiPopulationGovernor weather multiplier from UDelhiWeatherSubsystem wetness. */
	UPROPERTY(Config, EditAnywhere, Category = "General") bool bDriveGovernorFromWeather = true;
	UPROPERTY(Config, EditAnywhere, Category = "General") bool bShowDebug = false;

	// ---- Population budget
	UPROPERTY(Config, EditAnywhere, Category = "Population", meta = (ClampMin = "100", ClampMax = "8000")) int32 TotalPopulation = 5000;
	/** Typical visible peak (hard cap for normal play). */
	UPROPERTY(Config, EditAnywhere, Category = "Population") int32 VisibleCap = 600;
	/** Cap while an event (Friday prayers, baraat, Eid) is active. */
	UPROPERTY(Config, EditAnywhere, Category = "Population") int32 EventVisibleCap = 750;
	/** Most agents simulated at once (research/05: ~2,500 active near the player). */
	UPROPERTY(Config, EditAnywhere, Category = "Population") int32 ActiveSimCap = 2500;
	UPROPERTY(Config, EditAnywhere, Category = "Population") int32 HeroCount = 12;
	UPROPERTY(Config, EditAnywhere, Category = "Population") int32 SkeletalCount = 40;
	UPROPERTY(Config, EditAnywhere, Category = "Population") int32 NearInstancedCount = 150;

	// ---- Radii (metres)
	UPROPERTY(Config, EditAnywhere, Category = "Radii") float SimRadiusM = 320.f;
	UPROPERTY(Config, EditAnywhere, Category = "Radii") float VisibleRadiusM = 150.f;
	UPROPERTY(Config, EditAnywhere, Category = "Radii") float HeroRadiusM = 8.f;
	UPROPERTY(Config, EditAnywhere, Category = "Radii") float SkeletalRadiusM = 25.f;
	UPROPERTY(Config, EditAnywhere, Category = "Radii") float NearInstancedRadiusM = 60.f;
	/** Agents never spawn closer than this to the player, nor inside the view cone nearer than NoPopRadiusM. */
	UPROPERTY(Config, EditAnywhere, Category = "Radii") float MinSpawnDistanceM = 12.f;
	UPROPERTY(Config, EditAnywhere, Category = "Radii") float NoPopInViewRadiusM = 70.f;
	/** Share of the street density that is simulated beyond the visible radius (they walk in later). */
	UPROPERTY(Config, EditAnywhere, Category = "Radii") float OuterDensityFactor = 0.35f;

	// ---- Pacing
	UPROPERTY(Config, EditAnywhere, Category = "Pacing") float RebalanceIntervalS = 1.5f;
	UPROPERTY(Config, EditAnywhere, Category = "Pacing") float TierIntervalS = 0.25f;
	UPROPERTY(Config, EditAnywhere, Category = "Pacing") int32 MaxSpawnPerRebalance = 60;
	UPROPERTY(Config, EditAnywhere, Category = "Pacing") int32 MaxTierChangesPerUpdate = 24;
	UPROPERTY(Config, EditAnywhere, Category = "Pacing") int32 GroundTracesPerTick = 24;
	UPROPERTY(Config, EditAnywhere, Category = "Pacing") float AbstractStepS = 1.0f;

	// ---- Backend (Mass is opt-in until the Mass config asset is verified in the editor)
	UPROPERTY(Config, EditAnywhere, Category = "Backend") EDhCrowdBackend Backend = EDhCrowdBackend::Pooled;
	/** Entity config with UDhCrowdAgentTrait + Crowd Visualization trait (made by Tools/ue/setup_crowd.py). */
	UPROPERTY(Config, EditAnywhere, Category = "Backend", meta = (AllowedClasses = "/Script/MassSpawner.MassEntityConfigAsset"))
	FSoftObjectPath MassEntityConfig = FSoftObjectPath(TEXT("/Game/Crowd/Mass/DA_DhCrowdEntityConfig.DA_DhCrowdEntityConfig"));

	// ---- Visuals (stand-ins until MetaHumans exist)
	UPROPERTY(Config, EditAnywhere, Category = "Visuals") TSubclassOf<ADhCrowdCharacter> CharacterClass;
	/** First set whose mesh loads is used; female sets are used for female agents when present. */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals") TArray<FDhCrowdVisualSet> VisualSets;
	UPROPERTY(Config, EditAnywhere, Category = "Visuals", meta = (AllowedClasses = "/Script/Engine.MaterialInterface"))
	FSoftObjectPath SkeletalMaterial = FSoftObjectPath(TEXT("/Game/Crowd/Materials/MI_CrowdSkeletal.MI_CrowdSkeletal"));
	UPROPERTY(Config, EditAnywhere, Category = "Visuals", meta = (AllowedClasses = "/Script/Engine.StaticMesh"))
	FSoftObjectPath InstancedMesh = FSoftObjectPath(TEXT("/Game/Crowd/Meshes/SM_CrowdProxy.SM_CrowdProxy"));
	UPROPERTY(Config, EditAnywhere, Category = "Visuals", meta = (AllowedClasses = "/Script/Engine.MaterialInterface"))
	FSoftObjectPath InstancedMaterial = FSoftObjectPath(TEXT("/Game/Crowd/Materials/MI_CrowdInstanced.MI_CrowdInstanced"));
	/** Engine fallback when SM_CrowdProxy does not exist yet (a cylinder scaled to a person). */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals") FSoftObjectPath FallbackInstancedMesh = FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UPROPERTY(Config, EditAnywhere, Category = "Visuals") FName TintParameter = TEXT("BodyTint");
	UPROPERTY(Config, EditAnywhere, Category = "Visuals") FName SkinParameter = TEXT("SkinTone");

	// ---- Data
	/** Optional DataTable assets (FDelhiArchetypeRow / FDelhiStreetDensityRow). CSVs in Data/ are used when unset. */
	UPROPERTY(Config, EditAnywhere, Category = "Data", meta = (AllowedClasses = "/Script/Engine.DataTable")) FSoftObjectPath ArchetypeTableKarachi;
	UPROPERTY(Config, EditAnywhere, Category = "Data", meta = (AllowedClasses = "/Script/Engine.DataTable")) FSoftObjectPath ArchetypeTableDelhiNCR;
	UPROPERTY(Config, EditAnywhere, Category = "Data", meta = (AllowedClasses = "/Script/Engine.DataTable")) FSoftObjectPath ArchetypeTableOldDelhi;
	UPROPERTY(Config, EditAnywhere, Category = "Data", meta = (AllowedClasses = "/Script/Engine.DataTable")) FSoftObjectPath StreetDensityStory;
	/** Folder holding DT_*.csv, relative to the project directory. */
	UPROPERTY(Config, EditAnywhere, Category = "Data") FString CsvFolder = TEXT("Data");
	/** Folders searched (relative to the project directory) for <location>_lanes.json. */
	UPROPERTY(Config, EditAnywhere, Category = "Data") TArray<FString> LaneFolders;
};
