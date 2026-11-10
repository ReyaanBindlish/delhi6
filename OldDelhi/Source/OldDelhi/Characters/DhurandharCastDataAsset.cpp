#include "DhurandharCastDataAsset.h"

#include "Engine/DataTable.h"

DEFINE_LOG_CATEGORY_STATIC(LogCastData, Log, All);

UDhurandharCastDataAsset::UDhurandharCastDataAsset()
{
	CastTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Characters/Cast/DT_Cast.DT_Cast")));
	DayaAnchors = MakeDefaultDayaAnchors();
}

TArray<FCastBodyAnchor> UDhurandharCastDataAsset::MakeDefaultDayaAnchors()
{
	TArray<FCastBodyAnchor> Out;

	// Act 1: healthy 2019, 78 kg at 178 cm (BMI 24.6), wiry but fed.
	{
		FCastBodyAnchor A;
		A.State = ECastBodyState::Healthy2019;
		A.WeightKg = 78.f;
		A.HunchDegrees = 0.f;
		A.GaitSpeedScale = 1.f;
		A.Frailty = 0.f;
		Out.Add(A);
	}
	// Act 2: minus 14 kg (64 kg, BMI 20.2). Gaunt, tired, filthy from the platform.
	{
		FCastBodyAnchor A;
		A.State = ECastBodyState::Beggar;
		A.WeightKg = 64.f;
		A.Morphs.Add(TEXT("DayaBody_Lean14"), 1.f);
		A.Morphs.Add(TEXT("DayaFace_Hollow"), 0.45f);
		A.MaterialScalars.Add(TEXT("Gauntness"), 0.5f);
		A.MaterialScalars.Add(TEXT("Dirt"), 0.55f);
		A.MaterialScalars.Add(TEXT("Pallor"), 0.15f);
		A.HunchDegrees = 4.f;
		A.GaitSpeedScale = 0.92f;
		A.Frailty = 0.3f;
		Out.Add(A);
	}
	// Acts 5-6: minus 29 kg (49 kg, BMI 15.5). Untreated then diagnosed tuberculosis.
	{
		FCastBodyAnchor A;
		A.State = ECastBodyState::Diseased;
		A.WeightKg = 49.f;
		A.Morphs.Add(TEXT("DayaBody_Lean14"), 1.f);
		A.Morphs.Add(TEXT("DayaBody_Lean29"), 1.f);
		A.Morphs.Add(TEXT("DayaFace_Hollow"), 1.f);
		A.MaterialScalars.Add(TEXT("Gauntness"), 1.f);
		A.MaterialScalars.Add(TEXT("Dirt"), 0.2f);
		A.MaterialScalars.Add(TEXT("Pallor"), 0.7f);
		A.MaterialScalars.Add(TEXT("Sweat"), 0.5f);
		A.HunchDegrees = 9.f;
		A.GaitSpeedScale = 0.78f;
		A.Frailty = 1.f;
		Out.Add(A);
	}
	return Out;
}

const FCastMemberRow* UDhurandharCastDataAsset::FindCastRow(FName CastId) const
{
	UDataTable* Table = CastTable.LoadSynchronous();
	if (!Table)
	{
		return nullptr;
	}
	return Table->FindRow<FCastMemberRow>(CastId, TEXT("UDhurandharCastDataAsset::FindCastRow"), /*bWarnIfRowMissing*/ false);
}

bool UDhurandharCastDataAsset::GetCastRow(FName CastId, FCastMemberRow& OutRow) const
{
	if (const FCastMemberRow* Row = FindCastRow(CastId))
	{
		OutRow = *Row;
		return true;
	}
	return false;
}

TArray<FName> UDhurandharCastDataAsset::GetAllCastIds() const
{
	UDataTable* Table = CastTable.LoadSynchronous();
	return Table ? Table->GetRowNames() : TArray<FName>();
}

bool UDhurandharCastDataAsset::GetOutfit(FName CastId, ECastAct Act, ECastSeason Season, FCastOutfitEntry& OutEntry) const
{
	const FCastOutfitEntry* SameActAnySeason = nullptr;
	for (const FCastOutfitEntry& E : Outfits)
	{
		if (E.CastId != CastId || E.Act != Act)
		{
			continue;
		}
		if (E.Season == Season)
		{
			OutEntry = E;
			return true;
		}
		if (!SameActAnySeason)
		{
			SameActAnySeason = &E;
		}
	}
	if (SameActAnySeason)
	{
		OutEntry = *SameActAnySeason;
		return true;
	}
	return false;
}

bool UDhurandharCastDataAsset::ValidateCastInvariants(TArray<FString>& OutErrors) const
{
	OutErrors.Reset();

	const FCastMemberRow* Daya = FindCastRow(TEXT("Daya"));
	const FCastMemberRow* Bablu = FindCastRow(TEXT("Bablu"));
	const FCastMemberRow* Gurnoor = FindCastRow(TEXT("Gurnoor"));
	const FCastMemberRow* Bibi = FindCastRow(TEXT("Bibi"));
	if (!Daya || !Bablu || !Gurnoor || !Bibi)
	{
		OutErrors.Add(TEXT("DT_Cast is missing Daya, Bablu, Gurnoor or Bibi (is the table imported at CastTable?)."));
		return false;
	}

	if (!FMath::IsNearlyEqual(Bablu->WeightKg - Daya->WeightKg, 20.f, 0.5f))
	{
		OutErrors.Add(FString::Printf(TEXT("Bablu (%.1f kg) must be 20 kg heavier than Daya (%.1f kg)."), Bablu->WeightKg, Daya->WeightKg));
	}
	if (Gurnoor->AgeAtStart != 9)
	{
		OutErrors.Add(FString::Printf(TEXT("Gurnoor must be 9 (is %d)."), Gurnoor->AgeAtStart));
	}
	if (Bibi->AgeAtStart != 84)
	{
		OutErrors.Add(FString::Printf(TEXT("Bibi must be 84 (is %d)."), Bibi->AgeAtStart));
	}

	if (DayaAnchors.Num() < 3)
	{
		OutErrors.Add(TEXT("Daya needs 3 body anchors (healthy, beggar, diseased)."));
	}
	else
	{
		float Heaviest = 0.f;
		float Lightest = TNumericLimits<float>::Max();
		for (const FCastBodyAnchor& A : DayaAnchors)
		{
			Heaviest = FMath::Max(Heaviest, A.WeightKg);
			Lightest = FMath::Min(Lightest, A.WeightKg);
		}
		if (!FMath::IsNearlyEqual(Heaviest, Daya->WeightKg, 0.5f))
		{
			OutErrors.Add(TEXT("Daya's heaviest anchor must equal his DT_Cast weight."));
		}
		for (const FCastBodyAnchor& A : DayaAnchors)
		{
			if (A.State == ECastBodyState::Beggar && !FMath::IsNearlyEqual(Daya->WeightKg - A.WeightKg, 14.f, 0.5f))
			{
				OutErrors.Add(TEXT("Beggar anchor must be 14 kg below Daya's Act 1 weight."));
			}
		}
		if (!FMath::IsNearlyEqual(Daya->WeightKg - Lightest, 29.f, 0.5f))
		{
			OutErrors.Add(TEXT("Diseased anchor must be 29 kg below Daya's Act 1 weight."));
		}
	}

	for (const FString& E : OutErrors)
	{
		UE_LOG(LogCastData, Warning, TEXT("Cast invariant: %s"), *E);
	}
	return OutErrors.Num() == 0;
}
