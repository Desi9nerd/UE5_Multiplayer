#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MULTIPLAYER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuffComponent();
	friend class ABaseCharacter; // BaseCharacter Ŭ������ friend Ŭ������ ����.

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void Heal(float HealAmount, float HealingTime);

protected:
	virtual void BeginPlay() override;
	void HealRampUp(float DeltaTime);

private:
	TWeakObjectPtr<class ABaseCharacter> Character;

	bool bHealing = false; // ȸ�� true/false
	float HealingRate = 0; // ȸ���Ǵ� �ӵ�
	float AmountToHeal = 0.0f; // ü�� ȸ����ġ

};
