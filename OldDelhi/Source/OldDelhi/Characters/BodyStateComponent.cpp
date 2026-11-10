#include "Characters/BodyStateComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StoryCalendarSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogBodyState, Log, All);

UBodyStateComponent::UBodyStateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.25f;

	// Weight deltas from the Act 1 weight, disease excluded. Act 2 is the bible's minus 14 kg.
	WeightDeltaKgByAct.Add(ECastAct::Act1_Gangster, 0.f);
	WeightDeltaKgByAct.Add(ECastAct::Act2_Fall, -14.f);
	WeightDeltaKgByAct.Add(ECastAct::Act3_Training, -6.f);
	WeightDeltaKgByAct.Add(ECastAct::Act4_Karachi, -4.f);
	WeightDeltaKgByAct.Add(ECastAct::Act5_Extraction, -4.f);
	WeightDeltaKgByAct.Add(ECastAct::Act6_EmptyHouse, -4.f);

	StageFloorLossKg.Add(EDiseaseStage::Healthy, 0.f);
	StageFloorLossKg.Add(EDiseaseStage::Cough, 2.f);
	StageFloorLossKg.Add(EDiseaseStage::WeightLoss, 9.f);
	StageFloorLossKg.Add(EDiseaseStage::Untreated, 16.f);
	StageFloorLossKg.Add(EDiseaseStage::Diagnosed, 22.f);
}

bool UBodyStateComponent::UsesAnchors() const
{
	return bHasRow ? Row.bHasBodyStates : (CastId == FName(TEXT("Daya")));
}

void UBodyStateComponent::BeginPlay()
{
	Super::BeginPlay();

	if (CastData)
	{
		if (const FCastMemberRow* Found = CastData->FindCastRow(CastId))
		{
			Row = *Found;
			bHasRow = true;
		}
		else
		{
			UE_LOG(LogBodyState, Warning, TEXT("CastId '%s' not found in the cast table; using defaults."), *CastId.ToString());
		}
	}

	ResolveMesh();
	if (MeshComp)
	{
		BaseMeshScale = MeshComp->GetRelativeScale3D();
	}
	ResolveVitals();
	BuildAnchors();
	RefreshMorphSupport();

	if (bAutoSeasonFromCalendar)
	{
		SeasonTimer = 0.f;
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GI = World->GetGameInstance())
			{
				if (UStoryCalendarSubsystem* Cal = GI->GetSubsystem<UStoryCalendarSubsystem>())
				{
					Season = SeasonFromMonth(Cal->GetDate().GetMonth());
				}
			}
		}
	}

	SnapToTarget();
	RefreshOutfit();
}

void UBodyStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Vitals)
	{
		Vitals->OnCoughFit.RemoveDynamic(this, &UBodyStateComponent::HandleCoughFit);
	}
	if (SwapHandle.IsValid())
	{
		SwapHandle->CancelHandle();
		SwapHandle.Reset();
	}
	if (OutfitHandle.IsValid())
	{
		OutfitHandle->CancelHandle();
		OutfitHandle.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void UBodyStateComponent::ResolveMesh()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	if (const ACharacter* Char = Cast<ACharacter>(Owner))
	{
		MeshComp = Char->GetMesh();
	}
	if (!MeshComp)
	{
		MeshComp = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}
}

void UBodyStateComponent::ResolveVitals()
{
	if (!bAutoLinkVitals || Vitals || !GetOwner())
	{
		return;
	}
	Vitals = GetOwner()->FindComponentByClass<UDayaVitalsComponent>();
	if (Vitals && bReactToCoughFits)
	{
		Vitals->OnCoughFit.AddUniqueDynamic(this, &UBodyStateComponent::HandleCoughFit);
	}
}

