#include "Crowd/DhCrowdLaneNetwork.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogDhCrowdLanes, Log, All);

void FDhCrowdLaneNetwork::Reset()
{
	LocationId.Reset();
	Lanes.Reset();
	NodeLanes.Reset();
	Cells.Reset();
}

bool FDhCrowdLaneNetwork::LoadFromJsonFile(const FString& Path)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		UE_LOG(LogDhCrowdLanes, Warning, TEXT("Lane file not found: %s"), *Path);
		return false;
	}
	return LoadFromJsonString(Text);
}

bool FDhCrowdLaneNetwork::LoadFromJsonString(const FString& Json)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogDhCrowdLanes, Warning, TEXT("Lane JSON did not parse"));
		return false;
	}

	Reset();
	Root->TryGetStringField(TEXT("location"), LocationId);

	const TArray<TSharedPtr<FJsonValue>>* LaneArray = nullptr;
	if (!Root->TryGetArrayField(TEXT("lanes"), LaneArray) || !LaneArray)
	{
		UE_LOG(LogDhCrowdLanes, Warning, TEXT("Lane JSON has no 'lanes' array"));
		return false;
	}

	for (const TSharedPtr<FJsonValue>& LaneValue : *LaneArray)
	{
		const TSharedPtr<FJsonObject> LaneObj = LaneValue.IsValid() ? LaneValue->AsObject() : nullptr;
		if (!LaneObj.IsValid()) { continue; }

		FString Street = TEXT("MainRoad");
		LaneObj->TryGetStringField(TEXT("street"), Street);
		double Width = 400.0;
		LaneObj->TryGetNumberField(TEXT("width_cm"), Width);

		const TArray<TSharedPtr<FJsonValue>>* PointArray = nullptr;
		if (!LaneObj->TryGetArrayField(TEXT("points"), PointArray) || !PointArray) { continue; }

		TArray<FVector> Pts;
		Pts.Reserve(PointArray->Num());
		for (const TSharedPtr<FJsonValue>& PV : *PointArray)
		{
			const TArray<TSharedPtr<FJsonValue>>& C = PV->AsArray();
			if (C.Num() >= 2)
			{
				Pts.Add(FVector(C[0]->AsNumber(), C[1]->AsNumber(), C.Num() > 2 ? C[2]->AsNumber() : 0.0));
			}
		}
		AddLane(FName(*Street), static_cast<float>(Width), Pts);
	}

	Rebuild();
	UE_LOG(LogDhCrowdLanes, Log, TEXT("Lane network '%s': %d lanes, %d cells"), *LocationId, Lanes.Num(), Cells.Num());
	return Lanes.Num() > 0;
}

void FDhCrowdLaneNetwork::AddLane(FName Street, float WidthCm, const TArray<FVector>& Points)
{
	if (Points.Num() < 2) { return; }
	FDhCrowdLane Lane;
	Lane.Id = Lanes.Num();
	Lane.Street = Street;
	Lane.WidthCm = FMath::Clamp(WidthCm, 150.f, 2500.f);
	Lane.Points = Points;
	Lanes.Add(MoveTemp(Lane));
}

void FDhCrowdLaneNetwork::Rebuild()
{
	NodeLanes.Reset();
	Cells.Reset();

	// Drop degenerate lanes (under 1 m) and renumber.
	TArray<FDhCrowdLane> Clean;
	Clean.Reserve(Lanes.Num());
	for (FDhCrowdLane& L : Lanes)
	{
		L.Cum.SetNumUninitialized(L.Points.Num());
		float Total = 0.f;
		L.Cum[0] = 0.f;
		for (int32 i = 1; i < L.Points.Num(); ++i)
		{
			Total += static_cast<float>(FVector::Dist(L.Points[i - 1], L.Points[i]));
			L.Cum[i] = Total;
		}
		L.Length = Total;
		if (Total < 100.f) { continue; }
		L.Id = Clean.Num();
		L.NodeA = NodeKey(L.Points[0]);
		L.NodeB = NodeKey(L.Points.Last());
		Clean.Add(MoveTemp(L));
	}
	Lanes = MoveTemp(Clean);

	for (const FDhCrowdLane& L : Lanes)
	{
		NodeLanes.FindOrAdd(L.NodeA).AddUnique(L.Id);
		NodeLanes.FindOrAdd(L.NodeB).AddUnique(L.Id);

		for (float D = 0.f; D <= L.Length; D += SampleStepCm)
		{
			FVector P, T;
			SampleLane(L.Id, D, P, T);
			FDhLaneSample S;
			S.Lane = L.Id;
			S.Dist = D;
			Cells.FindOrAdd(CellOf(P)).Add(S);
		}
	}
}

