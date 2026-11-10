#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "CastTypes.h"
#include "DhurandharCastDataAsset.h"
#include "../DayaVitalsComponent.h"
#include "BodyStateComponent.generated.h"

class USkeletalMesh;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class UAnimMontage;
class UAnimInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCastBodyStateChanged, ECastBodyState, NewState, float, WeightKg);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCastOutfitChanged, FName, OutfitTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBodyCough, float, LockSeconds);

/**
 * Drives how a principal character looks and moves as the story ages them.
 *
 * Daya (bHasBodyStates): weight = Act 1 weight + per-act delta - disease loss. The disease loss is read
 * live from UDayaVitalsComponent (max stamina shrinking from BaseMaxStamina towards MinMaxStamina) so the
 * weight loss the player sees is the same ledger the stamina meter uses; NotifyDiseaseStage() gives a floor
 * when the story sets a stage before the stamina curve has moved. The blended result drives morph
 * targets, material scalars (gaunt, pallor, dirt, sweat), a hunch angle and a gait scale for the anim BP.
 * Meshes and animation Blueprints of the nearest anchor are swapped when the character is off camera.
 *
 * Everyone else: constant weight from DT_Cast; outfits by act and season; when the mesh has no morph
 * targets (UE mannequin fallback) the mesh component is scaled by height and BMI.
 *
 * Attach to the character (or any actor with a skeletal mesh). Works for the mannequin fallback and for a
 * combined MetaHuman face+body mesh.
 */
