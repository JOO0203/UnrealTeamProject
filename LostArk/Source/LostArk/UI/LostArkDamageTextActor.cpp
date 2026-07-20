#include "LostArk/UI/LostArkDamageTextActor.h"
#include "Components/WidgetComponent.h"
#include "LostArk/System/LostArkObjectPoolSubsystem.h"

ALostArkDamageTextActor::ALostArkDamageTextActor()
{
	PrimaryActorTick.bCanEverTick = false;

	DamageTextWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageTextWidgetComponent"));
	RootComponent = DamageTextWidgetComponent;

	// 스크린 스페이스(Screen Space) 모드로 설정하여 UI 캔버스에 직접 렌더링되게 함
	DamageTextWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DamageTextWidgetComponent->SetDrawSize(FVector2D(200.f, 50.f));
}

void ALostArkDamageTextActor::BeginPlay()
{
	Super::BeginPlay();
}

void ALostArkDamageTextActor::OnAcquiredFromPool_Implementation()
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
}

void ALostArkDamageTextActor::OnReleasedToPool_Implementation()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void ALostArkDamageTextActor::ReturnToPool()
{
	if (UWorld* World = GetWorld())
	{
		if (ULostArkObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<ULostArkObjectPoolSubsystem>())
		{
			PoolSubsystem->ReleaseActor(this);
		}
	}
}
