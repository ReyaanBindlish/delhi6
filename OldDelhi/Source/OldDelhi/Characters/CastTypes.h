#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CastTypes.generated.h"

/** Story act, as used for outfits and Daya's body state. Prefixed "Cast" to stay clear of other modules' enums. */
UENUM(BlueprintType)
enum class ECastAct : uint8
{
	Act1_Gangster,     // Ghaziabad / East Delhi, 2019
	Act2_Fall,         // the begging bowl, 2019-2022
	Act3_Training,     // 2022
	Act4_Karachi,      // 2023-2024
	Act5_Extraction,   // 2024
	Act6_EmptyHouse    // 2024-2025
};

/** Clothing season. Delhi and Karachi share the mapping used by UBodyStateComponent::SeasonFromMonth. */
UENUM(BlueprintType)
enum class ECastSeason : uint8
{
	Summer,
	Monsoon,
	Winter
};

/** The three authored anchor looks of Daya (the component blends continuously between them by weight). */
UENUM(BlueprintType)
enum class ECastBodyState : uint8
{
	Healthy2019,   // Act 1: baseline weight
	Beggar,        // Act 2: baseline minus 14 kg
	Diseased       // Acts 5-6: baseline minus 29 kg (TB)
};

/** Locomotion family: picks the animation set and the physics profile (see docs/CHARACTERS.md section 4). */
UENUM(BlueprintType)
enum class ECastGait : uint8
{
	Standard,
	Lean,         // wiry, quick
	Heavy,        // big frame, wide base
	KneeLimp,     // a permanently wrecked knee (Ravi)
	Elderly,      // slower, shorter stride, upright
	ElderlyStooped,
	Child,
	WheeledBoard, // seated on a low wheeled board, pushes with hands (Mangal)
	Waddle        // heavy, diabetic, careful (Haji Sattar)
};

/**
 * One row of DT_Cast.csv. Column headers in the CSV must equal these property names.
 * Free-text columns hold a single line; ';' separates items, '|' separates alternatives.
 */
USTRUCT(BlueprintType)
struct OLDDELHI_API FCastMemberRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Role;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Faction;

	// ---- body ------------------------------------------------------------------------------------
	/** Age when the game starts (October 2019). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AgeAtStart = 30;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float HeightCm = 170.f;
	/** Baseline (first appearance) weight. Daya's later weights come from UBodyStateComponent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float WeightKg = 70.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Build;

	// ---- physics / movement ----------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float CapsuleHalfHeightCm = 88.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float CapsuleRadiusCm = 34.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float EyeHeightCm = 160.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaxStepHeightCm = 45.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float WalkSpeedCmS = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float RunSpeedCmS = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) ECastGait Gait = ECastGait::Standard;

	// ---- look ------------------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Skin;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Hair;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Beard;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString OutfitSummer;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString OutfitMonsoon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString OutfitWinter;
	/** Per-act changes, "A1=...;A2=...". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString OutfitByAct;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Accessories;

	// ---- behaviour -------------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString GaitNotes;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Tells;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Voice;
	/** "Pa-Lahori=60;Ur=25;Hi=15" style, same convention as DT_Archetypes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Languages;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ActsPresent;

	// ---- authoring -------------------------------------------------------------------------------
	/** Asset names created by setup_characters.py ("MH_Daya_A1|MH_Daya_A2|MH_Daya_A6"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString MetaHumanAssets;
	/** UE mannequin fallback: "SKM_Manny" or "SKM_Quinn". Scaled by height and BMI at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString FallbackMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString SensitivityNote;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bPlayable = false;
	/** True only for Daya: the body changes with act and disease (UBodyStateComponent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bHasBodyStates = false;
};
