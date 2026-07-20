#pragma once

#include "CoreMinimal.h"
#include "LostArk/Ability/LostArkGameplayAbility.h"
#include "LostArkJustGuardAbility.generated.h"

/**
 * 저스트가드 (Just Guard) 어빌리티
 * 
 * 보스 패턴 중 'State.Player.GuardReady' 태그가 있을 때만 발동 가능.
 * 마우스 커서 방향으로 회전 후 가드 몽타주를 재생하고, 보스에게 이벤트를 전송함.
 */
UCLASS()
class LOSTARK_API ULostArkJustGuardAbility : public ULostArkGameplayAbility
{
	GENERATED_BODY()

public:
	ULostArkJustGuardAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category = "JustGuard|Anim")
	class UAnimMontage* GuardStartMontage;

	UPROPERTY(EditDefaultsOnly, Category = "JustGuard|Anim")
	class UAnimMontage* GuardLoopMontage;

	UPROPERTY(EditDefaultsOnly, Category = "JustGuard|Anim")
	class UAnimMontage* GuardSuccessMontage;

	UPROPERTY(EditDefaultsOnly, Category = "JustGuard|Anim")
	class UAnimMontage* GuardFailMontage;

	UPROPERTY(EditDefaultsOnly, Category = "JustGuard|Config")
	float MaxGuardDuration = 1.5f;

private:
	void RotateToCursor(const FGameplayAbilityActorInfo* ActorInfo);
	void SendJustGuardEventToBoss(const FGameplayAbilityActorInfo* ActorInfo);
	AActor* FindBossActor(UWorld* World) const;

	void PlayLoopMontage();
	void EndGuardDueToTimeout();

	UFUNCTION()
	void OnStartMontageCompleted();

	UFUNCTION()
	void OnMontageCancelledOrInterrupted();

	UFUNCTION()
	void OnSuccessEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnFailEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnResultMontageCompleted();

	FTimerHandle GuardTimeoutTimerHandle;
};
