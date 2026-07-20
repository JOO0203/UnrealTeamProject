#include "LostArk/Ability/LostArkJustGuardAbility.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "LostArk/Core/LostArkPlayerController.h"
#include "EngineUtils.h"
#include "TimerManager.h"

ULostArkJustGuardAbility::ULostArkJustGuardAbility()
{
	// 보스가 ?�어주는 �??�그 (???�그가 ?�어?�만 발동 가??
	FGameplayTag GuardReadyTag = FGameplayTag::RequestGameplayTag(FName("State.Player.GuardReady"));
	ActivationRequiredTags.AddTag(GuardReadyTag);

	// 가??모션 �??��????�그
	FGameplayTag GuardingTag = FGameplayTag::RequestGameplayTag(FName("State.Player.Guarding"));
	ActivationOwnedTags.AddTag(GuardingTag);
}

void ULostArkJustGuardAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 마우??방향?�로 캐릭???�전
	RotateToCursor(ActorInfo);

	// 2. 보스?�게 ?�벤???�송
	SendJustGuardEventToBoss(ActorInfo);

	// 3. ?�정 ?�벤??리슨 ?�작 (병렬)
	UAbilityTask_WaitGameplayEvent* WaitSuccessEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag("Event.Player.JustGuard.Success"));
	WaitSuccessEvent->EventReceived.AddDynamic(this, &ULostArkJustGuardAbility::OnSuccessEventReceived);
	WaitSuccessEvent->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitFailEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag("Event.Player.JustGuard.Fail"));
	WaitFailEvent->EventReceived.AddDynamic(this, &ULostArkJustGuardAbility::OnFailEventReceived);
	WaitFailEvent->ReadyForActivation();

	// 4. ?�작 몽�?�??�생
	if (GuardStartMontage)
	{
		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, GuardStartMontage, 1.0f);
		Task->OnBlendOut.AddDynamic(this, &ULostArkJustGuardAbility::OnStartMontageCompleted);
		Task->OnCompleted.AddDynamic(this, &ULostArkJustGuardAbility::OnStartMontageCompleted);
		Task->OnInterrupted.AddDynamic(this, &ULostArkJustGuardAbility::OnMontageCancelledOrInterrupted);
		Task->OnCancelled.AddDynamic(this, &ULostArkJustGuardAbility::OnMontageCancelledOrInterrupted);
		Task->ReadyForActivation();
	}
	else
	{
		PlayLoopMontage();
	}

	// 5. 무한루프 방�???최�? ?��??�간 ?�?�머 ?�정 (?�판 �?무판???�외처리)
	GetWorld()->GetTimerManager().SetTimer(GuardTimeoutTimerHandle, this, &ULostArkJustGuardAbility::EndGuardDueToTimeout, MaxGuardDuration, false);
}

void ULostArkJustGuardAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (GuardTimeoutTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(GuardTimeoutTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULostArkJustGuardAbility::RotateToCursor(const FGameplayAbilityActorInfo* ActorInfo)
{
	ALostArkPlayerController* PC = Cast<ALostArkPlayerController>(ActorInfo->PlayerController.Get());
	ACharacter* AvatarCharacter = Cast<ACharacter>(ActorInfo->AvatarActor.Get());

	if (PC && AvatarCharacter)
	{
		FHitResult HitResult;
		if (PC->GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
		{
			FVector TargetLocation = HitResult.Location;
			FVector CurrentLocation = AvatarCharacter->GetActorLocation();
			TargetLocation.Z = CurrentLocation.Z;

			FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(CurrentLocation, TargetLocation);
			AvatarCharacter->SetActorRotation(TargetRotation);
		}
	}
}

void ULostArkJustGuardAbility::SendJustGuardEventToBoss(const FGameplayAbilityActorInfo* ActorInfo)
{
	UWorld* World = GetWorld();
	if (!World) return;

	AActor* BossActor = FindBossActor(World);
	if (BossActor)
	{
		FGameplayEventData Payload;
		Payload.Instigator = ActorInfo->AvatarActor.Get();
		
		FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Boss.JustGuardInput"));
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(BossActor, EventTag, Payload);
	}
}

AActor* ULostArkJustGuardAbility::FindBossActor(UWorld* World) const
{
	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		ACharacter* Char = *It;
		if (Char == GetAvatarActorFromActorInfo()) continue;

		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Char))
		{
			UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
			if (ASC && ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Boss.JustGuardable"))))
			{
				return Char;
			}
		}
	}

	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		ACharacter* Char = *It;
		if (Char != GetAvatarActorFromActorInfo() && !Char->IsPlayerControlled())
		{
			return Char;
		}
	}

	return nullptr;
}

void ULostArkJustGuardAbility::OnStartMontageCompleted()
{
	PlayLoopMontage();
}

void ULostArkJustGuardAbility::PlayLoopMontage()
{
	if (GuardLoopMontage)
	{
		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, GuardLoopMontage, 1.0f);
		Task->OnInterrupted.AddDynamic(this, &ULostArkJustGuardAbility::OnMontageCancelledOrInterrupted);
		Task->OnCancelled.AddDynamic(this, &ULostArkJustGuardAbility::OnMontageCancelledOrInterrupted);
		// 루프 몽�?주이므�??�연?�럽�??�나??경우(OnCompleted)??Timeout???�해 캔슬?�는 것과 ?�일?�게 처리
		Task->OnCompleted.AddDynamic(this, &ULostArkJustGuardAbility::OnMontageCancelledOrInterrupted);
		Task->ReadyForActivation();
	}
}

void ULostArkJustGuardAbility::OnSuccessEventReceived(FGameplayEventData Payload)
{
	if (GuardTimeoutTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(GuardTimeoutTimerHandle);
	}

	if (GuardSuccessMontage)
	{
		// ?�재 몽�?주�? ?�고 ?�공 몽�?�??�생
		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, GuardSuccessMontage, 1.0f);
		Task->OnBlendOut.AddDynamic(this, &ULostArkJustGuardAbility::OnResultMontageCompleted);
		Task->OnCompleted.AddDynamic(this, &ULostArkJustGuardAbility::OnResultMontageCompleted);
		Task->OnInterrupted.AddDynamic(this, &ULostArkJustGuardAbility::OnResultMontageCompleted);
		Task->OnCancelled.AddDynamic(this, &ULostArkJustGuardAbility::OnResultMontageCompleted);
		Task->ReadyForActivation();
	}
	else
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void ULostArkJustGuardAbility::OnFailEventReceived(FGameplayEventData Payload)
{
	if (GuardTimeoutTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(GuardTimeoutTimerHandle);
	}

	if (GuardFailMontage)
	{
		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, GuardFailMontage, 1.0f);
		Task->OnBlendOut.AddDynamic(this, &ULostArkJustGuardAbility::OnResultMontageCompleted);
		Task->OnCompleted.AddDynamic(this, &ULostArkJustGuardAbility::OnResultMontageCompleted);
		Task->OnInterrupted.AddDynamic(this, &ULostArkJustGuardAbility::OnResultMontageCompleted);
		Task->OnCancelled.AddDynamic(this, &ULostArkJustGuardAbility::OnResultMontageCompleted);
		Task->ReadyForActivation();
	}
	else
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void ULostArkJustGuardAbility::OnResultMontageCompleted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void ULostArkJustGuardAbility::EndGuardDueToTimeout()
{
	// ?�?�아??발생 ???�력 종료 (가?��? ?�연?�럽�??��?
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void ULostArkJustGuardAbility::OnMontageCancelledOrInterrupted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}