void UBodyStateComponent::BuildAnchors()
{
	Anchors.Reset();
	AllMorphNames.Reset();
	AllScalarNames.Reset();
	if (!UsesAnchors())
	{
		return;
	}

	Anchors = (CastData && CastData->DayaAnchors.Num() > 0) ? CastData->DayaAnchors : UDhurandharCastDataAsset::MakeDefaultDayaAnchors();
	Anchors.Sort([](const FCastBodyAnchor& A, const FCastBodyAnchor& B) { return A.WeightKg > B.WeightKg; });

	for (const FCastBodyAnchor& A : Anchors)
	{
		for (const TPair<FName, float>& P : A.Morphs)
		{
			AllMorphNames.AddUnique(P.Key);
		}
		for (const TPair<FName, float>& P : A.MaterialScalars)
		{
			AllScalarNames.AddUnique(P.Key);
		}
	}
}

void UBodyStateComponent::RefreshMorphSupport()
{
	bAnyMorphOnMesh = false;
	if (!MeshComp)
	{
		return;
	}
	if (const USkeletalMesh* Mesh = MeshComp->GetSkeletalMeshAsset())
	{
		for (const FName& Name : AllMorphNames)
		{
			if (Mesh->FindMorphTarget(Name))
			{
				bAnyMorphOnMesh = true;
				break;
			}
		}
	}
}

float UBodyStateComponent::GetBmi() const
{
	const float HeightM = (bHasRow ? Row.HeightCm : 178.f) * 0.01f;
	return HeightM > KINDA_SMALL_NUMBER ? CurrentWeightKg / (HeightM * HeightM) : 0.f;
}

float UBodyStateComponent::ComputeTargetWeight()
{
	const float BaseWeight = bHasRow ? Row.WeightKg : (UsesAnchors() ? 78.f : 70.f);
	if (!UsesAnchors())
	{
		DiseaseFraction = 0.f;
		return BaseWeight;
	}

	float VitalsLoss = 0.f;
	if (bAutoLinkVitals && Vitals)
	{
		const float Base = Vitals->BaseMaxStamina;
		const float Min = Vitals->MinMaxStamina;
		if (Base > Min)
		{
			const float Fraction = FMath::Clamp((Base - Vitals->GetMaxStamina()) / (Base - Min), 0.f, 1.f);
			VitalsLoss = Fraction * MaxDiseaseLossKg;
		}
	}
	float StageLoss = 0.f;
	if (bStageNotified)
	{
		if (const float* Floor = StageFloorLossKg.Find(NotifiedStage))
		{
			StageLoss = *Floor;
		}
	}
	const float Loss = FMath::Max(VitalsLoss, StageLoss);
	DiseaseFraction = MaxDiseaseLossKg > KINDA_SMALL_NUMBER ? FMath::Clamp(Loss / MaxDiseaseLossKg, 0.f, 1.f) : 0.f;

	const float* Delta = WeightDeltaKgByAct.Find(Act);
	return FMath::Max(BaseWeight + (Delta ? *Delta : 0.f) - Loss, MinWeightKg);
}

void UBodyStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!MeshComp)
	{
		ResolveMesh();
	}
	if (bAutoLinkVitals && !Vitals)
	{
		RelinkTimer += DeltaTime;
		if (RelinkTimer > 2.f)
		{
			RelinkTimer = 0.f;
			ResolveVitals();
		}
	}
	if (bAutoSeasonFromCalendar)
	{
		SeasonTimer += DeltaTime;
		if (SeasonTimer > 5.f)
		{
			SeasonTimer = 0.f;
			if (UWorld* World = GetWorld())
			{
				if (UGameInstance* GI = World->GetGameInstance())
				{
					if (UStoryCalendarSubsystem* Cal = GI->GetSubsystem<UStoryCalendarSubsystem>())
					{
						SetSeason(SeasonFromMonth(Cal->GetDate().GetMonth()));
					}
				}
			}
		}
	}

	TargetWeightKg = ComputeTargetWeight();
	const float NewWeight = FMath::FInterpConstantTo(CurrentWeightKg, TargetWeightKg, DeltaTime, WeightRateKgPerSecond);
	if (!FMath::IsNearlyEqual(NewWeight, CurrentWeightKg, 0.001f))
	{
		CurrentWeightKg = NewWeight;
		bBodyDirty = true;
	}
	if (bBodyDirty)
	{
		ApplyBody();
	}

	if (bSwapPending && bSwapAssetsReady)
	{
		SwapWaitSeconds += DeltaTime;
		const bool bHidden = !bSwapMeshOnlyWhenHidden || !MeshComp || !MeshComp->WasRecentlyRendered(0.5f);
		if (bForceSwap || bHidden || SwapWaitSeconds > MaxSwapDeferSeconds)
		{
			CommitPendingSwap();
		}
	}
}

