#pragma once
#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "RocketMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYER_API URocketMovementComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()

protected:
	virtual EHandleBlockingHitResult HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining) override; // UProjectileMovementComponent�� ���ǵ� �Լ� ������.
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.f, const FVector& MoveDelta = FVector::ZeroVector) override; 
};

