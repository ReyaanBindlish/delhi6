#include "MoneyLedgerComponent.h"

#include "StoryCalendarSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UMoneyLedgerComponent::UMoneyLedgerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	ReferencePrices.Add(TEXT("Chai"), 10);
	ReferencePrices.Add(TEXT("Meal"), 50);
	ReferencePrices.Add(TEXT("ShelterNight"), 10);
	ReferencePrices.Add(TEXT("Katta"), 8000);
	ReferencePrices.Add(TEXT("Clerk"), 2000);
	ReferencePrices.Add(TEXT("BoatPKR"), 11000);
}

int64 UMoneyLedgerComponent::GetBalance(ECurrency Currency) const
{
	return Currency == ECurrency::INR ? BalanceINR : BalancePKR;
}

void UMoneyLedgerComponent::Receive(ECurrency Currency, int64 Amount, const FString& Reason)
{
	if (Amount > 0) { Record(Currency, Amount, Reason); }
}

bool UMoneyLedgerComponent::TrySpend(ECurrency Currency, int64 Amount, const FString& Reason)
{
	if (Amount <= 0) { return true; }
	if (GetBalance(Currency) < Amount) { return false; }
	Record(Currency, -Amount, Reason);
	return true;
}

int64 UMoneyLedgerComponent::Lose(ECurrency Currency, int64 Amount, const FString& Reason)
{
	const int64 Taken = FMath::Clamp<int64>(Amount, 0, GetBalance(Currency));
	if (Taken > 0) { Record(Currency, -Taken, Reason); }
	return Taken;
}

void UMoneyLedgerComponent::Record(ECurrency Currency, int64 Delta, const FString& Reason)
{
	int64& Bal = Currency == ECurrency::INR ? BalanceINR : BalancePKR;
	Bal += Delta;

	FLedgerEntry E;
	E.Currency = Currency;
	E.Amount = Delta;
	E.Reason = Reason;
	E.BalanceAfter = Bal;
	if (const UWorld* W = GetWorld())
	{
		if (const UGameInstance* GI = W->GetGameInstance())
		{
			if (const UStoryCalendarSubsystem* Cal = GI->GetSubsystem<UStoryCalendarSubsystem>())
			{
				E.Date = Cal->GetDate();
			}
		}
	}
	History.Add(E);

	OnBalanceChanged.Broadcast(Currency, Bal);
	if (Bal == 0 && Delta < 0) { OnOutOfMoney.Broadcast(Currency); }
}