void FDhCrowdLaneNetwork::SampleLane(int32 LaneIdx, float Dist, FVector& OutPos, FVector& OutTangent) const
{
	const FDhCrowdLane& L = Lanes[LaneIdx];
	const float D = FMath::Clamp(Dist, 0.f, L.Length);

	int32 Lo = 0;
	int32 Hi = L.Cum.Num() - 1;
	while (Hi - Lo > 1)
	{
		const int32 Mid = (Lo + Hi) / 2;
		if (L.Cum[Mid] <= D) { Lo = Mid; } else { Hi = Mid; }
	}
	const float SegLen = FMath::Max(L.Cum[Hi] - L.Cum[Lo], KINDA_SMALL_NUMBER);
	const float A = FMath::Clamp((D - L.Cum[Lo]) / SegLen, 0.f, 1.f);
	OutPos = FMath::Lerp(L.Points[Lo], L.Points[Hi], A);
	OutTangent = (L.Points[Hi] - L.Points[Lo]).GetSafeNormal();
	if (OutTangent.IsNearlyZero()) { OutTangent = FVector::ForwardVector; }
}

void FDhCrowdLaneNetwork::GetPose(const FDhCrowdLaneWalker& W, FVector& OutPos, FVector& OutDir) const
{
	if (!Lanes.IsValidIndex(W.Lane))
	{
		OutPos = FVector::ZeroVector;
		OutDir = FVector::ForwardVector;
		return;
	}
	FVector P, T;
	SampleLane(W.Lane, W.Dist, P, T);
	OutDir = (W.Dir >= 0) ? T : -T;
	OutDir.Z = 0.f;
	OutDir = OutDir.GetSafeNormal();
	// UE is left-handed (X forward, Y right): right of travel = (-dy, dx).
	const FVector Right(-OutDir.Y, OutDir.X, 0.f);
	OutPos = P + Right * W.Lateral;
}

bool FDhCrowdLaneNetwork::Advance(FDhCrowdLaneWalker& W, float DeltaCm, FRandomStream& Rng) const
{
	if (!Lanes.IsValidIndex(W.Lane)) { return false; }

	bool bChanged = false;
	float D = W.Dist + static_cast<float>(W.Dir) * DeltaCm;

	for (int32 Guard = 0; Guard < 8; ++Guard)
	{
		const FDhCrowdLane& L = Lanes[W.Lane];
		if (D >= 0.f && D <= L.Length)
		{
			W.Dist = D;
			return bChanged;
		}

		const bool bAtB = D > L.Length;
		const float Over = bAtB ? (D - L.Length) : -D;
		const FIntPoint Node = bAtB ? L.NodeB : L.NodeA;

		TArray<int32, TInlineAllocator<8>> Options;
		if (const TArray<int32>* AtNode = NodeLanes.Find(Node))
		{
			for (int32 Other : *AtNode)
			{
				if (Other != W.Lane) { Options.Add(Other); }
			}
		}

		if (Options.Num() == 0)
		{
			// Dead end: turn around.
			W.Dir = -W.Dir;
			W.Dist = FMath::Clamp(bAtB ? (L.Length - Over) : Over, 0.f, L.Length);
			return true;
		}

		const int32 Next = Options[Rng.RandRange(0, Options.Num() - 1)];
		const FDhCrowdLane& NL = Lanes[Next];
		if (NL.NodeA == Node)
		{
			W.Dir = 1;
			D = Over;
		}
		else
		{
			W.Dir = -1;
			D = NL.Length - Over;
		}
		W.Lane = Next;
		const float Half = FMath::Max(0.f, 0.5f * NL.WidthCm - 35.f);
		W.HomeLateral = FMath::Clamp(W.HomeLateral, -Half, Half);
		W.Lateral = FMath::Clamp(W.Lateral, -Half, Half);
		bChanged = true;
	}

	W.Dist = FMath::Clamp(D, 0.f, Lanes[W.Lane].Length);
	return bChanged;
}

void FDhCrowdLaneNetwork::Place(FDhCrowdLaneWalker& W, int32 LaneIdx, float Dist, int32 Dir, float HomeFraction) const
{
	W.Lane = LaneIdx;
	W.Dir = (Dir >= 0) ? 1 : -1;
	W.Dist = FMath::Clamp(Dist, 0.f, Lanes[LaneIdx].Length);
	const float Half = FMath::Max(0.f, 0.5f * Lanes[LaneIdx].WidthCm - 35.f);
	// Negative = left of the direction of travel, so opposing flows pass on opposite sides.
	W.HomeLateral = -FMath::Clamp(HomeFraction, 0.f, 1.f) * Half;
	W.Lateral = W.HomeLateral;
}

