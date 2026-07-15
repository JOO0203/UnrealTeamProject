#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LostArk/System/LostArkPoolableInterface.h"
#include "LostArkDamageTextActor.generated.h"

class UWidgetComponent;

/**
 * 데미지 발생 시 월드(또는 화면)에 띄우는 텍스트 액터
 */
UCLASS()
class LOSTARK_API ALostArkDamageTextActor : public AActor, public ILostArkPoolableInterface
{
	GENERATED_BODY()
	
public:	
	ALostArkDamageTextActor();

	virtual void OnAcquiredFromPool_Implementation() override;
	virtual void OnReleasedToPool_Implementation() override;

	// 데미지 수치 세팅 (블루프린트 위젯 업데이트 및 연출 시작)
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Damage Text")
	void SetupDamageText(float DamageAmount);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* DamageTextWidgetComponent;

	// 애니메이션 연출(타임라인 등) 종료 시 블루프린트에서 직접 호출하여 풀에 반납
	UFUNCTION(BlueprintCallable, Category = "Damage Text")
	void ReturnToPool();
};