void UBodyStateComponent::ApplyBody()
{
	bBodyDirty = false;
	if (!MeshComp)
	{
		return;
	}

	if (UsesAnchors() && Anchors.Num() > 0)
	{
		// Find the two anchors around the current weight (Anchors is sorted heaviest first).
		const float W = CurrentWeightKg;
		int32 Lo = 0;
		int32 Hi = 0;
		float Alpha = 0.f;
		if (W >= Anchors[0].WeightKg)
		{
			Lo = Hi = 0;
		}
		else if (W <= Anchors.Last().WeightKg)
		{
			Lo = Hi = Anchors.Num() - 1;
		}
		else
		{
			for (int32 i = 0; i + 1 < Anchors.Num(); ++i)
			{
				if (W <= Anchors[i].WeightKg && W > Anchors[i + 1].WeightKg)
				{
					Lo = i;
					Hi = i + 1;
					const float Span = Anchors[Lo].WeightKg - Anchors[Hi].WeightKg;
					Alpha = Span > KINDA_SMALL_NUMBER ? (Anchors[Lo].WeightKg - W) / Span : 0.f;
					break;
				}
			}
		}
		const FCastBodyAnchor& A = Anchors[Lo];
		const FCastBodyAnchor& B = Anchors[Hi];

		for (const FName& Name : AllMorphNames)
		{
			MeshComp->SetMorphTarget(Name, FMath::Lerp(A.Morphs.FindRef(Name), B.Morphs.FindRef(Name), Alpha));
		}
		if (AllScalarNames.Num() > 0)
		{
			EnsureMaterialInstances();
			for (UMaterialInstanceDynamic* MID : MaterialInstances)
			{
				if (!MID)
				{
					continue;
				}
				for (const FName& Name : AllScalarNames)
				{
					MID->SetScalarParameterValue(Name, FMath::Lerp(A.MaterialScalars.FindRef(Name), B.MaterialScalars.FindRef(Name), Alpha));
				}
			}
		}
		HunchDegrees = FMath::Lerp(A.HunchDegrees, B.HunchDegrees, Alpha);
		GaitSpeedScale = FMath::Lerp(A.GaitSpeedScale, B.GaitSpeedScale, Alpha);
		Frailty = FMath::Lerp(A.Frailty, B.Frailty, Alpha);

		const FCastBodyAnchor& Nearest = Alpha < 0.5f ? A : B;
		if (!bStateInitialised || Nearest.State != CurrentState)
		{
			const bool bFirst = !bStateInitialised;
			CurrentState = Nearest.State;
			bStateInitialised = true;
			if (!bFirst)
			{
				OnBodyStateChanged.Broadcast(CurrentState, CurrentWeightKg);
			}
		}

		// Mesh / animation Blueprint of the nearest anchor, swapped when it is safe.
		if (!Nearest.Mesh.IsNull())
		{
			if (Nearest.Mesh == CurrentMeshSoft)
			{
				if (bSwapPending)
				{
					bSwapPending = false;
					bSwapAssetsReady = false;
					SwapHandle.Reset();
				}
			}
			else if (!(bSwapPending && Nearest.Mesh == PendingMeshSoft))
			{
				RequestAnchorSwap(Nearest);
			}
		}
	}
	else
	{
		HunchDegrees = 0.f;
		GaitSpeedScale = 1.f;
		Frailty = 0.f;
	}

	ApplyFallbackScale();
}

void UBodyStateComponent::EnsureMaterialInstances()
{
	if (!MeshComp)
	{
		return;
	}
	const int32 NumMats = MeshComp->GetNumMaterials();
	if (MaterialInstances.Num() == NumMats)
	{
		return;
	}
	MaterialInstances.Reset();
	for (int32 i = 0; i < NumMats; ++i)
	{
		MaterialInstances.Add(MeshComp->CreateAndSetMaterialInstanceDynamic(i));
	}
}

