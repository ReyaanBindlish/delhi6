#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MoneyLedgerComponent.generated.h"

UENUM(BlueprintType)
enum class ECurrency : uint8
{
	INR,
	PKR
};

USTRUCT(BlueprintType)
struct FLedgerEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FDateTime Date;
	UPROPERTY(BlueprintReadOnly) ECurrency Currency = ECurrency::INR;
	/** Positive = received, negative = spent. Whole rupees: the bible tracks every rupee. */
	UPROPERTY(BlueprintReadOnly) int64 Amount = 0;
	UPROPERTY(BlueprintReadOnly) FString Reason;
	UPROPERTY(BlueprintReadOnly) int64 BalanceAfter = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBalanceChanged, ECurrency, Currency, int64, NewBalance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOutOfMoney, ECurrency, Currency);

/**
 * Daya's cash. No score, no abstraction: the HUD shows cash on hand (bible, Act 1 tutorial).
 * Running out of money is the most common fail state in Acts 2 and 6.
 */
UCLASS(ClassGroup = (Story), meta = (BlueprintSpawnableComponent))
class OLDDELHI_API UMoneyLedgerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Story|Money") int64 GetBalance(ECurrency Currency) const;

	/** Receive money (begging, wages, the pension). */
	UFUNCTION(BlueprintCallable, Category = "Story|Money") void Receive(ECurrency Currency, int64 Amount, const FString& Reason);

	/** Spend if affordable; returns false and changes nothing if not. */
	UFUNCTION(BlueprintCallable, Category = "Story|Money") bool TrySpend(ECurrency Currency, int64 Amount, const FString& Reason);

	/** Lose money involuntarily (theft, police, seizure); may go to zero, never negative. */
	UFUNCTION(BlueprintCallable, Category = "Story|Money") int64 Lose(ECurrency Currency, int64 Amount, const FString& Reason);

	UFUNCTION(BlueprintPure, Category = "Story|Money") const TArray<FLedgerEntry>& GetHistory() const { return History; }

	UPROPERTY(BlueprintAssignable, Category = "Story|Money") FOnBalanceChanged OnBalanceChanged;
	UPROPERTY(BlueprintAssignable, Category = "Story|Money") FOnOutOfMoney OnOutOfMoney;

	/** Reference prices from the bible, editable per act (chai 10 INR, katta 8,000 INR, clerk 2,000 INR, boat 11,000 PKR). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story|Money") TMap<FName, int64> ReferencePrices;

	UMoneyLedgerComponent();

private:
	void Record(ECurrency Currency, int64 Delta, const FString& Reason);

	UPROPERTY(SaveGame) int64 BalanceINR = 0;
	UPROPERTY(SaveGame) int64 BalancePKR = 0;
	UPROPERTY(SaveGame) TArray<FLedgerEntry> History;
};
