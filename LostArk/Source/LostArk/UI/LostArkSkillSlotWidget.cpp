#include "LostArk/UI/LostArkSkillSlotWidget.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"

void ULostArkSkillSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		// 쿨다운 태그 바인딩
		if (CooldownTag.IsValid())
		{
			CooldownTagDelegateHandle = ASC->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ULostArkSkillSlotWidget::OnCooldownTagChanged);
		}

		// 콤보 이벤트 바인딩
		if (ComboEventTag.IsValid())
		{
			ComboEventDelegateHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(ComboEventTag).AddUObject(this, &ULostArkSkillSlotWidget::OnComboEventReceived);
		}
	}
}

void ULostArkSkillSlotWidget::NativeDestruct()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		if (CooldownTag.IsValid() && CooldownTagDelegateHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::NewOrRemoved).Remove(CooldownTagDelegateHandle);
		}

		if (ComboEventTag.IsValid() && ComboEventDelegateHandle.IsValid())
		{
			ASC->GenericGameplayEventCallbacks.FindOrAdd(ComboEventTag).Remove(ComboEventDelegateHandle);
		}
	}

	Super::NativeDestruct();
}

void ULostArkSkillSlotWidget::OnCooldownTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
		if (ASC)
		{
			FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(CooldownTag));
			TArray<float> Durations = ASC->GetActiveEffectsTimeRemaining(Query);
			
			if (Durations.Num() > 0)
			{
				float TimeRemaining = Durations[0];
				
				// 전체 Duration을 찾기 위해 Effect 정보 조회
				float TotalDuration = TimeRemaining; // 기본값
				TArray<float> AllDurations = ASC->GetActiveEffectsDuration(Query);
				if(AllDurations.Num() > 0)
				{
					TotalDuration = AllDurations[0];
				}

				OnCooldownStarted(TimeRemaining, TotalDuration);
			}
			else
			{
				// Effect 쿼리가 실패했어도 임의의 쿨다운 연출 시작 (대비용)
				OnCooldownStarted(1.0f, 1.0f);
			}
		}
	}
	else
	{
		OnCooldownEnded();
	}
}

void ULostArkSkillSlotWidget::OnComboEventReceived(const FGameplayEventData* Payload)
{
	if (Payload)
	{
		// Payload의 EventMagnitude를 인덱스로 사용 (LostArkComboSkillAbility에서 넘겨줄 예정)
		int32 NextIconIndex = FMath::RoundToInt(Payload->EventMagnitude);
		
		// 블루프린트로 변경 이벤트를 전달
		OnComboIconChanged(NextIconIndex);
	}
}
