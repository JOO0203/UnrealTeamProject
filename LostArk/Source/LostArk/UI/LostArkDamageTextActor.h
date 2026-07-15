#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LostArk/System/LostArkPoolableInterface.h"
#include "LostArkDamageTextActor.generated.h"

class UWidgetComponent;

/**
 */
UCLASS()
class LOSTARK_API ALostArkDamageTextActor : public AActor, public ILostArkPoolableInterface
{
	GENERATED_BODY()
	
public:	
	ALostArkDamageTextActor();

	virtual void OnAcquiredFromPool_Implementation() override;
	virtual void OnReleasedToPool_Implementation() override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Damage Text")
	void SetupDamageText(float DamageAmount);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* DamageTextWidgetComponent;

	UFUNCTION(BlueprintCallable, Category = "Damage Text")
	void ReturnToPool();
};
