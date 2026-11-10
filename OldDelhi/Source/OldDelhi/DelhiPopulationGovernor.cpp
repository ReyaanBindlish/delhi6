#include "DelhiPopulationGovernor.h"

#include "Engine/DataTable.h"

namespace
{
	// Time blocks from research/05 section 1.3, in hours since midnight (02:00 wraps to 26).
	struct FBlock { float Hours; const TCHAR* Name; };
	const FBlock GBlocks[] = { {5.f, TEXT("H05")}, {9.f, TEXT("H09")}, {13.f, TEXT("H13")},
	                           {18.f, TEXT("H18")}, {22.f, TEXT("H22")}, {26.f, TEXT("H02")} };
}

void UDelhiPopulationGovernor::BlockFor(float Hours, FName& A, FName& B, float& Alpha)
{
	float H = FMath::Fmod(Hours, 24.f);
	if (H < 5.f) { H += 24.f; } // 00:00-05:00 sits between H22 (22) and H02 (26) / H05 (29)
	const int32 N = UE_ARRAY_COUNT(GBlocks);
	for (int32 i = 0; i < N; ++i)
	{
		const FBlock& Cur = GBlocks[i];
		const FBlock& Nxt = GBlocks[(i + 1) % N];
		const float NxtH = (i + 1 < N) ? Nxt.Hours : Nxt.Hours + 24.f;
		if (H >= Cur.Hours && H < NxtH)
		{
			A = Cur.Name; B = Nxt.Name;
			Alpha = (H - Cur.Hours) / (NxtH - Cur.Hours);
			return;
		}
	}
	A = B = TEXT("H02"); Alpha = 0.f;
}

float UDelhiPopulationGovernor::DensityAtBlock(const FDelhiStreetDensityRow& Row, FName Block) const
{
	if (Block == TEXT("H05")) return Row.H05;
	if (Block == TEXT("H09")) return Row.H09;
	if (Block == TEXT("H13")) return Row.H13;
	if (Block == TEXT("H18")) return Row.H18;
	if (Block == TEXT("H22")) return Row.H22;
	return Row.H02;
}

float UDelhiPopulationGovernor::GetTargetDensity(FName Street, float Hours) const
{
	if (!StreetDensity) { return 0.f; }
	const FDelhiStreetDensityRow* Row = StreetDensity->FindRow<FDelhiStreetDensityRow>(Street, TEXT("Governor"));
	if (!Row) { return 0.f; }
	FName A, B; float Alpha;
	BlockFor(Hours, A, B, Alpha);
	const float Base = FMath::Lerp(DensityAtBlock(*Row, A), DensityAtBlock(*Row, B), FMath::SmoothStep(0.f, 1.f, Alpha));
	return Base * WeatherMultiplier * EventMultiplier * CrowdDensitySetting;
}

int32 UDelhiPopulationGovernor::GetTargetCount(FName Street, float Hours, float SegmentLengthM, bool bEventActive) const
{
	const float PerHundred = GetTargetDensity(Street, Hours);
	const int32 Cap = bEventActive ? EventVisibleCap : VisibleCap;
	return FMath::Clamp(FMath::RoundToInt(PerHundred * SegmentLengthM / 100.f), 0, Cap);
}

FName UDelhiPopulationGovernor::PickArchetype(FName Street, float Hours, int32 Seed) const
{
	if (!Schedule) { return NAME_None; }
	FName A, B; float Alpha;
	BlockFor(Hours, A, B, Alpha);
	const FName Block = Alpha < 0.5f ? A : B;   // nearest block's mix

	TArray<const FDelhiScheduleRow*> Rows;
	float Total = 0.f;
	Schedule->ForeachRow<FDelhiScheduleRow>(TEXT("Governor"), [&](const FName&, const FDelhiScheduleRow& R)
	{
		if (R.TimeBlock == Block && R.Street == Street && R.Weight > 0.f)
		{
			Rows.Add(&R);
			Total += R.Weight;
		}
	});
	if (Rows.Num() == 0 || Total <= 0.f) { return NAME_None; }

	FRandomStream Rng(Seed);
	float Pick = Rng.FRandRange(0.f, Total);
	for (const FDelhiScheduleRow* R : Rows)
	{
		Pick -= R->Weight;
		if (Pick <= 0.f) { return R->Archetype; }
	}
	return Rows.Last()->Archetype;
}
