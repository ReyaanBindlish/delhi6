#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Math/RandomStream.h"
#include "Crowd/DhCrowdCharacter.h"
#include "Crowd/DhCrowdLaneNetwork.h"
#include "Crowd/DhCrowdMassBridge.h"
#include "Crowd/DhCrowdTypes.h"
#include "DhCrowdSubsystem.generated.h"

class ADhCrowdInstanceRenderer;
class UDataTable;
class UDelhiPopulationGovernor;
class UDelhiWeatherSubsystem;
class UStaticMesh;
class UMaterialInterface;

/**
 * Ambient crowd director (research/05 + 07): holds a pool of up to 5000 agents per location, asks
 * UDelhiPopulationGovernor how many people each street cell should hold at the current time of day,
 * weather and event, activates / retires agents to match (never popping in view), and gives the nearest
 * ones a body: 12 hero + ~40 skeletal pooled characters, ~150 near-instanced, the rest far-instanced up to
 * the visible cap (600, 750 on events). The rest are simulated as abstract points on their lane.
 *
 * Backends: Pooled (ACharacter pool + instanced static mesh, no extra plugins) or Mass (far tiers become
 * Mass entities with MassCrowd visualisation). Mass falls back to Pooled on any setup failure.
 */
UCLASS()
class OLDDELHI_API UDhCrowdSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// UWorldSubsystem
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	// UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDhCrowdSubsystem, STATGROUP_Tickables); }

	// ---- Player-facing settings
	/** 0.5 / 0.75 / 1.0 (Low / Medium / High). Applied to the governor and the next rebalance. */
	UFUNCTION(BlueprintCallable, Category = "Crowd") void SetCrowdDensity(float NewDensity);
	UFUNCTION(BlueprintPure, Category = "Crowd") float GetCrowdDensity() const { return Density; }

	// ---- World hooks
	/** Switch location (karachi_lyari, nizamuddin, vasundhara, old_delhi...). Reloads tables and lanes, resets agents. */
	UFUNCTION(BlueprintCallable, Category = "Crowd") bool SetLocation(FName NewLocation);
	/** Event layer (Friday prayers, baraat, Eid): raises the governor multiplier and the visible cap to the hard cap. */
	UFUNCTION(BlueprintCallable, Category = "Crowd") void SetEventActive(bool bActive, float Multiplier = 1.5f);
	/** Gunfire or a crash: everyone within RadiusM runs away along their lane (bible: guns have consequences). */
	UFUNCTION(BlueprintCallable, Category = "Crowd") void ReportDisturbance(FVector WorldLocation, float RadiusM = 60.f, float FleeSeconds = 14.f);
	/** Pin the crowd to a specific actor (cutscenes, spectator) instead of player 0's pawn. */
	UFUNCTION(BlueprintCallable, Category = "Crowd") void SetFocusActor(AActor* Actor);
	/** Override time of day (hours) for tests; negative = follow UDelhiWeatherSubsystem. */
	UFUNCTION(BlueprintCallable, Category = "Crowd") void SetHoursOverride(float Hours) { HoursOverride = Hours; }
	/** Offset (cm) from the lane file frame (tile centre) to the level frame; call before SetLocation. */
	UFUNCTION(BlueprintCallable, Category = "Crowd") void SetLaneOffset(FVector OffsetCm) { LaneOffsetCm = OffsetCm; }
	UFUNCTION(BlueprintCallable, Category = "Crowd") void SetPaused(bool bInPaused) { bPaused = bInPaused; }
	UFUNCTION(BlueprintPure, Category = "Crowd") bool IsPaused() const { return bPaused; }
	/** Immediate full rebalance (loading screens, teleports): fills the street without the 60-120 s convergence. */
	UFUNCTION(BlueprintCallable, Category = "Crowd") void WarmStart();
	UFUNCTION(BlueprintPure, Category = "Crowd") FDhCrowdStats GetStats() const;

	// ---- Native access (Mass processors, tests)
	const FDhCrowdLaneNetwork& GetLaneNetwork() const { return Net; }
	FDhCrowdPlayerInfo GetPlayerInfo() const { return Player; }
	bool UsesMass() const { return bUseMass; }
	const TArray<FDhCrowdArchetypeInfo>& GetArchetypes() const { return Archetypes; }

	void DebugPrint() const;