// ----------------------------------------------------------------------------------------------------------
// FDhCrowdSim
// ----------------------------------------------------------------------------------------------------------

EDhCrowdActivity FDhCrowdSim::Step(const FDhCrowdLaneNetwork& Net, FDhCrowdLaneWalker& W, FDhCrowdAgentMotion& M,
	const FDhCrowdPlayerInfo& Player, float Dt, FVector& OutPos, FVector& OutDir)
{
	if (!Net.IsValidLane(W.Lane))
	{
		OutPos = FVector::ZeroVector;
		OutDir = FVector::ForwardVector;
		return EDhCrowdActivity::Idle;
	}

	Net.GetPose(W, OutPos, OutDir);
	const FDhCrowdLane& Lane = Net.GetLane(W.Lane);
	const bool bFleeing = M.FleeLeft > 0.f;
	if (bFleeing) { M.FleeLeft = FMath::Max(0.f, M.FleeLeft - Dt); }

	// --- the player: sidestep inside the lane and yield when head-on (research/05 "simple avoidance of the player")
	float SpeedMul = 1.f;
	bool bNearPlayer = false;
	if (Player.bValid)
	{
		const FVector ToPlayer(Player.Pos.X - OutPos.X, Player.Pos.Y - OutPos.Y, 0.f);
		const float D = static_cast<float>(ToPlayer.Size());
		if (D < 260.f)
		{
			bNearPlayer = true;
			const FVector Right(-OutDir.Y, OutDir.X, 0.f);
			const float Side = static_cast<float>(FVector::DotProduct(Right, -ToPlayer));
			const float Half = FMath::Max(0.f, 0.5f * Lane.WidthCm - 35.f);
			const float Target = FMath::Clamp(W.Lateral + (Side >= 0.f ? 120.f : -120.f), -Half, Half);
			W.Lateral = FMath::FInterpConstantTo(W.Lateral, Target, Dt, 160.f);
			const float Head = D > 1.f ? static_cast<float>(FVector::DotProduct(OutDir, ToPlayer / D)) : 0.f;
			if (!bFleeing && D < 140.f && Head > 0.2f) { SpeedMul = 0.3f; }
		}
	}
	if (!bNearPlayer)
	{
		W.Lateral = FMath::FInterpConstantTo(W.Lateral, W.HomeLateral, Dt, 60.f);
	}

	if (M.DwellLeft > 0.f && !bFleeing)
	{
		M.DwellLeft = FMath::Max(0.f, M.DwellLeft - Dt);
		return M.bStationary ? EDhCrowdActivity::Work : EDhCrowdActivity::Idle;
	}
	if (M.bStationary && !bFleeing)
	{
		return EDhCrowdActivity::Work;
	}

	FRandomStream Rng(W.RngState != 0 ? W.RngState : 1);
	const float Speed = M.BaseSpeed * SpeedMul * (bFleeing ? 2.4f : 1.f);
	const bool bChanged = Net.Advance(W, Speed * Dt, Rng);
	if (bChanged && !bFleeing && Rng.FRand() < 0.10f)
	{
		M.DwellLeft = Rng.FRandRange(4.f, 18.f); // stops to chat, look at a stall, wait
	}
	W.RngState = Rng.GetCurrentSeed();

	Net.GetPose(W, OutPos, OutDir);
	return bFleeing ? EDhCrowdActivity::Flee : EDhCrowdActivity::Walk;
}

void FDhCrowdSim::Panic(const FDhCrowdLaneNetwork& Net, FDhCrowdLaneWalker& W, FDhCrowdAgentMotion& M,
	const FVector& From, float Seconds)
{
	if (!Net.IsValidLane(W.Lane)) { return; }
	FVector P, T;
	Net.SampleLane(W.Lane, W.Dist, P, T);
	const FVector Away = FVector(P.X - From.X, P.Y - From.Y, 0.f).GetSafeNormal();
	// Run along the lane in whichever direction leads away from the source.
	W.Dir = (FVector::DotProduct(T, Away) >= 0.0) ? 1 : -1;
	M.FleeLeft = Seconds;
	M.DwellLeft = 0.f;
	M.bStationary = false; // even a shopkeeper runs when the shooting starts
}
