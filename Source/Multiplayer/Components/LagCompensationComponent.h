#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

/** Server-side Rewind를 구현
 *  Recording Frame History: 모든 Player들의 위치를 기록
 */

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;
	UPROPERTY()
	FRotator Rotation;
	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;
	UPROPERTY() // 각 Bone(=FName)에 매 프레임 BoxInformation를 기록할 때 사용할 변수
	TMap<FName, FBoxInformation> HitBoxInfo;
	UPROPERTY()
	class ABaseCharacter* Character;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;
	UPROPERTY()
	bool bHeadShot; 
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()
	// HitCharacter가 얼마나 많이 피격됬는지 알아야되기 때문에 TMap 사용
	UPROPERTY()
	TMap<ABaseCharacter*, uint32> HeadShots;
	UPROPERTY()
	TMap<ABaseCharacter*, uint32> BodyShots;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MULTIPLAYER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULagCompensationComponent();
	friend class ABaseCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);

	//** Hitscan
	FServerSideRewindResult ServerSideRewind(
		ABaseCharacter* HitCharacter, 
		const FVector_NetQuantize& TraceStart, 
		const FVector_NetQuantize& HitLocation, 
		float HitTime); // Server-side Rewinding Time 알고리즘

	//** Projectile
	FServerSideRewindResult ProjectileServerSideRewind(
		ABaseCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime); // Server-side Rewinding Time 알고리즘

	//** Shotgun
	FShotgunServerSideRewindResult ShotgunServerSideRewind(
		const TArray<ABaseCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime); // Server-side Rewinding Time 알고리즘

	//** Hitscan
	UFUNCTION(Server, Reliable) // Client으로부터 call되어 Server에서 실행된다.
	void ServerScoreRequest(
		ABaseCharacter* HitCharacter, 
		const FVector_NetQuantize& TraceStart, 
		const FVector_NetQuantize& HitLocation, 
		float HitTime, 
		class AWeapon* DamageCauser);

	//** Projectile
	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(
		ABaseCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime);

	//** Shotgun
	UFUNCTION(Server, Reliable) // Client으로부터 call되어 Server에서 실행된다.
	void ShotgunServerScoreRequest(
		const TArray<ABaseCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime);

protected:
	virtual void BeginPlay() override;
	void SaveFramePackage();
	void SaveFramePackage(FFramePackage& Package); // 오버로딩
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);
	void CacheBoxPositions(ABaseCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package);
	void ResetHitBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package);
	void EnableCharacterMeshCollision(ABaseCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);
	FFramePackage GetFrameToCheck(ABaseCharacter* HitCharacter, float HitTime);

	//** Hitscan
	FServerSideRewindResult ConfirmHit(
		const FFramePackage& Package, 
		ABaseCharacter* HitCharacter, 
		const FVector_NetQuantize& TraceStart, 
		const FVector_NetQuantize& HitLocation);

	//** Projectile
	FServerSideRewindResult ProjectileConfirmHit(
		const FFramePackage& Package,
		ABaseCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime);

	//** Shotgun
	FShotgunServerSideRewindResult ShotgunConfirmHit(
		const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations);

private:
	UPROPERTY()
	ABaseCharacter* Character;
	UPROPERTY()
	class AMainPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.0f;

};