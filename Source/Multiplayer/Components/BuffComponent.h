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

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<class ABaseCharacter> Character;

};