UCLASS(ClassGroup = (Story), meta = (BlueprintSpawnableComponent))
class OLDDELHI_API UBodyStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBodyStateComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ---- story hooks ---------------------------------------------------------------------------------
	/** Change act. bSnap = time skip (jump weight immediately and swap mesh now). */
	UFUNCTION(BlueprintCallable, Category = "Cast|Body") void SetAct(ECastAct NewAct, bool bSnap = true);
	UFUNCTION(BlueprintCallable, Category = "Cast|Body") void SetSeason(ECastSeason NewSeason);
	UFUNCTION(BlueprintCallable, Category = "Cast|Body") void SetActAndSeason(ECastAct NewAct, ECastSeason NewSeason, bool bSnap = true);
	/** Story-set disease stage (floor on weight loss). Safe to call alongside UDayaVitalsComponent::SetDiseaseStage. */
	UFUNCTION(BlueprintCallable, Category = "Cast|Body") void NotifyDiseaseStage(EDiseaseStage Stage);
	/** Jump to the current target weight and swap meshes immediately (cutscene cuts, loading a save). */
	UFUNCTION(BlueprintCallable, Category = "Cast|Body") void SnapToTarget();

	/** Delhi and Karachi: Mar-Jun and early Oct summer, Jul-Sep monsoon, Nov-Feb winter. */
	UFUNCTION(BlueprintPure, Category = "Cast|Body") static ECastSeason SeasonFromMonth(int32 Month);

	// ---- reads for animation Blueprints and UI -----------------------------------------------------------
	UFUNCTION(BlueprintPure, Category = "Cast|Body") float GetCurrentWeightKg() const { return CurrentWeightKg; }
	UFUNCTION(BlueprintPure, Category = "Cast|Body") float GetTargetWeightKg() const { return TargetWeightKg; }
	UFUNCTION(BlueprintPure, Category = "Cast|Body") float GetBmi() const;
	UFUNCTION(BlueprintPure, Category = "Cast|Body") float GetHunchDegrees() const { return HunchDegrees; }
	UFUNCTION(BlueprintPure, Category = "Cast|Body") float GetGaitSpeedScale() const { return GaitSpeedScale; }
	UFUNCTION(BlueprintPure, Category = "Cast|Body") float GetFrailty() const { return Frailty; }
	UFUNCTION(BlueprintPure, Category = "Cast|Body") ECastBodyState GetBodyState() const { return CurrentState; }
	UFUNCTION(BlueprintPure, Category = "Cast|Body") ECastAct GetAct() const { return Act; }
	UFUNCTION(BlueprintPure, Category = "Cast|Body") ECastSeason GetSeason() const { return Season; }
	/** 0 at full health, 1 at the Act 6 floor (from vitals, or from the notified stage). */
	UFUNCTION(BlueprintPure, Category = "Cast|Body") float GetDiseaseFraction() const { return DiseaseFraction; }

	UPROPERTY(BlueprintAssignable, Category = "Cast|Body") FOnCastBodyStateChanged OnBodyStateChanged;
	UPROPERTY(BlueprintAssignable, Category = "Cast|Body") FOnCastOutfitChanged OnOutfitChanged;
	UPROPERTY(BlueprintAssignable, Category = "Cast|Body") FOnBodyCough OnBodyCough;

	// ---- configuration ---------------------------------------------------------------------------------
	/** Cast table and anchors. If empty the built-in defaults (Daya 78/64/49 kg) are used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast") TObjectPtr<UDhurandharCastDataAsset> CastData;
	/** Row name in DT_Cast: Daya, Sukhi, Gurnoor, Bibi, Bablu, Mangal, Khanna, Ravi, Qureshi, Sattar, Shahzaib. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast") FName CastId = TEXT("Daya");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast") ECastAct Act = ECastAct::Act1_Gangster;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast") ECastSeason Season = ECastSeason::Summer;

	/** Follow UDayaVitalsComponent on the same actor (max stamina -> disease weight loss). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Link") bool bAutoLinkVitals = true;
	/** Pick the season from UStoryCalendarSubsystem every few seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Link") bool bAutoSeasonFromCalendar = false;
	/** Play CoughMontage and fire OnBodyCough when the vitals component starts a coughing fit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Link") bool bReactToCoughFits = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Link") TSoftObjectPtr<UAnimMontage> CoughMontage;

	// ---- Daya's weight model (kg) ------------------------------------------------------------------------
	/** Delta from Act 1 weight per act, excluding disease: Act 2 -14, then partial recovery in training. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Weight") TMap<ECastAct, float> WeightDeltaKgByAct;
	/** Loss at full disease progress. Act 6 floor = Act 1 - 4 - 25 = 49 kg, i.e. minus 29. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Weight") float MaxDiseaseLossKg = 25.f;
	/** Minimum loss applied once the story sets a stage (before the stamina curve has moved). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Weight") TMap<EDiseaseStage, float> StageFloorLossKg;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Weight") float MinWeightKg = 45.f;
	/** Visual transition speed when the target moves (kg per real second). Time skips use SnapToTarget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Weight") float WeightRateKgPerSecond = 0.25f;

	// ---- mesh swap and fallback ---------------------------------------------------------------------------
	/** Swap the anchor mesh only while the mesh was not rendered recently (no visible pop). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Mesh") bool bSwapMeshOnlyWhenHidden = true;
	/** Swap anyway after waiting this long. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Mesh") float MaxSwapDeferSeconds = 20.f;
	/** Scale the mesh by height and BMI when it has no morph targets (UE mannequin). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Mesh") bool bAllowFallbackScaling = true;
	/** SKM_Manny / SKM_Quinn stand about 180 cm and read as BMI 22. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Mesh") float FallbackReferenceHeightCm = 180.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Mesh") float FallbackReferenceBmi = 22.f;

private:
	void ResolveMesh();
	void ResolveVitals();
	void BuildAnchors();
	void RefreshMorphSupport();
	float ComputeTargetWeight();
	void ApplyBody();
	void ApplyFallbackScale();
	void EnsureMaterialInstances();
	void RequestAnchorSwap(const FCastBodyAnchor& Anchor);
	void OnAnchorAssetsLoaded();
	void CommitPendingSwap();
	void RefreshOutfit();
	void OnOutfitAssetsLoaded(FName Tag, TArray<TSoftObjectPtr<USkeletalMesh>> Pieces);
	void RefreshOutfitLeaderPose();
	bool UsesAnchors() const;

	UFUNCTION() void HandleCoughFit(float LockSeconds);

	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> MeshComp;
	UPROPERTY(Transient) TObjectPtr<UDayaVitalsComponent> Vitals;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> MaterialInstances;
	UPROPERTY(Transient) TArray<TObjectPtr<USkeletalMeshComponent>> OutfitComponents;
	UPROPERTY(Transient) TObjectPtr<USkeletalMesh> PendingMesh;
	UPROPERTY(Transient) TSubclassOf<UAnimInstance> PendingAnimClass;

	FCastMemberRow Row;
	bool bHasRow = false;
	TArray<FCastBodyAnchor> Anchors;       // sorted by weight, heaviest first
	TArray<FName> AllMorphNames;
	TArray<FName> AllScalarNames;

	TSoftObjectPtr<USkeletalMesh> PendingMeshSoft;
	TSoftClassPtr<UAnimInstance> PendingAnimSoft;
	TSoftObjectPtr<USkeletalMesh> CurrentMeshSoft;
	TSharedPtr<FStreamableHandle> SwapHandle;
	TSharedPtr<FStreamableHandle> OutfitHandle;
	bool bSwapPending = false;
	bool bSwapAssetsReady = false;
	bool bForceSwap = false;
	float SwapWaitSeconds = 0.f;

	EDiseaseStage NotifiedStage = EDiseaseStage::Healthy;
	bool bStageNotified = false;
	float DiseaseFraction = 0.f;

	float CurrentWeightKg = 70.f;
	float TargetWeightKg = 70.f;
	float HunchDegrees = 0.f;
	float GaitSpeedScale = 1.f;
	float Frailty = 0.f;
	ECastBodyState CurrentState = ECastBodyState::Healthy2019;
	bool bStateInitialised = false;
	bool bAnyMorphOnMesh = false;
	bool bMeshFromAnchor = false;
	bool bBodyDirty = true;
	FVector BaseMeshScale = FVector::OneVector;
	float RelinkTimer = 0.f;
	float SeasonTimer = 0.f;
	FName CurrentOutfitTag = NAME_None;
};