private:
	// ---- Setup
	void InitWorld();
	void LoadVisuals();
	void CreateRenderer();
	bool LoadLocationData();
	UDataTable* LoadCsvTable(UScriptStruct* RowStruct, const FString& FileName) const;
	void BuildArchetypeInfos(const UDataTable* Table);
	void ResetAgents();

	// ---- Per tick
	void UpdatePlayer();
	void SimulateVisible(float Dt, double Now);
	void SimulateAbstract(double Now);
	void UpdateGround();
	void UpdateTiers();
	void Rebalance(int32 SpawnBudgetOverride = 0);
	void SyncGovernor();

	// ---- Agents
	bool TryActivate(const FIntPoint& Cell, float Hours, const TMap<FName, float>& DensityCache);
	void InitAgent(FDhCrowdAgent& A, int32 LaneIdx, float Dist, float Hours);
	int32 ChooseArchetype(FName Street, float Hours, FRandomStream& R) const;
	FLinearColor PickTint(const FDhCrowdArchetypeInfo& Info, FRandomStream& R) const;
	void Retire(int32 Index);
	bool InViewCone(const FVector& P, float MinDot) const;
	bool CanRetire(const FDhCrowdAgent& A, float D2, float VisR) const;
	float GetHours() const;

	// ---- Representation
	void SetTier(FDhCrowdAgent& A, EDhCrowdTier NewTier);
	void LeaveTier(FDhCrowdAgent& A);
	bool AcquireActor(FDhCrowdAgent& A, bool bHero);
	bool EnterInstanced(FDhCrowdAgent& A, bool bFar);
	bool TraceGround(const FVector& XY, float StartZ, float& OutZ) const;
	const FDhCrowdLoadedVisual* GetVisual(bool bFemale) const;

	// ---- State
	bool bWorldReady = false;
	bool bPaused = false;
	bool bUseMass = false;
	bool bEventActive = false;
	bool bWarmStarting = false;
	bool bPendingWarmStart = false;
	FVector LaneOffsetCm = FVector::ZeroVector;
	float EventMultiplier = 1.f;
	float Density = 1.f;
	float HoursOverride = -1.f;
	FName LocationId;
	EDhCrowdRegion Region = EDhCrowdRegion::Karachi;

	FDhCrowdLaneNetwork Net;
	FDhCrowdPlayerInfo Player;
	TWeakObjectPtr<AActor> FocusActor;
	FRandomStream Rng;

	TArray<FDhCrowdAgent> Agents;
	TArray<int32> FreeDormant;
	TArray<int32> VisibleIdx;
	TArray<FDhCrowdArchetypeInfo> Archetypes;

	float TierTimer = 0.f;
	float RebalanceTimer = 0.f;
	int32 AbstractCursor = 0;
	int32 GroundCursor = 0;
	int32 TierCounts[6] = { 0, 0, 0, 0, 0, 0 };
	int32 ActiveCount = 0;
	int32 CurrentVisibleCap = 600;

	// ---- Owned assets (GC-safe)
	UPROPERTY(Transient) TObjectPtr<UDataTable> ArchetypeTable;
	UPROPERTY(Transient) TObjectPtr<UDataTable> StreetDensityTable;
	UPROPERTY(Transient) TArray<FDhCrowdLoadedVisual> LoadedVisuals;
	UPROPERTY(Transient) TArray<TObjectPtr<ADhCrowdCharacter>> ActorPool;
	UPROPERTY(Transient) TObjectPtr<ADhCrowdInstanceRenderer> Renderer;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> SkeletalMaterial;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> InstancedMeshAsset;

	FDhCrowdMassBridge MassBridge;
};
