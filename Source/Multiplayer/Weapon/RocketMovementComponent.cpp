#include "RocketMovementComponent.h"

URocketMovementComponent::EHandleBlockingHitResult URocketMovementComponent::HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining); // 부모 기능 Super

	return EHandleBlockingHitResult::AdvanceNextSubstep; //로켓이 계속해서 날아가게 해준다.
}

void URocketMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	// 로켓은 멈추면 안됨. CollisionBox가 Hit 처리되었을때만 폭발해야함. 
}