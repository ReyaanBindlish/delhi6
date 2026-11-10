#include "StoryCalendarSubsystem.h"

void UStoryCalendarSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UStoryCalendarSubsystem::AdvanceHours(float Hours)
{
	if (Hours <= 0.f) { return; }
	const FDateTime Before = Date;
	Date += FTimespan::FromHours(Hours);
	// Fire once per midnight crossed, oldest first.
	FDateTime Midnight = FDateTime(Before.GetYear(), Before.GetMonth(), Before.GetDay()) + FTimespan::FromDays(1);
	while (Midnight <= Date)
	{
		OnDayEnded.Broadcast(Midnight - FTimespan::FromSeconds(1));
		Midnight += FTimespan::FromDays(1);
	}
}

int32 UStoryCalendarSubsystem::DaysSince(FDateTime Reference) const
{
	return FMath::FloorToInt((Date - Reference).GetTotalDays());
}