void UBodyStateComponent::ApplyFallbackScale()
{
	if (!MeshComp)
	{
		return;
	}
	const bool bUseScale = bAllowFallbackScaling && !bAnyMorphOnMesh && !bMeshFromAnchor;
	if (!bUseScale)
	{
		return;
	}
	const float HeightCm = bHasRow ? Row.HeightCm : 178.f;
	const float Girth = FMath::Clamp(FMath::Sqrt(FMath::Max(GetBmi(), 1.f) / FMath::Max(FallbackReferenceBmi, 1.f)), 0.7f, 1.35f);
	const float Z = HeightCm / FMath::Max(FallbackReferenceHeightCm, 1.f);
	MeshComp->SetRelativeScale3D(BaseMeshScale * FVector(Girth, Girth, Z));
}

void UBodyStateComponent::RequestAnchorSwap(const FCastBodyAnchor& Anchor)
{
	PendingMeshSoft = Anchor.Mesh;
	PendingAnimSoft = Anchor.AnimClass;
	PendingMesh = nullptr;
	PendingAnimClass = nullptr;
	bSwapPending = true;
	bSwapAssetsReady = false;
	SwapWaitSeconds = 0.f;

	TArray<FSoftObjectPath> Paths;
	if (!PendingMeshSoft.IsNull())
	{
		Paths.Add(PendingMeshSoft.ToSoftObjectPath());
	}
	if (!PendingAnimSoft.IsNull())
	{
		Paths.Add(PendingAnimSoft.ToSoftObjectPath());
	}
	if (Paths.Num() == 0)
	{
		bSwapPending = false;
		return;
	}
	SwapHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Paths, FStreamableDelegate::CreateUObject(this, &UBodyStateComponent::OnAnchorAssetsLoaded));
}

void UBodyStateComponent::OnAnchorAssetsLoaded()
{
	PendingMesh = PendingMeshSoft.Get();
	PendingAnimClass = PendingAnimSoft.IsNull() ? nullptr : PendingAnimSoft.Get();
	bSwapAssetsReady = true;
	if (bForceSwap)
	{
		CommitPendingSwap();
	}
}

void UBodyStateComponent::CommitPendingSwap()
{
	if (!bSwapPending || !bSwapAssetsReady)
	{
		return;
	}
	if (MeshComp && PendingMesh)
	{
		MeshComp->SetSkeletalMeshAsset(PendingMesh);
		MaterialInstances.Reset();
		CurrentMeshSoft = PendingMeshSoft;
		bMeshFromAnchor = true;
	}
	if (MeshComp && PendingAnimClass)
	{
		MeshComp->SetAnimInstanceClass(PendingAnimClass);
	}
	bSwapPending = false;
	bSwapAssetsReady = false;
	bForceSwap = false;
	SwapHandle.Reset();
	PendingMesh = nullptr;
	PendingAnimClass = nullptr;

	RefreshMorphSupport();
	RefreshOutfitLeaderPose();
	bBodyDirty = true;
	ApplyBody();
}

void UBodyStateComponent::SnapToTarget()
{
	TargetWeightKg = ComputeTargetWeight();
	CurrentWeightKg = TargetWeightKg;
	bForceSwap = true;
	ApplyBody();
	if (bSwapPending && bSwapAssetsReady)
	{
		CommitPendingSwap();
	}
	else if (!bSwapPending)
	{
		bForceSwap = false;
	}
}

void UBodyStateComponent::SetAct(ECastAct NewAct, bool bSnap)
{
	SetActAndSeason(NewAct, Season, bSnap);
}

void UBodyStateComponent::SetSeason(ECastSeason NewSeason)
{
	if (NewSeason != Season)
	{
		Season = NewSeason;
		RefreshOutfit();
	}
}

void UBodyStateComponent::SetActAndSeason(ECastAct NewAct, ECastSeason NewSeason, bool bSnap)
{
	const bool bChanged = (NewAct != Act) || (NewSeason != Season);
	Act = NewAct;
	Season = NewSeason;
	if (bSnap)
	{
		SnapToTarget();
	}
	else
	{
		bBodyDirty = true;
	}
	if (bChanged)
	{
		RefreshOutfit();
	}
}

