#include "RocketMovementComponent.h"

URocketMovementComponent::EHandleBlockingHitResult URocketMovementComponent::HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining); // �θ� ��� Super

	return EHandleBlockingHitResult::AdvanceNextSubstep; //������ ����ؼ� ���ư��� ���ش�.
}

void URocketMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	// ������ ���߸� �ȵ�. CollisionBox�� Hit ó���Ǿ������� �����ؾ���. 
}