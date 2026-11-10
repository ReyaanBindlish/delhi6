#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StoryCalendarSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoryDayEnded, FDateTime, Date);

/**
 * The story's real calendar (bible: "A real calendar runs across the game"). The disease, the pension,
 * the tide and Rangers drives all read it. Saves happen only at the end of each in-game day.
 */
UCLASS()
class OLDDELHI_API UStoryCalendarSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category = "Story|Calendar") FDateTime GetDate() const { return Date; }
	UFUNCTION(BlueprintCallable, Category = "Story|Calendar") void SetDate(FDateTime NewDate) { Date = NewDate; }

	/** Advance the clock; fires OnDayEnded (the only save checkpoint) for every midnight crossed. */
	UFUNCTION(BlueprintCallable, Category = "Story|Calendar") void AdvanceHours(float Hours);

	/** Whole days since a reference date (e.g. arrival in Karachi), for month/week-based curves. */
	UFUNCTION(BlueprintPure, Category = "Story|Calendar") int32 DaysSince(FDateTime Reference) const;

	UPROPERTY(BlueprintAssignable, Category = "Story|Calendar") FOnStoryDayEnded OnDayEnded;

private:
	/** Act 1 opens on the collection run, Ghaziabad, 2019. */
	FDateTime Date = FDateTime(2019, 10, 14, 9, 0, 0);
};
