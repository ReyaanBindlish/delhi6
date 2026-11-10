#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "CastTypes.h"
#include "DhurandharCastDataAsset.generated.h"

class USkeletalMesh;
class UAnimInstance;

/**
 * One authored look of Daya at a given weight. UBodyStateComponent sorts anchors by weight and blends
 * morph targets, material scalars, hunch and gait scale linearly between the two neighbours of the
 * current weight. The Mesh/AnimClass of the nearest anchor are swapped in (deferred until off camera).
 *
 * Morph and material names are a contract with the art: see docs/CHARACTERS.md section 5.
 */
USTRUCT(BlueprintType)
struct OLDDELHI_API FCastBodyAnchor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECastBodyState State = ECastBodyState::Healthy2019;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WeightKg = 78.f;
	/** Optional full-body mesh for this look (the MetaHuman assembly output). Empty = keep current mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<USkeletalMesh> Mesh;
	/** Optional animation Blueprint for this look (posture, stride, breathing). Empty = keep current. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftClassPtr<UAnimInstance> AnimClass;
	/** Morph target name -> weight at this anchor. Missing names count as 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FName, float> Morphs;
	/** Material scalar parameter -> value at this anchor (applied to every material slot via MIDs). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FName, float> MaterialScalars;
	/** Forward spine lean in degrees, read by the anim BP via UBodyStateComponent::GetHunchDegrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HunchDegrees = 0.f;
	/** Multiplier on stride speed and play rate (the movement speed itself stays with DayaVitals). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float GaitSpeedScale = 1.f;
	/** 0 = robust, 1 = frail. Drives breathing amplitude and idle choice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Frailty = 0.f;
};

/** Which clothing pieces a character wears in a given act and season. */
USTRUCT(BlueprintType)
struct OLDDELHI_API FCastOutfitEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName CastId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECastAct Act = ECastAct::Act1_Gangster;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECastSeason Season = ECastSeason::Summer;
	/** Name matching the table text, e.g. "A1_Summer_CheckedShirt". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName OutfitTag;
	/** Leader-pose clothing, shoe and groom (beard/hair card) meshes sharing the body skeleton. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TSoftObjectPtr<USkeletalMesh>> Pieces;
};

/** Meshes and animation for one cast member (everything optional; the fallback is a UE mannequin). */
USTRUCT(BlueprintType)
struct OLDDELHI_API FCastVisualSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<USkeletalMesh> BodyMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftClassPtr<UAnimInstance> AnimClass;
	/** The MetaHumanCharacter asset (kept as UObject so this module needs no MetaHuman dependency). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UObject> MetaHumanCharacter;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<USkeletalMesh> FallbackMesh;
};

/**
 * The principal cast in one asset: the DT_Cast table, per-character visuals, outfits and Daya's anchors.
 * Create one at /Game/Characters/Cast/DA_Cast (setup_characters.py does this).
 */
UCLASS(BlueprintType)
class OLDDELHI_API UDhurandharCastDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UDhurandharCastDataAsset();

	/** Imported from OldDelhi/Data/DT_Cast.csv (row struct FCastMemberRow). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast") TSoftObjectPtr<UDataTable> CastTable;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast") TMap<FName, FCastVisualSet> Visuals;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast") TArray<FCastOutfitEntry> Outfits;
	/** Daya's three authored looks. Defaults are filled in the constructor: 78 / 64 / 49 kg. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast|Daya") TArray<FCastBodyAnchor> DayaAnchors;

	/** Row lookup by CastId (Daya, Sukhi, Gurnoor, Bibi, Bablu, Mangal, Khanna, Ravi, Qureshi, Sattar, Shahzaib). */
	const FCastMemberRow* FindCastRow(FName CastId) const;

	UFUNCTION(BlueprintCallable, Category = "Cast") bool GetCastRow(FName CastId, FCastMemberRow& OutRow) const;
	UFUNCTION(BlueprintCallable, Category = "Cast") TArray<FName> GetAllCastIds() const;

	/** Exact act+season match, else same act with any season, else false. */
	UFUNCTION(BlueprintCallable, Category = "Cast") bool GetOutfit(FName CastId, ECastAct Act, ECastSeason Season, FCastOutfitEntry& OutEntry) const;

	/**
	 * Story invariants: Bablu is 20 kg heavier than Daya, Daya loses 14 kg (Act 2) and 29 kg (Act 6),
	 * Gurnoor is 9, Bibi is 84. Returns false and lists problems if the table or anchors drift.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cast") bool ValidateCastInvariants(TArray<FString>& OutErrors) const;

	/** The shipped default anchors (also used by the component when no data asset is assigned). */
	static TArray<FCastBodyAnchor> MakeDefaultDayaAnchors();
};
