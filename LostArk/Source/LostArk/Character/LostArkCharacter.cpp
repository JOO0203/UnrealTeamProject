#include "LostArk/Character/LostArkCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "LostArk/Character/LostArkAttributeSet.h"
#include "LostArk/Ability/LostArkCharacterComboAttackAbility.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "LostArk/UI/LostArkDamageTextActor.h"
#include "LostArk/System/LostArkObjectPoolSubsystem.h"
#include "LostArk/UI/LostArkDamageTextActor.h"
#include "LostArk/System/LostArkObjectPoolSubsystem.h"

static const float DefaultCapsuleRadius = 42.f;
static const float DefaultCapsuleHalfHeight = 96.f;
static const float CharacterDefaultRotationRateYaw = 640.f;
static const float DefaultCameraBoomLength = 800.f;
static const float DefaultCameraBoomPitch = -60.f;
static const float DefaultBaseRunSpeed = 600.f;

ALostArkCharacter::ALostArkCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(DefaultCapsuleRadius, DefaultCapsuleHalfHeight);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, CharacterDefaultRotationRateYaw, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion = true;

	BaseRunSpeed = DefaultBaseRunSpeed;
	GetCharacterMovement()->MaxWalkSpeed = BaseRunSpeed;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = DefaultCameraBoomLength;
	CameraBoom->SetRelativeRotation(FRotator(DefaultCameraBoomPitch, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<ULostArkAttributeSet>(TEXT("AttributeSet"));

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh());

	bIsLeftFootForward = true;
	bIsDead = false;
	bIsWeaponEquipped = false;

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

UAbilitySystemComponent* ALostArkCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ALostArkCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (WeaponMesh && GetMesh())
	{
		WeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponUnequippedSocketName);
	}
}

void ALostArkCharacter::SetWeaponEquipped(bool bIsEquipped)
{
	bIsWeaponEquipped = bIsEquipped;
	
	if (WeaponMesh && GetMesh())
	{
		FName TargetSocket = bIsEquipped ? WeaponEquippedSocketName : WeaponUnequippedSocketName;
		WeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetSocket);
	}
}

void ALostArkCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (WeaponMesh && GetMesh())
	{
		// 비전투 상태(등에 맴)로 시작
		WeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponUnequippedSocketName);
	}
}

void ALostArkCharacter::SetWeaponEquipped(bool bIsEquipped)
{
	if (WeaponMesh && GetMesh())
	{
		FName TargetSocket = bIsEquipped ? WeaponEquippedSocketName : WeaponUnequippedSocketName;
		WeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetSocket);
	}
}

void ALostArkCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		if (HasAuthority())
		{
			if (ComboAttackAbilityClass && !ComboAttackAbilityHandle.IsValid())
			{
				ComboAttackAbilityHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(ComboAttackAbilityClass, 1, 0, this));
			}

			for (const FLostArkSkillInputBind& Bind : SkillInputBinds)
			{
				if (Bind.AbilityClass)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Character] GiveAbility Called for %s"), *Bind.AbilityClass->GetName());
					AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Bind.AbilityClass, 1, static_cast<int32>(Bind.InputID), this));
				}
			}
		}

		AbilitySystemComponent->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag(FName("State.Attacking"), false), EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ALostArkCharacter::OnAttackingTagChanged);
	}
}

void ALostArkCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		for (const FLostArkSkillInputBind& Bind : SkillInputBinds)
		{
			if (Bind.InputAction)
			{
				EnhancedInputComponent->BindAction(Bind.InputAction, ETriggerEvent::Started, this, &ALostArkCharacter::OnSkillInputPressed, Bind.InputID);
				EnhancedInputComponent->BindAction(Bind.InputAction, ETriggerEvent::Completed, this, &ALostArkCharacter::OnSkillInputReleased, Bind.InputID);
			}
		}
	}
}

void ALostArkCharacter::ShowDamageText(float DamageAmount)
{
	if (DamageTextClass)
	{
		if (ULostArkObjectPoolSubsystem* Pool = GetWorld()->GetSubsystem<ULostArkObjectPoolSubsystem>())
		{
			// 탑뷰(쿼터뷰) 카메라 거리를 고려하여 오프셋(흔들림) 범위를 설정합니다.
			float RandomX = FMath::RandRange(-50.f, 50.f);
			float RandomY = FMath::RandRange(-50.f, 50.f);
			float RandomZ = FMath::RandRange(50.f, 150.f);
			FVector SpawnLoc = GetActorLocation() + FVector(RandomX, RandomY, RandomZ);
			
			if (AActor* SpawnedText = Pool->AcquireActor(DamageTextClass, SpawnLoc, FRotator::ZeroRotator))
			{
				if (ALostArkDamageTextActor* TextActor = Cast<ALostArkDamageTextActor>(SpawnedText))
				{
					TextActor->SetupDamageText(DamageAmount);
				}
			}
		}
	}
}

void ALostArkCharacter::RequestComboAttackInput()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(ComboAttackAbilityHandle);
	if (Spec && Spec->IsActive())
	{
		ULostArkCharacterComboAttackAbility* ComboAbility = Cast<ULostArkCharacterComboAttackAbility>(Spec->GetPrimaryInstance());
		if (ComboAbility)
		{
			ComboAbility->RequestComboInput();
		}
	}
}

void ALostArkCharacter::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead"), false));
		AbilitySystemComponent->CancelAllAbilities();
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
}

void ALostArkCharacter::SetCombatState(FGameplayTag NewStateTag)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (CurrentStateTag.IsValid())
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(CurrentStateTag);
	}

	CurrentStateTag = NewStateTag;

	if (CurrentStateTag.IsValid())
	{
		AbilitySystemComponent->AddLooseGameplayTag(CurrentStateTag);
	}
}

void ALostArkCharacter::OnSkillInputPressed(ELostArkAbilityInputID InputID)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(InputID));
	}
}

void ALostArkCharacter::OnSkillInputReleased(ELostArkAbilityInputID InputID)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityLocalInputReleased(static_cast<int32>(InputID));
	}
}

void ALostArkCharacter::OnAttackingTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		// 전투(공격) 진입 시 즉시 장착
		GetWorldTimerManager().ClearTimer(SheathWeaponTimerHandle);
		SetWeaponEquipped(true);
	}
	else
	{
		// 전투 종료 시 타이머 시작
		if (SheathWeaponTimeout > 0.f)
		{
			GetWorldTimerManager().SetTimer(SheathWeaponTimerHandle, this, &ALostArkCharacter::PlaySheathWeaponMontage, SheathWeaponTimeout, false);
		}
		else
		{
			PlaySheathWeaponMontage();
		}
	}
}

void ALostArkCharacter::PlaySheathWeaponMontage()
{
	if (SheathWeaponMontage)
	{
		PlayAnimMontage(SheathWeaponMontage);
	}
	else
	{
		// 설정된 몽타주가 없으면 즉시 납도
		SetWeaponEquipped(false);
	}
}