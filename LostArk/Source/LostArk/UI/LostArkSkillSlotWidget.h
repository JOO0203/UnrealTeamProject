#pragma once

#include "CoreMinimal.h"
#include "LostArk/UI/LostArkUserWidget.h"
#include "LostArk/Core/LostArkAbilityTypes.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "LostArkSkillSlotWidget.generated.h"

/**
 * 스킬 퀵슬롯 위젯 (쿨다운 및 콤보 스킬 상태 연동)
 */
UCLASS()
class LOSTARK_API ULostArkSkillSlotWidget : public ULostArkUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Slot")
	ELostArkAbilityInputID InputID;

	// 해당 슬롯이 리스닝할 쿨다운 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Slot")
	FGameplayTag CooldownTag;

	// 콤보 진행 시 수신할 이벤트 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Slot")
	FGameplayTag ComboEventTag;

protected:
	// 블루프린트에 쿨다운 애니메이션 트리거 (진행도 머티리얼 또는 프로그래스바)
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Skill Slot")
	void OnCooldownStarted(float TimeRemaining, float Duration);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Skill Slot")
	void OnCooldownEnded();

	// 콤보 단계가 넘어갈 때 아이콘 변경을 위한 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Skill Slot")
	void OnComboIconChanged(int32 NextIconIndex);

private:
	virtual void OnCooldownTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	virtual void OnComboEventReceived(const FGameplayEventData* Payload);

	FDelegateHandle CooldownTagDelegateHandle;
	FDelegateHandle ComboEventDelegateHandle;
};