void UBodyStateComponent::NotifyDiseaseStage(EDiseaseStage Stage)
{
	NotifiedStage = Stage;
	bStageNotified = true;
	bBodyDirty = true;
}

ECastSeason UBodyStateComponent::SeasonFromMonth(int32 Month)
{
	switch (Month)
	{
	case 7:
	case 8:
	case 9:
		return ECastSeason::Monsoon;
	case 11:
	case 12:
	case 1:
	case 2:
		return ECastSeason::Winter;
	default: // Mar-Jun and October
		return ECastSeason::Summer;
	}
}

void UBodyStateComponent::RefreshOutfit()
{
	if (!CastData || !MeshComp)
	{
		return;
	}
	FCastOutfitEntry Entry;
	if (!CastData->GetOutfit(CastId, Act, Season, Entry))
	{
		return; // nothing authored: leave whatever is on the mesh
	}
	if (Entry.OutfitTag == CurrentOutfitTag && OutfitComponents.Num() > 0)
	{
		return;
	}

	TArray<FSoftObjectPath> Paths;
	for (const TSoftObjectPtr<USkeletalMesh>& P : Entry.Pieces)
	{
		if (!P.IsNull())
		{
			Paths.Add(P.ToSoftObjectPath());
		}
	}
	const FName Tag = Entry.OutfitTag;
	const TArray<TSoftObjectPtr<USkeletalMesh>> Pieces = Entry.Pieces;
	if (Paths.Num() == 0)
	{
		OnOutfitAssetsLoaded(Tag, Pieces);
		return;
	}
	OutfitHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Paths, FStreamableDelegate::CreateWeakLambda(this, [this, Tag, Pieces]() { OnOutfitAssetsLoaded(Tag, Pieces); }));
}

void UBodyStateComponent::OnOutfitAssetsLoaded(FName Tag, TArray<TSoftObjectPtr<USkeletalMesh>> Pieces)
{
	AActor* Owner = GetOwner();
	if (!Owner || !MeshComp)
	{
		return;
	}
	for (int32 i = 0; i < Pieces.Num(); ++i)
	{
		USkeletalMeshComponent* Piece = OutfitComponents.IsValidIndex(i) ? OutfitComponents[i].Get() : nullptr;
		if (!Piece)
		{
			Piece = NewObject<USkeletalMeshComponent>(Owner, NAME_None, RF_Transient);
			Piece->SetupAttachment(MeshComp);
			Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Piece->RegisterComponent();
			OutfitComponents.Add(Piece);
		}
		USkeletalMesh* PieceMesh = Pieces[i].IsNull() ? nullptr : Pieces[i].Get();
		Piece->SetSkeletalMeshAsset(PieceMesh);
		Piece->SetVisibility(PieceMesh != nullptr);
	}
	for (int32 i = Pieces.Num(); i < OutfitComponents.Num(); ++i)
	{
		if (USkeletalMeshComponent* Extra = OutfitComponents[i])
		{
			Extra->SetSkeletalMeshAsset(nullptr);
			Extra->SetVisibility(false);
		}
	}
	RefreshOutfitLeaderPose();
	CurrentOutfitTag = Tag;
	OnOutfitChanged.Broadcast(Tag);
}

void UBodyStateComponent::RefreshOutfitLeaderPose()
{
	if (!MeshComp)
	{
		return;
	}
	for (USkeletalMeshComponent* Piece : OutfitComponents)
	{
		if (Piece)
		{
			Piece->SetLeaderPoseComponent(MeshComp);
		}
	}
}

void UBodyStateComponent::HandleCoughFit(float LockSeconds)
{
	OnBodyCough.Broadcast(LockSeconds);
	if (MeshComp && !CoughMontage.IsNull())
	{
		if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
		{
			if (UAnimMontage* Montage = CoughMontage.LoadSynchronous())
			{
				AnimInstance->Montage_Play(Montage);
			}
		}
	}
}
