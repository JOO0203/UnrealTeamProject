#pragma once

#include "CoreMinimal.h"
#include "LostArk/UI/LostArkUserWidget.h"
#include "GameplayEffectTypes.h"
#include "LostArkHUDWidget.generated.h"

/**
 * 플레이어의 메인 HUD 위젯 (HP, MP, Identity 게이지 관리)
 */
UCLASS()
class LOSTARK_API ULostArkHUDWidget : public ULostArkUserWidget
{
	GENERATED_BODY()

public:
	// 위젯이 생성되고 시스템 컴포넌트가 유효할 때 호출하여 델리게이트를 바인딩합니다.
	UFUNCTION(BlueprintCallable, Category = "UI|HUD")
	virtual void BindAttributeDelegates();

protected:
	// 속성 변경 시 블루프린트로 이벤트 전달 (UI 연출용)
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|HUD")
	void OnHealthChanged(float CurrentHealth, float MaxHealth);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|HUD")
	void OnManaChanged(float CurrentMana, float MaxMana);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|HUD")
	void OnIdentityGaugeChanged(float CurrentGauge, float MaxGauge);

private:
	void HealthChanged(const FOnAttributeChangeData& Data);
	void MaxHealthChanged(const FOnAttributeChangeData& Data);
	void ManaChanged(const FOnAttributeChangeData& Data);
	void MaxManaChanged(const FOnAttributeChangeData& Data);
	void IdentityGaugeChanged(const FOnAttributeChangeData& Data);
	void MaxIdentityGaugeChanged(const FOnAttributeChangeData& Data);

	// 현재 속성 값 캐싱
	float CachedHealth = 0.f;
	float CachedMaxHealth = 0.f;
	float CachedMana = 0.f;
	float CachedMaxMana = 0.f;
	float CachedIdentityGauge = 0.f;
	float CachedMaxIdentityGauge = 0.f;
};
